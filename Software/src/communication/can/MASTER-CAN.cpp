#include "MASTER-CAN.h"

#include <Arduino.h>

#include "../../battery/BATTERIES.h"
#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

MasterCan master_can;

// Voltage threshold for contactor safety (same as check_interconnect_available)
static const uint16_t VOLTAGE_DIFF_THRESHOLD_dV = 15;  // 1.5V
static const uint8_t VOLTAGE_DIFF_SECONDS_LIMIT = 10;  // 10s grace period
static const uint8_t MASTER_CONTACTOR_STAGGER_MS = 3;  // master-only inter-frame spacing

// Pre-join controller constants (master-only):
// adaptively reduce pack power while an additional blocked slave is preparing to join.
static const uint16_t PREJOIN_ENTER_DIFF_dV = 18u;          // 1.8V — activate when EMA diff <= this
static const uint16_t PREJOIN_PRESSURE_START_DIFF_dV = 0u;  // TEST: pressure disabled — full power throughout prejoin
static const uint16_t PREJOIN_CLOSE_RAW_DIFF_dV = 0u;       // TEST: close at diff = 0V
static const uint16_t PREJOIN_PRESSURE_FULL_DIFF_dV = 0u;   // TEST: unused (pressure disabled)
static const uint8_t PREJOIN_CLOSE_DWELL_S = 2u;
// Fixed inverter-independent prejoin floor profile:
// 1 joined battery => 1000W, each additional joined battery adds +400W,
// capped at 3000W.
static const uint16_t PREJOIN_FLOOR_BASE_W = 1000u;
static const uint16_t PREJOIN_FLOOR_PER_EXTRA_BATTERY_W = 400u;
static const uint16_t PREJOIN_FLOOR_MAX_W = 3000u;
// Asymmetric EMA: slow when diff falls (avoids cutting cap prematurely), fast when diff rises (releases cap quickly).
static const uint8_t PREJOIN_EMA_DEN_FALL = 5u;  // alpha=0.2 — diff falling → pressure would rise → be conservative
static const uint8_t PREJOIN_EMA_DEN_RISE = 8u;  // alpha=0.125 — diff rising → damped so brief spikes don't crash target
// Smooth cap ramp in permille per second (0..1000) — 15 = 1.5%/s, ~66 s to full pressure
static const uint16_t PREJOIN_PRESSURE_STEP_PER_S = 8u;
// Abort an active prejoin if load stays below this for too long (solar disappears mid-prejoin)
static const uint16_t PREJOIN_LOW_POWER_ABORT_W = 300u;
static const uint8_t PREJOIN_LOW_POWER_ABORT_S = 30u;

// Per-slave voltage diff grace period counters
static uint8_t voltage_diff_seconds[MAX_SLAVE_NODES] = {0};

// How long to keep the contactor allowed after offline balancing starts.
// Keep it allowed for 50s, then force block while balancing is active.
static const uint8_t BALANCING_HOLD_SECONDS = 50u;
// Per-slave countdown: while > 0, contactor_allowed is not yet forced false
static uint8_t balancing_hold_seconds[MAX_SLAVE_NODES] = {0};
// Per-slave last transmitted contactor command for change logging
static uint8_t last_contactor_command[MAX_SLAVE_NODES] = {0};
// Per-slave log-once flags — reset when the condition clears
static bool stale_logged[MAX_SLAVE_NODES] = {false};
static bool ident_mismatch_logged[MAX_SLAVE_NODES] = {false};

// Per-slave pre-join tracking
static bool prejoin_active[MAX_SLAVE_NODES] = {false};
static uint16_t prejoin_diff_ema_dV[MAX_SLAVE_NODES] = {0};
static uint8_t prejoin_raw_stable_seconds[MAX_SLAVE_NODES] = {0};
static uint16_t prejoin_pressure_permille[MAX_SLAVE_NODES] = {0};
// Per-slave low-power abort counter: how many seconds load has been below PREJOIN_LOW_POWER_ABORT_W
static uint8_t prejoin_low_power_seconds[MAX_SLAVE_NODES] = {0};
// Global applied pressure (used to cap BOTH charge and discharge simultaneously)
static uint16_t prejoin_applied_pressure_permille = 0;
// Snapshot of cap base at the moment a new prejoin session begins (rising edge of any_prejoin_active).
// Holds base_* stable throughout the session so mid-session slave reporting changes cannot shift the curve.
static uint32_t prejoin_cap_snapshot_charge_W = 0;
static uint32_t prejoin_cap_snapshot_discharge_W = 0;
static bool prejoin_was_active = false;  // edge detector

static uint8_t count_joined_slaves() {
  uint8_t joined_count = 0;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    const auto& node = datalayer.system.slave_nodes[i];
    if (node.online && node.contactor_allowed && !node.balancing) {
      joined_count++;
    }
  }
  return joined_count;
}

static uint32_t prejoin_floor_w_for_joined(uint8_t joined_count) {
  uint32_t floor_W = PREJOIN_FLOOR_BASE_W;
  if (joined_count > 1u) {
    floor_W += (uint32_t)(joined_count - 1u) * PREJOIN_FLOOR_PER_EXTRA_BATTERY_W;
  }
  if (floor_W > PREJOIN_FLOOR_MAX_W) {
    floor_W = PREJOIN_FLOOR_MAX_W;
  }
  return floor_W;
}

static void reset_prejoin_state(uint8_t idx) {
  prejoin_active[idx] = false;
  datalayer.system.slave_nodes[idx].prejoin_active = false;
  prejoin_diff_ema_dV[idx] = 0;
  prejoin_raw_stable_seconds[idx] = 0;
  prejoin_pressure_permille[idx] = 0;
  prejoin_low_power_seconds[idx] = 0;
}

