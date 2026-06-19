#include "CONTROLLER-CAN.h"

#include <Arduino.h>

#include "../../battery/BATTERIES.h"
#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

ControllerCan controller_can;

// Voltage threshold for contactor safety (same as check_interconnect_available)
static const uint16_t VOLTAGE_DIFF_THRESHOLD_dV = 15;      // 1.5V
static const uint8_t VOLTAGE_DIFF_SECONDS_LIMIT = 10;      // 10s grace period
static const uint8_t CONTROLLER_CONTACTOR_STAGGER_MS = 3;  // controller-only inter-frame spacing

// Pre-join controller constants (controller-only):
static const uint16_t PREJOIN_ENTER_DIFF_dV = 18u;     // 1.8V — activate when diff <= this
static const uint16_t PREJOIN_CLOSE_RAW_DIFF_dV = 5u;  // 0.5V — close gate (charging); direction window (discharging)
static const uint8_t PREJOIN_CLOSE_DWELL_S = 2u;       // seconds diff must stay in close window before ALLOW
// Abort an active prejoin if load stays below this for too long (inverter goes idle mid-prejoin)
static const uint16_t PREJOIN_LOW_POWER_ABORT_W = 300u;
static const uint8_t PREJOIN_LOW_POWER_ABORT_S = 30u;

// Per-node voltage diff grace period counters
static uint8_t voltage_diff_seconds[MAX_BATTERY_NODES] = {0};

// How long to keep the contactor allowed after offline balancing starts.
// Keep it allowed for 50s, then force block while balancing is active.
static const uint8_t BALANCING_HOLD_SECONDS = 50u;
// Per-node countdown: while > 0, contactor_allowed is not yet forced false
static uint8_t balancing_hold_seconds[MAX_BATTERY_NODES] = {0};
// Per-node last transmitted contactor command for change logging
static uint8_t last_contactor_command[MAX_BATTERY_NODES] = {0};
// Per-node log-once flags — reset when the condition clears
static bool stale_logged[MAX_BATTERY_NODES] = {false};
static bool ident_mismatch_logged[MAX_BATTERY_NODES] = {false};

// Per-node pre-join tracking
static bool prejoin_active[MAX_BATTERY_NODES] = {false};
static uint8_t prejoin_raw_stable_seconds[MAX_BATTERY_NODES] = {0};
static uint8_t prejoin_low_power_seconds[MAX_BATTERY_NODES] = {0};
static uint8_t prejoin_postclose_log_seconds[MAX_BATTERY_NODES] = {0};

static void reset_prejoin_state(uint8_t idx) {
  prejoin_active[idx] = false;
  datalayer.system.battery_nodes[idx].prejoin_active = false;
  prejoin_raw_stable_seconds[idx] = 0;
  prejoin_low_power_seconds[idx] = 0;
}

void InterUnitControllerBattery::setup() {
  datalayer.system.status.battery_allows_contactor_closing = false;
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  controller_can.begin();
  register_can_receiver(&controller_can, can_config.battery, CAN_Speed::CAN_SPEED_500KBPS);
  register_transmitter(&controller_can);
  logging.println("Controller CAN: registered on battery bus @ 500kbps");
}

void InterUnitControllerBattery::update_values() {
  controller_can.update_values();
  // Keep the controller "alive" only while at least one node is usable (online, identity OK, no
  // fault, not stale — balancing nodes count as usable). If every node is offline OR faulted,
  // let CAN_battery_still_alive run down so safety.cpp raises EVENT_CAN_BATTERY_MISSING — the
  // single, intended path for propagating a system-wide FAULT to the inverter.
  if (controller_can.any_node_usable()) {
    datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  }
}

