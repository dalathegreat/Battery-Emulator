#include "MASTER-CAN.h"

#include <Arduino.h>

#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

MasterCan master_can;

// Voltage threshold for contactor safety (same as check_interconnect_available)
static const uint16_t VOLTAGE_DIFF_THRESHOLD_dV = 15;  // 1.5V
static const uint8_t VOLTAGE_DIFF_SECONDS_LIMIT = 10;  // 10s grace period

// Per-slave voltage diff grace period counters
static uint8_t voltage_diff_seconds[MAX_SLAVE_NODES] = {0};

// How long to keep the contactor allowed after offline balancing starts.
// BMW I3 opens its own contactor ~20s after balancing begins; we wait 25s to be safe.
static const uint8_t BALANCING_HOLD_SECONDS = 25u;
// Per-slave countdown: while > 0, contactor_allowed is not yet forced false
static uint8_t balancing_hold_seconds[MAX_SLAVE_NODES] = {0};
// Per-slave last transmitted contactor command for change logging
static uint8_t last_contactor_command[MAX_SLAVE_NODES] = {0};

void setup_master_can() {
  master_can.begin();
  register_can_receiver(&master_can, can_config.inter_unit, CAN_Speed::CAN_SPEED_500KBPS);
  register_transmitter(&master_can);
  logging.println("Master CAN: registered on inter-unit bus @ 500kbps");
}

void MasterCan::begin() {
  _last_heartbeat_ms = 0;
  _startup_begin_ms = 0;
  _startup_grace_done = false;
  _estop_was_active = false;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    voltage_diff_seconds[i] = 0;
    balancing_hold_seconds[i] = 0;
    last_contactor_command[i] = 0xFFu;
    // Force all contactors blocked on (re)start — RAM may retain state from before
    datalayer.system.slave_nodes[i].contactor_allowed = false;
    datalayer.system.slave_nodes[i].online = false;
    datalayer.system.slave_nodes[i].still_alive = 0;
  }
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
        // Master will block contactor_allowed after BALANCING_HOLD_SECONDS (25s).
        balancing_hold_seconds[node_id - 1] = BALANCING_HOLD_SECONDS;
        logging.printf("Master CAN: Slave %d offline balancing started — contactor held for %us\n",
                       node_id, BALANCING_HOLD_SECONDS);
      }
      break;
    }
    case 0x02:  // INFO message (every 10s)
    {
      node.total_capacity_Wh = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      node.max_design_voltage_dV = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      node.min_design_voltage_dV = ((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5];
      node.soh_pptt              = ((uint16_t)rx_frame->data.u8[6] << 8) | rx_frame->data.u8[7];
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
        node.fw_version_num  = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
        node.battery_type_id = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
        node.ident_received  = true;
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
    send_contactor_commands();
  }
}

void MasterCan::send_heartbeat() {
  CAN_frame frame = {};
  frame.ID = IU_MASTER_HEARTBEAT_ID;
  frame.DLC = 0;
  frame.ext_ID = false;
  transmit_can_frame_to_interface(&frame, can_config.inter_unit);
}

void MasterCan::send_contactor_commands() {
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (!node.online) {
      continue;  // Don't send commands to slaves we've never heard from
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
          "Master CAN: TX contactor cmd slave %u -> %s (node_allowed=%u inverter_allow=%u estop=%u pack_allow=%u online=%u)\n",
          node_id, allow_command ? "ALLOW" : "OPEN", node.contactor_allowed ? 1 : 0,
          datalayer.system.status.inverter_allows_contactor_closing ? 1 : 0,
          datalayer.system.info.equipment_stop_active ? 1 : 0,
          datalayer.system.status.battery_allows_contactor_closing ? 1 : 0, node.online ? 1 : 0);
    }

    transmit_can_frame_to_interface(&frame, can_config.inter_unit);
  }
}

// ---- Value aggregation (called every 1s) ---------------------------

void MasterCan::update_values() {
  bool estop_active = datalayer.system.info.equipment_stop_active;

  // Lazy-init startup grace timer on first call
  if (_startup_begin_ms == 0) {
    _startup_begin_ms = millis();
    logging.printf("Master CAN: Startup grace period started (%u s) — all contactors held OPEN\n",
                   IU_STARTUP_GRACE_S);
  }

  // Advance grace period
  if (!_startup_grace_done) {
    unsigned long elapsed_s = (millis() - _startup_begin_ms) / 1000UL;
    if (elapsed_s >= IU_STARTUP_GRACE_S) {
      _startup_grace_done = true;
      // Pre-fill counters so slaves already within voltage threshold qualify immediately
      for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
        voltage_diff_seconds[i] = VOLTAGE_DIFF_SECONDS_LIMIT;
      }
      logging.println("Master CAN: Startup grace period done — contactor logic now active");
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
      continue;
    }

    if (estop_active) {
      voltage_diff_seconds[i] = 0;
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
      set_event(EVENT_SLAVE_BATTERY_MISSING, i + 1);  // data = node ID (1-8)
      logging.printf("Master CAN: Slave %d went OFFLINE\n", i + 1);
    }

    // Offline balancing hold timer: once expired, block the contactor
    if (node.balancing) {
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
        logging.printf("Master CAN: Slave %d STATUS is STALE (%u s)\n", i + 1, node.status_stale_seconds);
      } else {
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
        logging.printf("Master CAN: Slave %d IDENT mismatch (fw=0x%04X exp=0x%04X btype=%u exp=%u)\n",
                       i + 1, node.fw_version_num, IU_FW_VERSION_NUM, node.battery_type_id, ref_btype);
      } else {
        clear_event(EVENT_SLAVE_IDENT_MISMATCH);
      }
    }

    // Fault flag events: ERROR mask blocks contactor (red), WARNING mask is advisory (yellow).
    // Runs AFTER voltage safety so error faults always override any re-allow.
    if (node.online) {
      uint8_t node_id_1based = i + 1;
      if (node.fault_flags & IU_FAULT_ERROR_MASK) {
        set_event(EVENT_SLAVE_FAULT, node_id_1based);
        node.contactor_allowed = false;
      } else {
        clear_event(EVENT_SLAVE_FAULT);
      }
      // Slave warnings are shown per-node in the web UI — no master event needed
    }
  }

  // Aggregate all valid online slaves into datalayer.battery
  update_slave_aggregation();
}