void setup_master_can() {
  // Register as a virtual battery so safety.cpp and the rest of the system
  // treat the master exactly like a standalone battery node.
  battery = new InterUnitMasterBattery();
  battery->setup();

  master_can.begin();
  register_can_receiver(&master_can, can_config.inter_unit, CAN_Speed::CAN_SPEED_500KBPS);
  register_transmitter(&master_can);
  logging.println("Master CAN: registered on inter-unit bus @ 500kbps");
}

void MasterCan::begin() {
  _last_heartbeat_ms = 0;
  _next_contactor_tx_ms = 0;
  _next_contactor_idx = 0;
  _contactor_burst_pending = false;
  _startup_begin_ms = 0;
  _startup_grace_done = false;
  _estop_was_active = false;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    voltage_diff_seconds[i] = 0;
    balancing_hold_seconds[i] = 0;
    last_contactor_command[i] = 0xFFu;
    stale_logged[i] = false;
    ident_mismatch_logged[i] = false;
    reset_prejoin_state(i);
    // Force all contactors blocked on (re)start — RAM may retain state from before
    datalayer.system.slave_nodes[i].contactor_allowed = false;
    datalayer.system.slave_nodes[i].online = false;
    datalayer.system.slave_nodes[i].still_alive = 0;
  }
  prejoin_applied_pressure_permille = 0;
  prejoin_cap_snapshot_charge_W = 0;
  prejoin_cap_snapshot_discharge_W = 0;
}

// ---- CAN RX --------------------------------------------------------