void ControllerCan::begin() {
  _last_heartbeat_ms = 0;
  _next_contactor_tx_ms = 0;
  _next_contactor_idx = 0;
  _contactor_burst_pending = false;
  _startup_begin_ms = 0;
  _startup_grace_done = false;
  _estop_was_active = false;
  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    voltage_diff_seconds[i] = 0;
    balancing_hold_seconds[i] = 0;
    last_contactor_command[i] = 0xFFu;
    stale_logged[i] = false;
    ident_mismatch_logged[i] = false;
    reset_prejoin_state(i);
    // Force all contactors blocked on (re)start — RAM may retain state from before
    datalayer.system.battery_nodes[i].contactor_allowed = false;
    datalayer.system.battery_nodes[i].online = false;
    datalayer.system.battery_nodes[i].still_alive = 0;
  }
}

// ---- CAN RX --------------------------------------------------------

void ControllerCan::receive_can_frame(CAN_frame* rx_frame) {
  const uint32_t id = rx_frame->ID;

  // Only process node messages in the expected range
  if (id < IU_NODE_MSG_MIN_ID || id > IU_NODE_MSG_MAX_ID) {
    return;
  }

  // Decode node_id and sub-message from ID
  // IU_NODE_STATUS_ID(n) = 0x100 + n*0x10 + 0x00
  // IU_NODE_POWER_ID(n)  = 0x100 + n*0x10 + 0x01
  // IU_NODE_INFO_ID(n)   = 0x100 + n*0x10 + 0x02
  uint32_t offset = id - 0x100u;
  uint8_t node_id = (uint8_t)(offset >> 4);  // high nibble = node ID (1..24)
  uint8_t sub = (uint8_t)(offset & 0x0Fu);   // low nibble = sub-message (0..5)

  if (node_id < 1 || node_id > MAX_BATTERY_NODES) {
    return;
  }

  BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[node_id - 1];

  // Reset still_alive counter (decremented at 1Hz in update_values() => 60s timeout)
  node.still_alive = CAN_STILL_ALIVE;
  node.online = true;
  // Note: EVENT_BATTERY_NODE_MISSING is intentionally NOT cleared here so the
  // event history shows that a node was offline. Contactor is re-allowed by
  // check_node_voltage_safety() once voltage is within ±1.5V.

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
      // [7] node_pflags: check offline balancing flag
      bool was_balancing = node.balancing;
      node.balancing = (rx_frame->data.u8[7] & IU_NODE_PFLAG_BALANCING) != 0;
      if (!was_balancing && node.balancing) {
        // Balancing just started: start hold timer — BMW I3 opens its own contactor ~20s later.
        // Controller will block contactor_allowed after BALANCING_HOLD_SECONDS (50s).
        balancing_hold_seconds[node_id - 1] = BALANCING_HOLD_SECONDS;
        logging.printf("Controller CAN: Node %d offline balancing started — contactor held for %us\n", node_id,
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
      // Validate before accepting: a genuine IDENT is exactly 8 bytes with reserved [4..7] == 0.
      // Rejecting malformed frames stops a single garbled IDENT (CAN glitch, or a mis-framed
      // non-IDENT frame landing on a 0x_5 ID) from overwriting a known-good fw_version_num and
      // raising a false EVENT_BATTERY_NODE_IDENT_MISMATCH warning.
      if (rx_frame->DLC == 8 &&
          (rx_frame->data.u8[4] | rx_frame->data.u8[5] | rx_frame->data.u8[6] | rx_frame->data.u8[7]) == 0) {
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

void ControllerCan::transmit(unsigned long currentMillis) {
  if (currentMillis - _last_heartbeat_ms >= INTERVAL_1_S) {
    _last_heartbeat_ms = currentMillis;
    send_heartbeat();

    // Start a fresh burst of per-node contactor commands after each heartbeat.
    // Commands are staggered over a few milliseconds to avoid native CAN TX bursts.
    _next_contactor_idx = 0;
    _contactor_burst_pending = true;
    _next_contactor_tx_ms = currentMillis + 1u;
  }

  send_contactor_commands(currentMillis);
}

void ControllerCan::send_heartbeat() {
  CAN_frame frame = {};
  frame.ID = IU_CONTROLLER_HEARTBEAT_ID;
  frame.DLC = 0;
  frame.ext_ID = false;
  transmit_can_frame_to_interface(&frame, can_config.battery);
}

void ControllerCan::send_contactor_commands(unsigned long currentMillis) {
  if (!_contactor_burst_pending) {
    return;
  }

  if ((long)(currentMillis - _next_contactor_tx_ms) < 0) {
    return;
  }

  // Send one online node command per slot, then wait for the next stagger step.
  while (_next_contactor_idx < MAX_BATTERY_NODES) {
    uint8_t i = _next_contactor_idx++;
    BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
    if (!node.online) {
      continue;
    }

    uint8_t node_id = i + 1;
    CAN_frame frame = {};
    frame.ID = IU_CONTROLLER_CONTACTOR_ID(node_id);
    frame.DLC = 1;
    frame.ext_ID = false;
    bool allow_command = node.contactor_allowed && datalayer.system.status.inverter_allows_contactor_closing &&
                         !datalayer.system.info.equipment_stop_active;
    frame.data.u8[0] = allow_command ? IU_CONTACTOR_ALLOW : IU_CONTACTOR_OPEN;

    if (last_contactor_command[i] != frame.data.u8[0]) {
      last_contactor_command[i] = frame.data.u8[0];
      logging.printf(
          "Controller CAN: TX contactor cmd node %u -> %s (node_allowed=%u inverter_allow=%u estop=%u pack_allow=%u "
          "online=%u)\n",
          node_id, allow_command ? "ALLOW" : "OPEN", node.contactor_allowed ? 1 : 0,
          datalayer.system.status.inverter_allows_contactor_closing ? 1 : 0,
          datalayer.system.info.equipment_stop_active ? 1 : 0,
          datalayer.system.status.battery_allows_contactor_closing ? 1 : 0, node.online ? 1 : 0);
    }

    transmit_can_frame_to_interface(&frame, can_config.battery);
    _next_contactor_tx_ms = currentMillis + CONTROLLER_CONTACTOR_STAGGER_MS;
    return;
  }

  _contactor_burst_pending = false;
}

// ---- Value aggregation (called every 1s) ---------------------------

void ControllerCan::update_values() {
  bool estop_active = datalayer.system.info.equipment_stop_active;

  // Start grace timer the moment the first node comes online
  if (_startup_begin_ms == 0) {
    for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
      if (datalayer.system.battery_nodes[i].online) {
        _startup_begin_ms = millis();
        logging.printf("Controller CAN: First node online — grace period started (%u s)\n", IU_STARTUP_GRACE_S);
        break;
      }
    }
  }

  // Grace period: when it expires, move from startup hold to per-node voltage control.
  // This only marks startup as complete; actual contactor permission is still
  // decided per node by voltage safety.
  if (!_startup_grace_done && _startup_begin_ms != 0) {
    unsigned long elapsed_s = (millis() - _startup_begin_ms) / 1000UL;
    if (elapsed_s >= IU_STARTUP_GRACE_S) {
      _startup_grace_done = true;
      uint16_t reference_voltage_dV = 0;
      bool reference_found = false;
      bool all_online_nodes_match = true;
      uint8_t reference_node_id = 0;

      for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
        const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
        voltage_diff_seconds[i] = 0;

        if (!node.online || node.balancing || node.voltage_dV == 0) {
          continue;
        }

        if (!reference_found) {
          reference_voltage_dV = node.voltage_dV;
          reference_node_id = i + 1;
          reference_found = true;
          continue;
        }

        uint16_t diff = (node.voltage_dV > reference_voltage_dV) ? (node.voltage_dV - reference_voltage_dV)
                                                                 : (reference_voltage_dV - node.voltage_dV);
        if (diff > VOLTAGE_DIFF_THRESHOLD_dV) {
          all_online_nodes_match = false;
        }
      }

      if (reference_found) {
        logging.printf("Controller CAN: Grace reference node %u at %u.%uV\n", reference_node_id,
                       reference_voltage_dV / 10, reference_voltage_dV % 10);
      }

      if (reference_found && all_online_nodes_match) {
        for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
          BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
          if (node.online && !node.balancing && node.voltage_dV > 0 && node_identity_ok(i)) {
            node.contactor_allowed = true;
            logging.printf("Controller CAN: Grace done — Node %d contactor ALLOWED together (voltage %u.%uV)\n", i + 1,
                           node.voltage_dV / 10, node.voltage_dV % 10);
          }
        }
        logging.println("Controller CAN: Startup grace done — matched online nodes may close together");
      } else {
        logging.println("Controller CAN: Startup grace done — voltage safety now controls contactor permission");
      }
    }
  }

  if (estop_active && !_estop_was_active) {
    logging.println("Controller CAN: E-stop ACTIVE — resetting node contactor permissions and voltage qualification");
  } else if (!estop_active && _estop_was_active) {
    logging.println("Controller CAN: E-stop CLEARED — resuming contactor logic immediately");
  }
  _estop_was_active = estop_active;

  // Decrement still_alive counters
  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
    if (!node.online) {
      reset_prejoin_state(i);
      continue;
    }

    if (estop_active) {
      voltage_diff_seconds[i] = 0;
      reset_prejoin_state(i);
      if (node.contactor_allowed) {
        node.contactor_allowed = false;
        logging.printf("Controller CAN: Node %d contactor BLOCKED by E-stop\n", i + 1);
      }
    }

    if (node.still_alive > 0) {
      node.still_alive--;
    }
    if (node.still_alive == 0) {
      node.online = false;
      node.contactor_allowed = false;
      reset_prejoin_state(i);
      set_event(EVENT_BATTERY_NODE_MISSING, i + 1);  // data = node ID (1-24)
      logging.printf("Controller CAN: Node %d went OFFLINE\n", i + 1);
    }

    // Offline balancing hold timer: once expired, block the contactor
    if (node.balancing) {
      reset_prejoin_state(i);
      if (balancing_hold_seconds[i] > 0) {
        balancing_hold_seconds[i]--;
      } else if (node.contactor_allowed) {
        node.contactor_allowed = false;
        logging.printf("Controller CAN: Node %d balancing hold expired — contactor BLOCKED\n", i + 1);
      }
    }

    // Check voltage safety first (may allow contactor based on voltage match)
    // Skip entirely while e-stop is active — contactor must stay blocked
    if (!estop_active) {
      check_node_voltage_safety(i);
    }

    // Stale data detection: if STATUS toggle bit has not changed for IU_STATUS_STALE_SECONDS,
    // this node's data is not refreshing — block its contactor and flag it. This is a
    // per-node (controller-internal) condition raised as a WARNING, not a global ERROR/FAULT:
    // one stale node must not fault the whole system. A genuine system-wide fault is raised
    // only when no node is usable (see InterUnitControllerBattery::update_values / any_node_usable).
    // Runs AFTER voltage safety so it always overrides any re-allow.
    if (node.online && node._last_status_toggle != 0xFF) {
      node.status_stale_seconds++;
      if (node.status_stale_seconds > IU_STATUS_STALE_SECONDS) {
        node.contactor_allowed = false;
        set_event(EVENT_BATTERY_NODE_STATUS_STALE, i + 1);
        if (!stale_logged[i]) {
          stale_logged[i] = true;
          logging.printf("Controller CAN: Node %d STATUS is STALE (%u s)\n", i + 1, node.status_stale_seconds);
        }
      } else {
        stale_logged[i] = false;
        clear_event(EVENT_BATTERY_NODE_STATUS_STALE);
      }
    }

    // IDENT mismatch: firmware version or battery type mismatch blocks a not-yet-closed
    // contactor from closing (entry gate in check_node_voltage_safety), but must NEVER
    // force-open an already-closed one. A momentary IDENT glitch (one corrupted fw/btype
    // frame) must not drop a live pack — mirror the warning philosophy: block new closes,
    // never open. Only check after IDENT has been received from this node.
    if (node.online && node.ident_received) {
      if (!node_identity_ok(i)) {
        set_event(EVENT_BATTERY_NODE_IDENT_MISMATCH, i + 1);
        if (!ident_mismatch_logged[i]) {
          ident_mismatch_logged[i] = true;
          // Reference battery type = first node that reported IDENT.
          uint16_t ref_btype = 0;
          for (uint8_t j = 0; j < MAX_BATTERY_NODES; j++) {
            if (datalayer.system.battery_nodes[j].ident_received) {
              ref_btype = datalayer.system.battery_nodes[j].battery_type_id;
              break;
            }
          }
          logging.printf("Controller CAN: Node %d IDENT mismatch (fw=0x%04X exp=0x%04X btype=%u exp=%u)\n", i + 1,
                         node.fw_version_num, iu_fw_version_num(), node.battery_type_id, ref_btype);
        }
      } else {
        ident_mismatch_logged[i] = false;
        clear_event(EVENT_BATTERY_NODE_IDENT_MISMATCH);
      }
    }

    // Fault flag events: ERROR mask blocks contactor (red), WARNING mask is advisory (yellow).
    // Runs AFTER voltage safety so error faults always override any re-allow.
    if (node.online) {
      if (node.fault_flags & IU_FAULT_ERROR_MASK) {
        node.contactor_allowed = false;
      }
      // Node warnings are shown per-node in the web UI — no controller event needed
    }
  }

  // Set or clear EVENT_BATTERY_NODE_FAULT once after the full loop to avoid clear/set ping-pong
  // (clearing for node N then immediately re-setting for node N+1 would re-trigger the log).
  {
    uint8_t first_fault_node = 0;
    for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
      const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
      if (node.online && (node.fault_flags & IU_FAULT_ERROR_MASK)) {
        first_fault_node = i + 1;
        break;
      }
    }
    if (first_fault_node > 0) {
      set_event(EVENT_BATTERY_NODE_FAULT, first_fault_node);
    } else {
      clear_event(EVENT_BATTERY_NODE_FAULT);
    }
  }

  // Aggregate all valid online nodes into datalayer.battery
  update_node_aggregation();
}