/** Mimics check_interconnect_available() logic from parallel_safety.cpp */
void MasterCan::check_slave_voltage_safety(uint8_t idx) {
  SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[idx];
  if (!node.online || node.voltage_dV == 0) {
    return;
  }

  // While e-stop is active, contactors must stay blocked and any pending
  // voltage qualification must restart when the stop is cleared.
  if (datalayer.system.info.equipment_stop_active) {
    voltage_diff_seconds[idx] = 0;
    return;
  }

  // During startup grace period, keep all contactors blocked so all slaves
  // can announce themselves at matching voltages before the inverter starts.
  if (!_startup_grace_done) {
    return;
  }

  // First slave to come online sets the reference voltage.
  // Additional slaves must stay within the voltage threshold for
  // VOLTAGE_DIFF_SECONDS_LIMIT consecutive update cycles before closing.
  uint16_t reference_voltage_dV = 0;
  for (uint8_t j = 0; j < MAX_SLAVE_NODES; j++) {
    if (j == idx) continue;
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
      logging.printf("Master CAN: Slave %d contactor ALLOWED (first slave)\n", idx + 1);
    }
    return;
  }

  uint16_t diff = (node.voltage_dV > reference_voltage_dV) ? (node.voltage_dV - reference_voltage_dV)
                                                            : (reference_voltage_dV - node.voltage_dV);

  bool has_fault = (node.fault_flags != 0);

  if (has_fault) {
    voltage_diff_seconds[idx] = 0;
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
        node.contactor_allowed = true;
        voltage_diff_seconds[idx] = 0;
        logging.printf("Master CAN: Slave %d contactor ALLOWED (voltage OK for %us: %u.%uV)\n", idx + 1,
                       VOLTAGE_DIFF_SECONDS_LIMIT, node.voltage_dV / 10, node.voltage_dV % 10);
      }
    } else {
      voltage_diff_seconds[idx] = 0;
    }
  } else {
    voltage_diff_seconds[idx] = 0;
  }
  // If the contactor is already allowed and there are no faults, we permit voltage differences without opening the contactor.
}

void MasterCan::update_slave_aggregation() {
  // Aggregate online slaves into datalayer.battery.
  //
  // Charge/discharge rate inclusion rule:
  //   - If NO slave has confirmed contactor_engaged yet (startup / first slave):
  //     use contactor_allowed — the first slave connects and the inverter needs to
  //     see valid rates to decide to allow contactor closing.
  //   - Once at least one slave is contactor_engaged: require contactor_engaged for
  //     charge/discharge rates so parallel slaves only contribute once physically connected.
  uint8_t engaged_count = 0;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    const SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (node.online && node.contactor_engaged && !node.balancing) {
      engaged_count++;
    }
  }
  const bool require_engaged = (engaged_count > 0);

  uint32_t total_capacity_Wh = 0;
  uint32_t total_remaining_Wh = 0;
  uint32_t total_max_charge_W = 0;
  uint32_t total_max_discharge_W = 0;
  int32_t total_current_dA = 0;
  uint16_t lowest_soc = 10001;   // Above max to detect "no data"
  uint16_t highest_soc = 0;
  int16_t highest_temp = -1270;
  int16_t lowest_temp = 1270;
  uint16_t shared_voltage_dV = 0;  // All slaves share voltage (parallel)
  uint16_t lowest_max_design_voltage_dV = 65535; // To safely limit inverter charge voltage
  uint16_t highest_min_design_voltage_dV = 0;    // To safely limit inverter discharge voltage
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
    // For charge/discharge rates: require contactor_engaged once any slave is engaged
    if (require_engaged && !node.contactor_engaged) {
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
    if (t_max_dC > highest_temp) highest_temp = t_max_dC;
    if (t_min_dC < lowest_temp) lowest_temp = t_min_dC;

    if (node.max_design_voltage_dV > 0 && node.max_design_voltage_dV < lowest_max_design_voltage_dV) {
      lowest_max_design_voltage_dV = node.max_design_voltage_dV;
    }
    if (node.min_design_voltage_dV > highest_min_design_voltage_dV) {
      highest_min_design_voltage_dV = node.min_design_voltage_dV;
    }
  }

  if (active_count == 0) {
    // No active slaves — report safe zeros
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.reported_current_dA = 0;
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
  datalayer.battery.status.max_charge_power_W =
      (charge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : total_max_charge_W;
  datalayer.battery.status.max_discharge_power_W =
      (discharge_blocked || datalayer.system.info.equipment_stop_active) ? 0u : total_max_discharge_W;
  datalayer.battery.status.reported_current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.voltage_dV = shared_voltage_dV;
  // Power = V × I: voltage in dV, current in dA → W = (dV × dA) / 100
  datalayer.battery.status.active_power_W =
      (int32_t)shared_voltage_dV * (int32_t)total_current_dA / 100;

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

  // Keep BMS status active so inverter sees system as alive
  datalayer.battery.status.bms_status = ACTIVE;
  datalayer.battery.status.real_bms_status = BMS_ACTIVE;

  // Signal inverter that system allows contactor
  datalayer.system.status.battery_allows_contactor_closing = true;
}