void MasterCan::receive_can_frame(CAN_frame* rx_frame) {
  const uint32_t id = rx_frame->ID;

  // Only process slave messages in the expected range
  if (id < IU_SLAVE_MSG_MIN_ID || id > IU_SLAVE_MSG_MAX_ID) {
    return;
  }

  // Decode node_id and sub-message from ID
  // IU_SLAVE_STATUS_ID(n) = 0x100 + n*0x10 + 0x00
  // IU_SLAVE_POWER_ID(n)  = 0x100 + n*0x10 + 0x01
  // IU_SLAVE_INFO_ID(n)   = 0x100 + n*0x10 + 0x02
  uint32_t offset = id - 0x100u;
  uint8_t node_id = (uint8_t)(offset >> 4);  // high nibble = node ID (1..8)
  uint8_t sub = (uint8_t)(offset & 0x0Fu);   // low nibble = sub-message (0..2)

  if (node_id < 1 || node_id > MAX_SLAVE_NODES) {
    return;
  }

  SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[node_id - 1];

  // Reset still_alive counter (60s timeout)
  node.still_alive = IU_OFFLINE_TIMEOUT_S;
  node.online = true;
  // Note: EVENT_SLAVE_BATTERY_MISSING is intentionally NOT cleared here so the
  // event history shows that a slave was offline. Contactor is re-allowed by
  // check_slave_voltage_safety() once voltage is within ±1.5V.

  switch (sub) {
    case 0x00:  // STATUS message
    {
      uint16_t voltage_dV = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      uint16_t real_soc = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      int16_t current_dA = (int16_t)(((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5]);
      int8_t temp_max_raw = (int8_t)rx_frame->data.u8[6];
      uint8_t flags = rx_frame->data.u8[7];

      node.voltage_dV = voltage_dV;
      node.real_soc = real_soc;
      node.current_dA = current_dA;
      node.temp_max_dC = temp_max_raw;  // stored as °C (not deci)
      // Strip non-fault bits from stored fault_flags; handle toggle separately
      uint8_t toggle_now = flags & IU_FLAG_STATUS_TOGGLE;
      node.fault_flags = flags & ~(IU_FLAG_CONTACTOR_ENGAGED | IU_FLAG_STATUS_TOGGLE);
      node.contactor_engaged = (flags & IU_FLAG_CONTACTOR_ENGAGED) != 0;
      // Toggle bit changed -> data is fresh, reset stale counter
      if (toggle_now != node._last_status_toggle) {
        node._last_status_toggle = toggle_now;
        node.status_stale_seconds = 0;
      }
      break;
    }
    case 0x01:  // POWER message
    {
      node.max_charge_W = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      node.max_discharge_W = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      node.remaining_Wh = ((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5];
      node.temp_min_dC = (int8_t)rx_frame->data.u8[6];
      // [7] slave_pflags: check offline balancing flag
      bool was_balancing = node.balancing;
      node.balancing = (rx_frame->data.u8[7] & IU_SLAVE_PFLAG_BALANCING) != 0;
      if (!was_balancing && node.balancing) {
        // Balancing just started: start hold timer — BMW I3 opens its own contactor ~20s later.
        // Master will block contactor_allowed after BALANCING_HOLD_SECONDS (50s).
        balancing_hold_seconds[node_id - 1] = BALANCING_HOLD_SECONDS;
        logging.printf("Master CAN: Slave %d offline balancing started — contactor held for %us\n", node_id,
                       BALANCING_HOLD_SECONDS);
      }
      break;
    }
    case 0x02:  // INFO message (every 10s)
    {
      node.total_capacity_Wh = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      node.max_design_voltage_dV = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      node.min_design_voltage_dV = ((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5];
      node.soh_pptt = ((uint16_t)rx_frame->data.u8[6] << 8) | rx_frame->data.u8[7];
      break;
    }
    case 0x03:  // IP address message (every 10s)
    {
      if (rx_frame->DLC >= 4) {
        node.ip_address = ((uint32_t)rx_frame->data.u8[0] << 24) | ((uint32_t)rx_frame->data.u8[1] << 16) |
                          ((uint32_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      }
      break;
    }
    case 0x04:  // CELL message (every 2s)
    {
      if (rx_frame->DLC >= 4) {
        node.cell_max_voltage_mV = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
        node.cell_min_voltage_mV = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      }
      break;
    }
    case 0x05:  // IDENT message (startup only)
    {
      if (rx_frame->DLC >= 4) {
        node.fw_version_num = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
        node.battery_type_id = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
        node.ident_received = true;
      }
      break;
    }
    default:
      break;
  }
}

// ---- CAN TX --------------------------------------------------------

void MasterCan::transmit(unsigned long currentMillis) {
  if (currentMillis - _last_heartbeat_ms >= IU_HEARTBEAT_INTERVAL_MS) {
    _last_heartbeat_ms = currentMillis;
    send_heartbeat();

    // Start a fresh burst of per-slave contactor commands after each heartbeat.
    // Commands are staggered over a few milliseconds to avoid native CAN TX bursts.
    _next_contactor_idx = 0;
    _contactor_burst_pending = true;
    _next_contactor_tx_ms = currentMillis + 1u;
  }

  send_contactor_commands(currentMillis);
}

void MasterCan::send_heartbeat() {
  CAN_frame frame = {};
  frame.ID = IU_MASTER_HEARTBEAT_ID;
  frame.DLC = 0;
  frame.ext_ID = false;
  transmit_can_frame_to_interface(&frame, can_config.inter_unit);
}

void MasterCan::send_contactor_commands(unsigned long currentMillis) {
  if (!_contactor_burst_pending) {
    return;
  }

  if ((long)(currentMillis - _next_contactor_tx_ms) < 0) {
    return;
  }

  // Send one online slave command per slot, then wait for the next stagger step.
  while (_next_contactor_idx < MAX_SLAVE_NODES) {
    uint8_t i = _next_contactor_idx++;
    SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (!node.online) {
      continue;
    }

    uint8_t node_id = i + 1;
    CAN_frame frame = {};
    frame.ID = IU_MASTER_CONTACTOR_ID(node_id);
    frame.DLC = 1;
    frame.ext_ID = false;
    bool allow_command = node.contactor_allowed && datalayer.system.status.inverter_allows_contactor_closing &&
                         !datalayer.system.info.equipment_stop_active;
    frame.data.u8[0] = allow_command ? IU_CONTACTOR_ALLOW : IU_CONTACTOR_OPEN;

    if (last_contactor_command[i] != frame.data.u8[0]) {
      last_contactor_command[i] = frame.data.u8[0];
      logging.printf(
          "Master CAN: TX contactor cmd slave %u -> %s (node_allowed=%u inverter_allow=%u estop=%u pack_allow=%u "
          "online=%u)\n",
          node_id, allow_command ? "ALLOW" : "OPEN", node.contactor_allowed ? 1 : 0,
          datalayer.system.status.inverter_allows_contactor_closing ? 1 : 0,
          datalayer.system.info.equipment_stop_active ? 1 : 0,
          datalayer.system.status.battery_allows_contactor_closing ? 1 : 0, node.online ? 1 : 0);
    }

    transmit_can_frame_to_interface(&frame, can_config.inter_unit);
    _next_contactor_tx_ms = currentMillis + MASTER_CONTACTOR_STAGGER_MS;
    return;
  }

  _contactor_burst_pending = false;
}

// ---- Value aggregation (called every 1s) ---------------------------

void MasterCan::update_values() {
  bool estop_active = datalayer.system.info.equipment_stop_active;

  // Start grace timer the moment the first slave comes online
  if (_startup_begin_ms == 0) {
    for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
      if (datalayer.system.slave_nodes[i].online) {
        _startup_begin_ms = millis();
        logging.printf("Master CAN: First slave online — grace period started (%u s)\n", IU_STARTUP_GRACE_S);
        break;
      }
    }
  }

  // Grace period: when it expires, move from startup hold to per-slave voltage control.
  // This only marks startup as complete; actual contactor permission is still
  // decided per slave by voltage safety.
  if (!_startup_grace_done && _startup_begin_ms != 0) {
    unsigned long elapsed_s = (millis() - _startup_begin_ms) / 1000UL;
    if (elapsed_s >= IU_STARTUP_GRACE_S) {
      _startup_grace_done = true;
      uint16_t reference_voltage_dV = 0;
      bool reference_found = false;
      bool all_online_slaves_match = true;
      uint8_t reference_slave_id = 0;

      for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
        const SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
        voltage_diff_seconds[i] = 0;

        if (!node.online || node.balancing || node.voltage_dV == 0) {
          continue;
        }

        if (!reference_found) {
          reference_voltage_dV = node.voltage_dV;
          reference_slave_id = i + 1;
          reference_found = true;
          continue;
        }

        uint16_t diff = (node.voltage_dV > reference_voltage_dV) ? (node.voltage_dV - reference_voltage_dV)
                                                                 : (reference_voltage_dV - node.voltage_dV);
        if (diff > VOLTAGE_DIFF_THRESHOLD_dV) {
          all_online_slaves_match = false;
        }
      }

      if (reference_found) {
        logging.printf("Master CAN: Grace reference slave %u at %u.%uV\n", reference_slave_id,
                       reference_voltage_dV / 10, reference_voltage_dV % 10);
      }

      if (reference_found && all_online_slaves_match) {
        for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
          SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
          if (node.online && !node.balancing && node.voltage_dV > 0) {
            node.contactor_allowed = true;
            logging.printf("Master CAN: Grace done — Slave %d contactor ALLOWED together (voltage %u.%uV)\n", i + 1,
                           node.voltage_dV / 10, node.voltage_dV % 10);
          }
        }
        logging.println("Master CAN: Startup grace done — matched online slaves may close together");
      } else {
        logging.println("Master CAN: Startup grace done — voltage safety now controls contactor permission");
      }
    }
  }

  if (estop_active && !_estop_was_active) {
    logging.println("Master CAN: E-stop ACTIVE — resetting slave contactor permissions and voltage qualification");
  } else if (!estop_active && _estop_was_active) {
    logging.println("Master CAN: E-stop CLEARED — resuming contactor logic immediately");
  }
  _estop_was_active = estop_active;

  // Decrement still_alive counters
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (!node.online) {
      reset_prejoin_state(i);
      continue;
    }

    if (estop_active) {
      voltage_diff_seconds[i] = 0;
      reset_prejoin_state(i);
      if (node.contactor_allowed) {
        node.contactor_allowed = false;
        logging.printf("Master CAN: Slave %d contactor BLOCKED by E-stop\n", i + 1);
      }
    }

    if (node.still_alive > 0) {
      node.still_alive--;
    }
    if (node.still_alive == 0) {
      node.online = false;
      node.contactor_allowed = false;
      reset_prejoin_state(i);
      set_event(EVENT_SLAVE_BATTERY_MISSING, i + 1);  // data = node ID (1-8)
      logging.printf("Master CAN: Slave %d went OFFLINE\n", i + 1);
    }

    // Offline balancing hold timer: once expired, block the contactor
    if (node.balancing) {
      reset_prejoin_state(i);
      if (balancing_hold_seconds[i] > 0) {
        balancing_hold_seconds[i]--;
      } else if (node.contactor_allowed) {
        node.contactor_allowed = false;
        logging.printf("Master CAN: Slave %d balancing hold expired — contactor BLOCKED\n", i + 1);
      }
    }

    // Check voltage safety first (may allow contactor based on voltage match)
    // Skip entirely while e-stop is active — contactor must stay blocked
    if (!estop_active) {
      check_slave_voltage_safety(i);
    }

    // Stale data detection: if STATUS toggle bit has not changed for IU_STATUS_STALE_SECONDS,
    // the slave is frozen/stuck — block contactor and fire stale event.
    // This runs AFTER voltage safety so it always overrides any re-allow.
    if (node.online && node._last_status_toggle != 0xFF) {
      node.status_stale_seconds++;
      if (node.status_stale_seconds > IU_STATUS_STALE_SECONDS) {
        node.contactor_allowed = false;
        set_event(EVENT_STALE_VALUE, i + 1);
        if (!stale_logged[i]) {
          stale_logged[i] = true;
          logging.printf("Master CAN: Slave %d STATUS is STALE (%u s)\n", i + 1, node.status_stale_seconds);
        }
      } else {
        stale_logged[i] = false;
        clear_event(EVENT_STALE_VALUE);
      }
    }

    // IDENT mismatch: firmware version or battery type mismatch blocks contactor.
    // Only check after IDENT has been received from this slave.
    // Runs AFTER voltage safety so it always overrides any re-allow.
    if (node.online && node.ident_received) {
      bool fw_ok = (node.fw_version_num == (uint16_t)IU_FW_VERSION_NUM);
      // Battery type must match the first slave that reported one (used as reference).
      // Find the reference battery_type_id from the first slave that has ident_received.
      uint16_t ref_btype = 0;
      bool ref_found = false;
      for (uint8_t j = 0; j < MAX_SLAVE_NODES; j++) {
        if (datalayer.system.slave_nodes[j].ident_received) {
          ref_btype = datalayer.system.slave_nodes[j].battery_type_id;
          ref_found = true;
          break;
        }
      }
      bool btype_ok = (!ref_found || node.battery_type_id == ref_btype);
      if (!fw_ok || !btype_ok) {
        node.contactor_allowed = false;
        set_event(EVENT_SLAVE_IDENT_MISMATCH, i + 1);
        if (!ident_mismatch_logged[i]) {
          ident_mismatch_logged[i] = true;
          logging.printf("Master CAN: Slave %d IDENT mismatch (fw=0x%04X exp=0x%04X btype=%u exp=%u)\n", i + 1,
                         node.fw_version_num, IU_FW_VERSION_NUM, node.battery_type_id, ref_btype);
        }
      } else {
        ident_mismatch_logged[i] = false;
        clear_event(EVENT_SLAVE_IDENT_MISMATCH);
      }
    }

    // Fault flag events: ERROR mask blocks contactor (red), WARNING mask is advisory (yellow).
    // Runs AFTER voltage safety so error faults always override any re-allow.
    if (node.online) {
      if (node.fault_flags & IU_FAULT_ERROR_MASK) {
        node.contactor_allowed = false;
      }
      // Slave warnings are shown per-node in the web UI — no master event needed
    }

    // Manual stop: user-forced contactor open via WebUI — always wins last.
    // Reset voltage qualification so slave re-joins cleanly when stop is cleared.
    if (node.manual_contactor_open) {
      voltage_diff_seconds[i] = 0;
      reset_prejoin_state(i);
      if (node.contactor_allowed) {
        node.contactor_allowed = false;
        logging.printf("Master CAN: Slave %d contactor BLOCKED by manual stop\n", i + 1);
      }
    }
  }

  // Set or clear EVENT_SLAVE_FAULT once after the full loop to avoid clear/set ping-pong
  // (clearing for slave N then immediately re-setting for slave N+1 would re-trigger the log).
  {
    uint8_t first_fault_node = 0;
    for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
      const SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
      if (node.online && (node.fault_flags & IU_FAULT_ERROR_MASK)) {
        first_fault_node = i + 1;
        break;
      }
    }
    if (first_fault_node > 0) {
      set_event(EVENT_SLAVE_FAULT, first_fault_node);
    } else {
      clear_event(EVENT_SLAVE_FAULT);
    }
  }

  // Aggregate all valid online slaves into datalayer.battery
  update_slave_aggregation();
}