bool ControllerCan::node_identity_ok(uint8_t idx) const {
  const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[idx];
  // No IDENT received yet -> identity unverified -> treat as blocked.
  // We must never close a contactor to a node we have not verified.
  if (!node.ident_received) {
    return false;
  }
  if (node.fw_version_num != iu_fw_version_num()) {
    return false;
  }
  // Battery type must match the first node that reported one (reference).
  for (uint8_t j = 0; j < MAX_BATTERY_NODES; j++) {
    if (datalayer.system.battery_nodes[j].ident_received) {
      return node.battery_type_id == datalayer.system.battery_nodes[j].battery_type_id;
    }
  }
  return true;
}

bool ControllerCan::any_node_usable() const {
  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
    // Balancing nodes count as usable: they are temporarily idle, not failed, so a node
    // that is balancing must not let the whole system fault out.
    // A yellow warning (IU_FAULT_WARNING_MASK) does not make a node unusable — only ERROR-level
    // faults do. Otherwise an advisory warning on every node would let the controller go quiet
    // and indirectly fault the whole system (EVENT_CAN_BATTERY_MISSING), opening all contactors.
    if (node.online && node_identity_ok(i) && (node.fault_flags & IU_FAULT_ERROR_MASK) == 0 &&
        node.status_stale_seconds <= IU_STATUS_STALE_SECONDS) {
      return true;
    }
  }
  return false;
}

