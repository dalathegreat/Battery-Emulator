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

void setup_master_can() {
  register_can_receiver(&master_can, can_config.inter_unit, CAN_Speed::CAN_SPEED_500KBPS);
  register_transmitter(&master_can);
  logging.println("Master CAN: registered on inter-unit bus @ 500kbps");
}

void MasterCan::begin() {
  _last_heartbeat_ms = 0;
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    voltage_diff_seconds[i] = 0;
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
      node.fault_flags = flags & ~IU_FLAG_CONTACTOR_ENGAGED;
      node.contactor_engaged = (flags & IU_FLAG_CONTACTOR_ENGAGED) != 0;
      break;
    }
    case 0x01:  // POWER message
    {
      node.max_charge_W = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      node.max_discharge_W = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      node.remaining_Wh = ((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5];
      node.temp_min_dC = (int8_t)rx_frame->data.u8[6];
      break;
    }
    case 0x02:  // INFO message (every 10s)
    {
      node.total_capacity_Wh = ((uint16_t)rx_frame->data.u8[0] << 8) | rx_frame->data.u8[1];
      node.max_design_voltage_dV = ((uint16_t)rx_frame->data.u8[2] << 8) | rx_frame->data.u8[3];
      node.min_design_voltage_dV = ((uint16_t)rx_frame->data.u8[4] << 8) | rx_frame->data.u8[5];
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
    frame.data.u8[0] = (node.contactor_allowed && datalayer.system.status.inverter_allows_contactor_closing)
                           ? IU_CONTACTOR_ALLOW
                           : IU_CONTACTOR_OPEN;
    transmit_can_frame_to_interface(&frame, can_config.inter_unit);
  }
}

// ---- Value aggregation (called every 1s) ---------------------------

void MasterCan::update_values() {
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
      logging.println("Master CAN: Startup grace period done — contactor logic now active");
    }
  }

  // Decrement still_alive counters
  for (uint8_t i = 0; i < MAX_SLAVE_NODES; i++) {
    SLAVE_NODE_TYPE& node = datalayer.system.slave_nodes[i];
    if (!node.online) {
      continue;
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

    // Check voltage safety
    check_slave_voltage_safety(i);
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

  // During startup grace period, keep all contactors blocked so all slaves
  // can announce themselves at matching voltages before the inverter starts.
  if (!_startup_grace_done) {
    return;
  }

  // First slave to come online sets the reference voltage
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

  if (diff <= VOLTAGE_DIFF_THRESHOLD_dV) {
    // Voltage OK — allow contactor unless there's a fault
    voltage_diff_seconds[idx] = 0;
    bool has_fault = (node.fault_flags != 0);
    if (!has_fault && !node.contactor_allowed) {
      node.contactor_allowed = true;
      logging.printf("Master CAN: Slave %d contactor ALLOWED (voltage OK: %u.%uV)\n", idx + 1,
                     node.voltage_dV / 10, node.voltage_dV % 10);
    } else if (has_fault && node.contactor_allowed) {
      node.contactor_allowed = false;
      logging.printf("Master CAN: Slave %d contactor OPENED (fault flags: 0x%02X)\n", idx + 1, node.fault_flags);
    }
  } else {
    // Voltage difference too large
    voltage_diff_seconds[idx]++;
    if (voltage_diff_seconds[idx] >= VOLTAGE_DIFF_SECONDS_LIMIT && node.contactor_allowed) {
      node.contactor_allowed = false;
      logging.printf("Master CAN: Slave %d contactor OPENED (voltage diff %.1fV > threshold)\n", idx + 1,
                     diff / 10.0f);
    }
  }
}

void MasterCan::update_slave_aggregation() {
  // Aggregate online slaves into datalayer.battery
  uint32_t total_capacity_Wh = 0;
  uint32_t total_remaining_Wh = 0;
  uint32_t total_max_charge_W = 0;
  uint32_t total_max_discharge_W = 0;
  int32_t total_current_dA = 0;
  uint16_t lowest_soc = 10001;   // Above max to detect "no data"
  uint16_t highest_soc = 0;
  int16_t highest_temp = -1270;
  int16_t lowest_temp = 1270;
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
    active_count++;
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
  datalayer.battery.status.remaining_capacity_Wh = total_remaining_Wh;
  datalayer.battery.status.reported_remaining_capacity_Wh = total_remaining_Wh;
  datalayer.battery.status.max_charge_power_W = charge_blocked ? 0u : total_max_charge_W;
  datalayer.battery.status.max_discharge_power_W = discharge_blocked ? 0u : total_max_discharge_W;
  datalayer.battery.status.reported_current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.current_dA = (int16_t)total_current_dA;
  datalayer.battery.status.real_soc = lowest_soc;
  // Mirror Software.cpp multi-battery logic: report lowest SOC normally (discharge protection),
  // but override with highest SOC when any battery is nearly full (charge protection).
  uint16_t reported_soc = lowest_soc;
  if (highest_soc > 9900) {
    reported_soc = highest_soc;
  }
  datalayer.battery.status.reported_soc = reported_soc;
  datalayer.battery.status.temperature_max_dC = highest_temp;
  datalayer.battery.status.temperature_min_dC = lowest_temp;

  // Keep BMS status active so inverter sees system as alive
  datalayer.battery.status.bms_status = ACTIVE;
  datalayer.battery.status.real_bms_status = BMS_ACTIVE;

  // Signal inverter that system allows contactor
  datalayer.system.status.battery_allows_contactor_closing = true;
}