/** Mimics check_interconnect_available() logic from parallel_safety.cpp */
void MasterCan::check_slave_voltage_safety(uint8_t idx) {
  SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[idx];
  if (!node.online || node.voltage_dV == 0) {
    reset_prejoin_state(idx);
    return;
  }

  // Offline balancing slaves are handled by balancing_hold_seconds logic.
  // Never re-allow from voltage matching while balancing is active.
  if (node.balancing) {
    voltage_diff_seconds[idx] = 0;
    reset_prejoin_state(idx);
    return;
  }

  // While e-stop is active, contactors must stay blocked and any pending
  // voltage qualification must restart when the stop is cleared.
  if (datalayer.system.info.equipment_stop_active) {
    voltage_diff_seconds[idx] = 0;
    reset_prejoin_state(idx);
    return;
  }

  // During startup grace period, keep all contactors blocked so all slaves
  // can announce themselves at matching voltages before the inverter starts.
  if (!_startup_grace_done) {
    reset_prejoin_state(idx);
    return;
  }

  // First slave to come online sets the reference voltage.
  // Additional slaves must stay within the voltage threshold for
  // VOLTAGE_DIFF_SECONDS_LIMIT consecutive update cycles before closing.
  uint16_t reference_voltage_dV = 0;
  for (uint8_t j = 0; j < MAX_SLAVE_NODES; j++) {
    if (j == idx)
      continue;
    if (datalayer.system.slave_nodes[j].online && datalayer.system.slave_nodes[j].voltage_dV > 0 &&
        datalayer.system.slave_nodes[j].contactor_allowed) {
      reference_voltage_dV = datalayer.system.slave_nodes[j].voltage_dV;
      break;
    }
  }

  if (reference_voltage_dV == 0) {
    // No reference available yet — allow this slave to connect
    if (!node.contactor_allowed) {
      node.contactor_allowed = true;
      voltage_diff_seconds[idx] = 0;
      reset_prejoin_state(idx);
      logging.printf("Master CAN: Slave %d contactor ALLOWED (first slave)\n", idx + 1);
    }
    return;
  }

  uint16_t diff = (node.voltage_dV > reference_voltage_dV) ? (node.voltage_dV - reference_voltage_dV)
                                                           : (reference_voltage_dV - node.voltage_dV);

  bool has_fault = (node.fault_flags != 0);

  if (!node.contactor_allowed && !has_fault) {
    // Track EMA of diff for adaptive pre-join power capping.
    if (prejoin_diff_ema_dV[idx] == 0) {
      prejoin_diff_ema_dV[idx] = diff;
    } else {
      uint8_t ema_den = (diff < prejoin_diff_ema_dV[idx]) ? PREJOIN_EMA_DEN_FALL : PREJOIN_EMA_DEN_RISE;
      prejoin_diff_ema_dV[idx] =
          (uint16_t)(((uint32_t)prejoin_diff_ema_dV[idx] * (ema_den - 1u) + (uint32_t)diff) / ema_den);
    }

    // Prejoin is only relevant while the inverter is actively working with batteries
    // that are already online. If the system is idle, fall through to the normal
    // 1.5 V (VOLTAGE_DIFF_THRESHOLD_dV) path which closes the contactor without capping.
    int32_t load_W = datalayer.battery.status.active_power_W;
    uint32_t abs_load_W = (load_W >= 0) ? (uint32_t)load_W : (uint32_t)(-load_W);
    // Use the fixed joined-battery floor for prejoin entry gating.
    // If load <= floor, there is no headroom to apply pressure capping.
    uint8_t joined_count = count_joined_slaves();
    uint32_t pj_floor_W = prejoin_floor_w_for_joined(joined_count);
    bool inverter_working = (abs_load_W > pj_floor_W);

    if (!prejoin_active[idx] && inverter_working && prejoin_diff_ema_dV[idx] <= PREJOIN_ENTER_DIFF_dV) {
      prejoin_active[idx] = true;
      prejoin_raw_stable_seconds[idx] = 0;
      logging.printf("Master CAN: Pre-join START slave %d (raw=%u.%uV ema=%u.%uV load=%dW floor=%uW)\n", idx + 1,
                     diff / 10, diff % 10, prejoin_diff_ema_dV[idx] / 10, prejoin_diff_ema_dV[idx] % 10,
                     (int)load_W, (unsigned)pj_floor_W);
    }

    if (prejoin_active[idx]) {
      // Abort prejoin if power stays below 300W for 30s — e.g. solar source disappears mid-prejoin
      if (abs_load_W < PREJOIN_LOW_POWER_ABORT_W) {
        if (prejoin_low_power_seconds[idx] < PREJOIN_LOW_POWER_ABORT_S) {
          prejoin_low_power_seconds[idx]++;
        }
        if (prejoin_low_power_seconds[idx] >= PREJOIN_LOW_POWER_ABORT_S) {
          logging.printf(
              "Master CAN: Pre-join ABORT slave %d — load %dW below %uW for %us\n", idx + 1, (int)load_W,
              (unsigned)PREJOIN_LOW_POWER_ABORT_W, (unsigned)PREJOIN_LOW_POWER_ABORT_S);
          reset_prejoin_state(idx);
        }
      } else {
        prejoin_low_power_seconds[idx] = 0;
      }

      if (diff <= PREJOIN_CLOSE_RAW_DIFF_dV) {
        if (prejoin_raw_stable_seconds[idx] < PREJOIN_CLOSE_DWELL_S) {
          prejoin_raw_stable_seconds[idx]++;
        }
      } else if (diff > PREJOIN_CLOSE_RAW_DIFF_dV + 1u) {
        prejoin_raw_stable_seconds[idx] = 0;
      }

      if (prejoin_diff_ema_dV[idx] <= PREJOIN_PRESSURE_FULL_DIFF_dV) {
        prejoin_pressure_permille[idx] = 1000u;
      } else if (prejoin_diff_ema_dV[idx] >= PREJOIN_PRESSURE_START_DIFF_dV) {
        prejoin_pressure_permille[idx] = 0u;  // full power above 0.6V
      } else {
        uint32_t span = (uint32_t)(PREJOIN_PRESSURE_START_DIFF_dV - PREJOIN_PRESSURE_FULL_DIFF_dV);  // 0.6V–0.3V = 3 dV
        uint32_t above_full = (uint32_t)(prejoin_diff_ema_dV[idx] - PREJOIN_PRESSURE_FULL_DIFF_dV);
        uint32_t norm = (span - above_full) * 1000u / span;  // 0 at PRESSURE_START, 1000 at PRESSURE_FULL
        prejoin_pressure_permille[idx] = (uint16_t)(norm * norm / 1000u);  // quadratic: steep near 0.3V
      }
      // Log per-slave state every second while prejoin is active
      logging.printf(
          "Master CAN: Pre-join slave %d: raw=%u.%uV ema=%u.%uV target=%u permille stable=%us current=%d.%uA\n",
          idx + 1, diff / 10, diff % 10, prejoin_diff_ema_dV[idx] / 10, prejoin_diff_ema_dV[idx] % 10,
          prejoin_pressure_permille[idx], prejoin_raw_stable_seconds[idx],
          (int)node.current_dA / 10, (unsigned)(abs((int)node.current_dA) % 10));
    } else {
      prejoin_pressure_permille[idx] = 0;
      prejoin_raw_stable_seconds[idx] = 0;
    }
  } else if (node.contactor_allowed) {
    reset_prejoin_state(idx);
  }

  if (has_fault) {
    voltage_diff_seconds[idx] = 0;
    reset_prejoin_state(idx);
    if (node.contactor_allowed) {
      node.contactor_allowed = false;
      logging.printf("Master CAN: Slave %d contactor OPENED (fault flags: 0x%02X)\n", idx + 1, node.fault_flags);
    }
  } else if (!node.contactor_allowed) {
    if (diff <= VOLTAGE_DIFF_THRESHOLD_dV) {
      if (voltage_diff_seconds[idx] < VOLTAGE_DIFF_SECONDS_LIMIT) {
        voltage_diff_seconds[idx]++;
      }
      if (voltage_diff_seconds[idx] >= VOLTAGE_DIFF_SECONDS_LIMIT) {
        bool prejoin_gate_ok = true;
        if (prejoin_active[idx]) {
          prejoin_gate_ok = (diff <= PREJOIN_CLOSE_RAW_DIFF_dV) &&
                            (prejoin_raw_stable_seconds[idx] >= PREJOIN_CLOSE_DWELL_S);
        }
        if (prejoin_gate_ok) {
          node.contactor_allowed = true;
          voltage_diff_seconds[idx] = 0;
          reset_prejoin_state(idx);
          logging.printf("Master CAN: Slave %d contactor ALLOWED (voltage OK for %us: %u.%uV)\n", idx + 1,
                         VOLTAGE_DIFF_SECONDS_LIMIT, node.voltage_dV / 10, node.voltage_dV % 10);
        }
      }
    } else {
      if (voltage_diff_seconds[idx] != 0) {
        logging.printf("Master CAN: Slave %d contactor BLOCKED (voltage diff %u.%uV > %u.%uV)\n", idx + 1, diff / 10,
                       diff % 10, VOLTAGE_DIFF_THRESHOLD_dV / 10, VOLTAGE_DIFF_THRESHOLD_dV % 10);
      }
      voltage_diff_seconds[idx] = 0;
    }
  } else {
    voltage_diff_seconds[idx] = 0;
  }
  // If the contactor is already allowed and there are no faults, we permit voltage differences without opening the contactor.
}