/** Mimics check_interconnect_available() logic from parallel_safety.cpp */
void ControllerCan::check_node_voltage_safety(uint8_t idx) {
  BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[idx];
  if (!node.online || node.voltage_dV == 0) {
    reset_prejoin_state(idx);
    return;
  }

  // Offline balancing nodes are handled by balancing_hold_seconds logic.
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

  // During startup grace period, keep all contactors blocked so all nodes
  // can announce themselves at matching voltages before the inverter starts.
  if (!_startup_grace_done) {
    reset_prejoin_state(idx);
    return;
  }

  // Identity gate: a node whose FW/battery type does not match (or which has
  // not yet reported IDENT) must never prejoin or be allowed to close. This is
  // an entry gate — not just a post-override — so a blocked node can never even
  // enter the prejoin state machine (the IDENT-mismatch event is still raised in
  // update_values()).
  if (!node_identity_ok(idx)) {
    // Entry gate only: block a not-yet-closed node from prejoining/closing, but NEVER
    // force-open an already-closed contactor. A momentary IDENT glitch (one corrupted
    // fw/btype frame) must not drop a live pack — mirror the warning philosophy.
    if (!node.contactor_allowed) {
      voltage_diff_seconds[idx] = 0;
      reset_prejoin_state(idx);
    }
    return;
  }

  // First node to come online sets the reference voltage.
  // Additional nodes must stay within the voltage threshold for
  // VOLTAGE_DIFF_SECONDS_LIMIT consecutive update cycles before closing.
  uint16_t reference_voltage_dV = 0;
  for (uint8_t j = 0; j < MAX_BATTERY_NODES; j++) {
    if (j == idx)
      continue;
    if (datalayer.system.battery_nodes[j].online && datalayer.system.battery_nodes[j].voltage_dV > 0 &&
        datalayer.system.battery_nodes[j].contactor_allowed) {
      reference_voltage_dV = datalayer.system.battery_nodes[j].voltage_dV;
      break;
    }
  }

  if (reference_voltage_dV == 0) {
    // No reference available yet — allow this node to connect
    if (!node.contactor_allowed) {
      node.contactor_allowed = true;
      voltage_diff_seconds[idx] = 0;
      reset_prejoin_state(idx);
      logging.printf("Controller CAN: Node %d contactor ALLOWED (first node)\n", idx + 1);
    }
    return;
  }

  uint16_t diff = (node.voltage_dV > reference_voltage_dV) ? (node.voltage_dV - reference_voltage_dV)
                                                           : (reference_voltage_dV - node.voltage_dV);

  // Only ERROR-level faults open the contactor. Yellow warnings (cell over/under-voltage,
  // over-temperature) are advisory — a standalone BE keeps system_status ACTIVE on a WARNING
  // and lets the node's charge/discharge power limits derate instead; if the condition were
  // severe enough to open, the node would already have raised an ERROR and opened its own
  // contactor. Mirror that here so the controller never opens on a mere warning.
  bool has_fault = (node.fault_flags & IU_FAULT_ERROR_MASK) != 0;

  if (!node.contactor_allowed && !has_fault) {
    int32_t load_W = datalayer.battery.status.active_power_W;
    uint32_t abs_load_W = (load_W >= 0) ? (uint32_t)load_W : (uint32_t)(-load_W);
    bool inverter_working = (abs_load_W > PREJOIN_LOW_POWER_ABORT_W);

    if (!prejoin_active[idx] && inverter_working && diff <= PREJOIN_ENTER_DIFF_dV) {
      prejoin_active[idx] = true;
      prejoin_raw_stable_seconds[idx] = 0;
      logging.printf("Controller CAN: Pre-join START node %d (raw=%u.%uV load=%dW)\n", idx + 1, diff / 10, diff % 10,
                     (int)load_W);
    }

    if (prejoin_active[idx]) {
      if (abs_load_W < PREJOIN_LOW_POWER_ABORT_W) {
        if (prejoin_low_power_seconds[idx] < PREJOIN_LOW_POWER_ABORT_S) {
          prejoin_low_power_seconds[idx]++;
        }
        if (prejoin_low_power_seconds[idx] >= PREJOIN_LOW_POWER_ABORT_S) {
          logging.printf("Controller CAN: Pre-join ABORT node %d — load %dW below %uW for %us\n", idx + 1, (int)load_W,
                         (unsigned)PREJOIN_LOW_POWER_ABORT_W, (unsigned)PREJOIN_LOW_POWER_ABORT_S);
          reset_prejoin_state(idx);
        }
      } else {
        prejoin_low_power_seconds[idx] = 0;
      }

      int16_t signed_diff_dV = (int16_t)node.voltage_dV - (int16_t)reference_voltage_dV;
      bool close_window = (load_W < 0) ? (signed_diff_dV >= 0 && diff <= 7u) : (diff <= PREJOIN_CLOSE_RAW_DIFF_dV);
      if (close_window) {
        if (prejoin_raw_stable_seconds[idx] < PREJOIN_CLOSE_DWELL_S) {
          prejoin_raw_stable_seconds[idx]++;
        }
      } else {
        prejoin_raw_stable_seconds[idx] = 0;
      }

      logging.printf("Controller CAN: Pre-join node %d: raw=%u.%uV stable=%us\n", idx + 1, diff / 10, diff % 10,
                     prejoin_raw_stable_seconds[idx]);
    } else {
      prejoin_raw_stable_seconds[idx] = 0;
    }
  } else if (node.contactor_allowed) {
    reset_prejoin_state(idx);
    if (prejoin_postclose_log_seconds[idx] > 0) {
      prejoin_postclose_log_seconds[idx]--;
      logging.printf("Controller CAN: Post-close node %d: voltage=%u.%uV current=%d.%uA diff=%u.%uV\n", idx + 1,
                     node.voltage_dV / 10, node.voltage_dV % 10, (int)node.current_dA / 10,
                     (unsigned)(abs((int)node.current_dA) % 10), diff / 10, diff % 10);
    }
  }

  if (has_fault) {
    voltage_diff_seconds[idx] = 0;
    reset_prejoin_state(idx);
    if (node.contactor_allowed) {
      node.contactor_allowed = false;
      logging.printf("Controller CAN: Node %d contactor OPENED (fault flags: 0x%02X)\n", idx + 1, node.fault_flags);
    }
  } else if (!node.contactor_allowed) {
    if (diff <= VOLTAGE_DIFF_THRESHOLD_dV) {
      if (voltage_diff_seconds[idx] < VOLTAGE_DIFF_SECONDS_LIMIT) {
        voltage_diff_seconds[idx]++;
      }
      if (voltage_diff_seconds[idx] >= VOLTAGE_DIFF_SECONDS_LIMIT) {
        bool prejoin_gate_ok = true;
        if (prejoin_active[idx]) {
          int32_t load_W = datalayer.battery.status.active_power_W;
          int16_t signed_diff_dV = (int16_t)node.voltage_dV - (int16_t)reference_voltage_dV;
          bool direction_ok;
          if (load_W < 0) {
            direction_ok = (signed_diff_dV >= 0 && diff <= 7u);
          } else {
            direction_ok = (diff <= PREJOIN_CLOSE_RAW_DIFF_dV);
          }
          prejoin_gate_ok = direction_ok && (prejoin_raw_stable_seconds[idx] >= PREJOIN_CLOSE_DWELL_S);
        }
        if (prejoin_gate_ok) {
          node.contactor_allowed = true;
          voltage_diff_seconds[idx] = 0;
          reset_prejoin_state(idx);
          prejoin_postclose_log_seconds[idx] = 20u;
          logging.printf("Controller CAN: Node %d contactor ALLOWED (voltage OK for %us: %u.%uV)\n", idx + 1,
                         VOLTAGE_DIFF_SECONDS_LIMIT, node.voltage_dV / 10, node.voltage_dV % 10);
        }
      }
    } else {
      if (voltage_diff_seconds[idx] != 0) {
        logging.printf("Controller CAN: Node %d contactor BLOCKED (voltage diff %u.%uV > %u.%uV)\n", idx + 1, diff / 10,
                       diff % 10, VOLTAGE_DIFF_THRESHOLD_dV / 10, VOLTAGE_DIFF_THRESHOLD_dV % 10);
      }
      voltage_diff_seconds[idx] = 0;
    }
  } else {
    voltage_diff_seconds[idx] = 0;
  }
  // If the contactor is already allowed and there are no faults, we permit voltage differences without opening the contactor.
}

void ControllerCan::update_node_aggregation() {
  // Aggregate online nodes into datalayer.battery
  uint32_t total_capacity_Wh = 0;
  uint32_t total_remaining_Wh = 0;
  uint32_t total_max_charge_W = 0;
  uint32_t total_max_discharge_W = 0;
  int32_t total_current_dA = 0;
  uint16_t lowest_soc = 10001;  // Above max to detect "no data"
  uint16_t highest_soc = 0;
  int16_t highest_temp = -1270;
  int16_t lowest_temp = 1270;
  uint16_t shared_voltage_dV = 0;                 // All nodes share voltage (parallel)
  uint16_t lowest_max_design_voltage_dV = 65535;  // To safely limit inverter charge voltage
  uint16_t highest_min_design_voltage_dV = 0;     // To safely limit inverter discharge voltage
  uint16_t lowest_soh_pptt = 9900;                // Use lowest SOH across all nodes
  uint16_t max_cell_voltage_mV = 0;
  uint16_t min_cell_voltage_mV = 65535;
  uint8_t active_count = 0;
  // If any battery's BMS says stop charging/discharging, block all power flow.
  // This ensures we stop as soon as the first battery is full or empty,
  // protecting against overcharge/overdischarge in mixed battery setups.
  bool charge_blocked = false;
  bool discharge_blocked = false;

  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
    // Only aggregate nodes whose contactor is actually CLOSED (engaged). A node that is
    // merely allowed but has not physically closed its contactor is not part of the pack —
    // it reports max_charge_W/max_discharge_W = 0, which would otherwise trip charge_blocked/
    // discharge_blocked and zero the whole pack's power even when other nodes are healthy.
    if (!node.online || !node.contactor_engaged) {
      continue;
    }
    // A node entering offline balancing reports max_charge_W = max_discharge_W = 0 (see
    // BMW-I3-BATTERY.cpp) and keeps reporting its contactor engaged until it actually opens it
    // (~20-30 s later). We deliberately KEEP such a node in the aggregation while its contactor
    // is still engaged so that its reported zero is honoured: it trips charge_blocked/
    // discharge_blocked below, driving the whole-pack limit to 0 and letting the inverter wind
    // current down BEFORE the node opens its contactor — i.e. a no-load open. Once the node
    // reports its contactor open (or goes offline) it is dropped by the !contactor_engaged /
    // !online check above and the remaining healthy nodes resume.
    //
    // NOTE: this means the whole pack briefly stops charge/discharge while a node winds down for
    // balancing. That is intentional and necessary: the packs share one DC bus with passive
    // current sharing, so the only way to guarantee no current through the leaving node when its
    // contactor opens is to stop the inverter entirely. (Previously balancing nodes were excluded
    // here immediately, so their zero was never seen and the contactor could open under load.)
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
    // No contactor-engaged nodes yet (during grace period or startup).
    // Show informational data from the first online node so the inverter can
    // initialise, but hold charge/discharge rates at zero.
    for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
      const BATTERY_NODE_TYPE& node = datalayer.system.battery_nodes[i];
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
      // NOTE: do NOT force system_status here. The event system owns it (default ACTIVE;
      // FAULT only via a system-wide event). Forcing ACTIVE would mask a genuine FAULT.
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

  // Update prejoin_active flag in datalayer for UI display
  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    if (prejoin_active[i]) {
      datalayer.system.battery_nodes[i].prejoin_active = true;
    }
  }

  datalayer.battery.status.max_charge_power_W =
      (charge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : total_max_charge_W;
  datalayer.battery.status.max_discharge_power_W =
      (discharge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : total_max_discharge_W;
  datalayer.battery.status.reported_current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.voltage_dV = shared_voltage_dV;
  // Power = V × I: voltage in dV, current in dA → W = (dV × dA) / 100
  datalayer.battery.status.active_power_W = (int32_t)shared_voltage_dV * (int32_t)total_current_dA / 100;

  // SOC selection: report lowest_soc normally (discharge protection).
  // When the fullest node enters the top 5% (>=95%), blend smoothly toward
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
  // Keep the CAN alive counter refreshed so safety.cpp treats controller like a live battery
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  // Keep the inverter-facing BMS status active. Do NOT touch system_status — the event
  // system owns it, so a real system-wide FAULT is not masked by the controller.
  datalayer.battery.status.real_bms_status = BMS_ACTIVE;

  // Signal inverter that system allows contactor
  datalayer.system.status.battery_allows_contactor_closing = true;
}