void MasterCan::update_slave_aggregation() {
  // Aggregate online slaves into datalayer.battery
  uint32_t total_capacity_Wh = 0;
  uint32_t total_remaining_Wh = 0;
  uint32_t total_max_charge_W = 0;
  uint32_t total_max_discharge_W = 0;
  int32_t total_current_dA = 0;
  uint16_t lowest_soc = 10001;  // Above max to detect "no data"
  uint16_t highest_soc = 0;
  int16_t highest_temp = -1270;
  int16_t lowest_temp = 1270;
  uint16_t shared_voltage_dV = 0;                 // All slaves share voltage (parallel)
  uint16_t lowest_max_design_voltage_dV = 65535;  // To safely limit inverter charge voltage
  uint16_t highest_min_design_voltage_dV = 0;     // To safely limit inverter discharge voltage
  uint16_t lowest_soh_pptt = 9900;                // Use lowest SOH across all slaves
  uint16_t max_cell_voltage_mV = 0;
  uint16_t min_cell_voltage_mV = 65535;
  uint8_t active_count = 0;
  // If any battery's BMS says stop charging/discharging, block all power flow.
  // This ensures we stop as soon as the first battery is full or empty,
  // protecting against overcharge/overdischarge in mixed battery setups.
  bool charge_blocked = false;
  bool discharge_blocked = false;

  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    const SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (!node.online || !node.contactor_allowed) {
      continue;
    }
    // Exclude slaves performing offline balancing — they are sleeping and not part of the pack
    if (node.balancing) {
      continue;
    }
    active_count++;
    if (shared_voltage_dV == 0 && node.voltage_dV > 0) {
      shared_voltage_dV = node.voltage_dV;  // Same for all — take first available
    }
    total_capacity_Wh += node.total_capacity_Wh;
    total_remaining_Wh += node.remaining_Wh;
    if (node.max_charge_W == 0) {
      charge_blocked = true;
    } else {
      total_max_charge_W += node.max_charge_W;
    }
    if (node.max_discharge_W == 0) {
      discharge_blocked = true;
    } else {
      total_max_discharge_W += node.max_discharge_W;
    }
    total_current_dA += node.current_dA;

    if (node.real_soc < lowest_soc) {
      lowest_soc = node.real_soc;
    }
    if (node.real_soc > highest_soc) {
      highest_soc = node.real_soc;
    }
    // temp stored as °C, convert to dC (×10) for datalayer
    int16_t t_max_dC = (int16_t)node.temp_max_dC * 10;
    int16_t t_min_dC = (int16_t)node.temp_min_dC * 10;
    if (t_max_dC > highest_temp)
      highest_temp = t_max_dC;
    if (t_min_dC < lowest_temp)
      lowest_temp = t_min_dC;

    if (node.max_design_voltage_dV > 0 && node.max_design_voltage_dV < lowest_max_design_voltage_dV) {
      lowest_max_design_voltage_dV = node.max_design_voltage_dV;
    }
    if (node.min_design_voltage_dV > highest_min_design_voltage_dV) {
      highest_min_design_voltage_dV = node.min_design_voltage_dV;
    }
    if (node.soh_pptt > 0 && node.soh_pptt < lowest_soh_pptt) {
      lowest_soh_pptt = node.soh_pptt;
    }
    if (node.cell_max_voltage_mV > max_cell_voltage_mV) {
      max_cell_voltage_mV = node.cell_max_voltage_mV;
    }
    if (node.cell_min_voltage_mV > 0 && node.cell_min_voltage_mV < min_cell_voltage_mV) {
      min_cell_voltage_mV = node.cell_min_voltage_mV;
    }
  }

  if (active_count == 0) {
    // No contactor_allowed slaves yet (during grace period or startup).
    // Show informational data from the first online slave so the inverter can
    // initialise, but hold charge/discharge rates at zero.
    for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
      const SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
      if (!node.online || node.balancing) {
        continue;
      }
      datalayer.battery.status.voltage_dV = node.voltage_dV;
      datalayer.battery.status.real_soc = node.real_soc;
      datalayer.battery.status.temperature_max_dC = (int16_t)node.temp_max_dC * 10;
      datalayer.battery.status.temperature_min_dC = (int16_t)node.temp_min_dC * 10;
      datalayer.battery.info.total_capacity_Wh = node.total_capacity_Wh;
      datalayer.battery.info.reported_total_capacity_Wh = node.total_capacity_Wh;
      datalayer.battery.status.remaining_capacity_Wh = node.remaining_Wh;
      datalayer.battery.status.reported_remaining_capacity_Wh = node.remaining_Wh;
      if (node.max_design_voltage_dV > 0) {
        datalayer.battery.info.max_design_voltage_dV = node.max_design_voltage_dV;
      }
      if (node.min_design_voltage_dV > 0) {
        datalayer.battery.info.min_design_voltage_dV = node.min_design_voltage_dV;
      }
      datalayer.battery.status.bms_status = ACTIVE;
      datalayer.battery.status.real_bms_status = BMS_ACTIVE;
      datalayer.system.status.battery_allows_contactor_closing = true;
      break;
    }
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.reported_current_dA = 0;
    datalayer.battery.status.current_dA = 0;
    datalayer.battery.status.active_power_W = 0;
    return;
  }

  // Push aggregated values into datalayer.battery (what the inverter reads)
  datalayer.battery.info.total_capacity_Wh = total_capacity_Wh;
  datalayer.battery.info.reported_total_capacity_Wh = total_capacity_Wh;

  if (lowest_max_design_voltage_dV < 65535) {
    datalayer.battery.info.max_design_voltage_dV = lowest_max_design_voltage_dV;
  }
  if (highest_min_design_voltage_dV > 0) {
    datalayer.battery.info.min_design_voltage_dV = highest_min_design_voltage_dV;
  }

  datalayer.battery.status.remaining_capacity_Wh = total_remaining_Wh;
  datalayer.battery.status.reported_remaining_capacity_Wh = total_remaining_Wh;

  // Apply master pre-join power cap (both charge/discharge simultaneously).
  // The cap is driven by the strongest active pre-join pressure among blocked slaves.
  // One-way ratchet: pressure only increases while any slave is in prejoin.
  // Power is never released back until all prejoin sessions complete (slave connected or aborted).
  uint16_t target_pressure_permille = 0;
  bool any_prejoin_active = false;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    if (prejoin_pressure_permille[i] > target_pressure_permille) {
      target_pressure_permille = prejoin_pressure_permille[i];
    }
    if (prejoin_active[i]) {
      any_prejoin_active = true;
      datalayer.system.slave_nodes[i].prejoin_active = true;
    }
  }

  if (target_pressure_permille == 0 && !any_prejoin_active) {
    // All prejoin sessions done — release immediately so cap returns to full without delay
    prejoin_applied_pressure_permille = 0;
  } else if (prejoin_applied_pressure_permille < target_pressure_permille) {
    // Ramp up slowly toward target
    uint16_t delta = (uint16_t)(target_pressure_permille - prejoin_applied_pressure_permille);
    if (delta > PREJOIN_PRESSURE_STEP_PER_S) {
      delta = PREJOIN_PRESSURE_STEP_PER_S;
    }
    prejoin_applied_pressure_permille = (uint16_t)(prejoin_applied_pressure_permille + delta);
  } else if (prejoin_applied_pressure_permille > target_pressure_permille) {
    // Ramp down toward non-zero target (another slave still in prejoin at lower pressure)
    uint16_t delta = (uint16_t)(prejoin_applied_pressure_permille - target_pressure_permille);
    if (delta > PREJOIN_PRESSURE_STEP_PER_S) {
      delta = PREJOIN_PRESSURE_STEP_PER_S;
    }
    prejoin_applied_pressure_permille = (uint16_t)(prejoin_applied_pressure_permille - delta);
  }

  // Outside prejoin: use full slave aggregation as cap base (normal operation).
  uint32_t cap_base_charge_W = total_max_charge_W;
  uint32_t cap_base_discharge_W = total_max_discharge_W;

  // Snapshot on rising edge only: clip to user's configured inverter limit so the
  // pressure curve spans a meaningful range (e.g. 30 A × 370 V = 11 100 W).
  // max_user_set_*_dA is a static setting — no feedback loop.
  // Before and after prejoin, full slave aggregation is used unchanged.
  if (any_prejoin_active && !prejoin_was_active) {
    prejoin_cap_snapshot_charge_W = cap_base_charge_W;
    prejoin_cap_snapshot_discharge_W = cap_base_discharge_W;
    if (shared_voltage_dV > 0) {
      uint32_t user_charge_W =
          (uint32_t)datalayer.battery.settings.max_user_set_charge_dA * shared_voltage_dV / 100u;
      uint32_t user_discharge_W =
          (uint32_t)datalayer.battery.settings.max_user_set_discharge_dA * shared_voltage_dV / 100u;
      if (user_charge_W > 0 && user_charge_W < prejoin_cap_snapshot_charge_W) {
        prejoin_cap_snapshot_charge_W = user_charge_W;
      }
      if (user_discharge_W > 0 && user_discharge_W < prejoin_cap_snapshot_discharge_W) {
        prejoin_cap_snapshot_discharge_W = user_discharge_W;
      }
    }
    logging.printf("Master CAN: Pre-join cap snapshot: charge=%uW discharge=%uW\n",
                   prejoin_cap_snapshot_charge_W, prejoin_cap_snapshot_discharge_W);
  } else if (!any_prejoin_active && prejoin_applied_pressure_permille == 0) {
    prejoin_cap_snapshot_charge_W = 0;
    prejoin_cap_snapshot_discharge_W = 0;
  }
  prejoin_was_active = any_prejoin_active;

  uint32_t base_charge_W =
      (prejoin_cap_snapshot_charge_W > 0) ? prejoin_cap_snapshot_charge_W : cap_base_charge_W;
  uint32_t base_discharge_W =
      (prejoin_cap_snapshot_discharge_W > 0) ? prejoin_cap_snapshot_discharge_W : cap_base_discharge_W;
  // Fixed floor based only on joined battery count (inverter-independent).
  uint32_t fixed_floor_W = prejoin_floor_w_for_joined(active_count);
  uint32_t floor_charge_W = fixed_floor_W;
  uint32_t floor_discharge_W = fixed_floor_W;

  // Start from snapshot (base_*) and apply pressure formula.
  // Snapshot is frozen at prejoin start to break the feedback loop where capping the inverter
  // causes the battery to report lower limits, which would otherwise further tighten the cap.
  // After pressure reaches 0 and the snapshot clears, base_* equals the live cap naturally.
  uint32_t capped_max_charge_W = base_charge_W;
  uint32_t capped_max_discharge_W = base_discharge_W;
  if (prejoin_applied_pressure_permille > 0) {
    if (base_charge_W > floor_charge_W) {
      uint32_t span = base_charge_W - floor_charge_W;
      uint32_t pressure_cap =
          floor_charge_W + (span * (1000u - prejoin_applied_pressure_permille)) / 1000u;
      if (pressure_cap < capped_max_charge_W) {
        capped_max_charge_W = pressure_cap;
      }
    }
    if (base_discharge_W > floor_discharge_W) {
      uint32_t span = base_discharge_W - floor_discharge_W;
      uint32_t pressure_cap =
          floor_discharge_W + (span * (1000u - prejoin_applied_pressure_permille)) / 1000u;
      if (pressure_cap < capped_max_discharge_W) {
        capped_max_discharge_W = pressure_cap;
      }
    }
  }

  // Hard floor for the full prejoin flow.
  // While prejoin is active for any slave (or pressure is still releasing),
  // caps are never allowed below the defined joined-battery floor.
  if (any_prejoin_active || prejoin_applied_pressure_permille > 0) {
    if (capped_max_charge_W < floor_charge_W) {
      capped_max_charge_W = floor_charge_W;
    }
    if (capped_max_discharge_W < floor_discharge_W) {
      capped_max_discharge_W = floor_discharge_W;
    }
  }

  // Log every second while any prejoin is active or pressure is still being released
  if (any_prejoin_active || prejoin_applied_pressure_permille > 0) {
    int32_t act_W = datalayer.battery.status.active_power_W;
    logging.printf(
        "Master CAN: Pre-join applied=%u permille  cap_charge=%uW cap_discharge=%uW  actual=%dW  "
        "base_charge=%uW base_discharge=%uW\n",
        prejoin_applied_pressure_permille, capped_max_charge_W, capped_max_discharge_W, (int)act_W,
        base_charge_W, base_discharge_W);
  }

  datalayer.battery.status.max_charge_power_W =
      (charge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : capped_max_charge_W;
  datalayer.battery.status.max_discharge_power_W =
      (discharge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : capped_max_discharge_W;
  datalayer.battery.status.reported_current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.voltage_dV = shared_voltage_dV;
  // Power = V × I: voltage in dV, current in dA → W = (dV × dA) / 100
  datalayer.battery.status.active_power_W = (int32_t)shared_voltage_dV * (int32_t)total_current_dA / 100;

  // SOC selection: report lowest_soc normally (discharge protection).
  // When the fullest slave enters the top 5% (>=95%), blend smoothly toward
  // highest_soc so the inverter sees a gradual rise to 100% rather than a jump.
  // blend_factor goes 0..500 as highest_soc goes 9500..10000.
  uint16_t reported_real_soc;
  if (highest_soc >= 9500) {
    uint32_t blend = (uint32_t)(highest_soc - 9500);  // 0..500
    uint32_t diff = (highest_soc > lowest_soc) ? (uint32_t)(highest_soc - lowest_soc) : 0u;
    reported_real_soc = (uint16_t)(lowest_soc + diff * blend / 500u);
  } else {
    reported_real_soc = lowest_soc;
  }
  datalayer.battery.status.real_soc = reported_real_soc;
  datalayer.battery.status.temperature_max_dC = highest_temp;
  datalayer.battery.status.temperature_min_dC = lowest_temp;
  datalayer.battery.status.soh_pptt = lowest_soh_pptt;
  if (max_cell_voltage_mV > 0) {
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage_mV;
  }
  if (min_cell_voltage_mV < 65535) {
    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage_mV;
  }
  // Keep the CAN alive counter refreshed so safety.cpp treats master like a live battery
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  // Keep BMS status active so inverter sees system as alive
  datalayer.battery.status.bms_status = ACTIVE;
  datalayer.battery.status.real_bms_status = BMS_ACTIVE;

  // Signal inverter that system allows contactor
  datalayer.system.status.battery_allows_contactor_closing = true;
}
