#include "BATTERY-NODE-CAN.h"

#include <Arduino.h>
#include <WiFi.h>

#include "../../battery/Battery.h"
#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

BatteryNodeCan battery_node_can;

void setup_battery_node_can() {
  register_can_receiver(&battery_node_can, can_config.inverter, CAN_Speed::CAN_SPEED_500KBPS);
  register_transmitter(&battery_node_can);
  // Block contactor until controller explicitly sends ALLOW command
  datalayer.system.status.inverter_allows_contactor_closing = false;
  logging.println("Battery node CAN: registered on inverter bus @ 500kbps");
}

void BatteryNodeCan::begin() {
  _last_heartbeat_ms = 0;
  _reply_pending = false;
  _controller_online = false;
  _heartbeat_count = 0;
}

void BatteryNodeCan::receive_can_frame(CAN_frame* rx_frame) {
  const uint32_t id = rx_frame->ID;
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  // Controller heartbeat broadcast
  if (id == IU_CONTROLLER_HEARTBEAT_ID) {
    const unsigned long now = millis();
    // Detect a gap in controller heartbeats: the controller restarted (e.g. after a
    // firmware flash) and cleared its RAM, including our IDENT and IP. Our own
    // _heartbeat_count keeps running across the controller outage, so the periodic
    // re-announce (every 600 heartbeats) could be up to 10 minutes away. Restart
    // the announce window on any such gap so the controller re-learns our FW/battery
    // type and IP immediately instead of showing "waiting..." for minutes.
    if (_last_heartbeat_ms != 0 && (now - _last_heartbeat_ms) > IU_CONTROLLER_REBOOT_GAP_MS) {
      _heartbeat_count = 0;
    }
    _last_heartbeat_ms = now;
    _controller_online = true;
    datalayer.system.status.controller_online = true;
    datalayer.system.status.CAN_controller_still_alive = CAN_STILL_ALIVE;  // Reset watchdog
    _heartbeat_count++;

    // Schedule reply after (node_id * 5ms) to avoid CAN collisions
    _reply_due_ms = millis() + IU_NODE_REPLY_DELAY_MS(node_id);
    _reply_pending = true;
    return;
  }

  // Contactor command addressed to this node
  if (id == IU_CONTROLLER_CONTACTOR_ID(node_id)) {
    bool allow = (rx_frame->data.u8[0] & IU_CONTACTOR_ALLOW) != 0;
    if (datalayer.system.status.inverter_allows_contactor_closing != allow) {
      logging.printf("Battery node CAN: contactor command received - %s (raw byte: 0x%02X)\n",
                     allow ? "ALLOW CLOSE" : "BLOCK CLOSE", rx_frame->data.u8[0]);
    }
    datalayer.system.status.inverter_allows_contactor_closing = allow;
    return;
  }
}

void BatteryNodeCan::transmit(unsigned long currentMillis) {
  // If controller has gone offline, block contactor closing.
  // contactor control only checks inverter_allows_contactor_closing, so we must set it here.
  if (!datalayer.system.status.controller_online) {
    datalayer.system.status.inverter_allows_contactor_closing = false;
  }

  // === Send reply frames if due ===
  if (_reply_pending && currentMillis >= _reply_due_ms) {
    _reply_pending = false;
    send_status_frame();
    send_power_frame();
    if (_heartbeat_count % IU_INFO_INTERVAL_HEARTBEATS == 0) {
      send_info_frame();
    }
    if (_heartbeat_count % (IU_INFO_INTERVAL_HEARTBEATS / 5) ==
        0) {  // Every 2s (assuming heartbeat is 1s and info interval is 10)
      send_cell_frame();
    }
    // Send IP on first 3 heartbeats (so controller gets it quickly after boot),
    // then only every 10 minutes (600s) since IP rarely changes.
    if (_heartbeat_count <= 3 || _heartbeat_count % 600 == 0) {
      send_ip_frame();
      send_ident_frame();
    }
  }
}

static bool node_event_active(EVENTS_ENUM_TYPE e) {
  const EVENTS_STRUCT_TYPE* p = get_event_pointer(e);
  return p != nullptr && (p->state == EVENT_STATE_ACTIVE || p->state == EVENT_STATE_ACTIVE_LATCHED);
}

uint8_t BatteryNodeCan::build_fault_flags() {
  uint8_t flags = 0;
  const auto& status = datalayer.battery.status;

  // BMS fault (set by any ERROR-level event on the node)
  if (datalayer.system.status.system_status == FAULT) {
    flags |= IU_FAULT_BMS_FAULT;
  }
  // Map specific WARNING conditions to their matching warning bit so the controller UI shows
  // the real cause (OV vs UV vs over-temp) instead of a single placeholder bit.
  if (node_event_active(EVENT_CELL_OVER_VOLTAGE)) {
    flags |= IU_FAULT_CELL_OVERVOLTAGE;
  }
  if (node_event_active(EVENT_CELL_UNDER_VOLTAGE)) {
    flags |= IU_FAULT_CELL_UNDERVOLTAGE;
  }
  if (status.temperature_max_dC > 550) {  // IU_FAULT_OVERTEMPERATURE is defined as >55°C
    flags |= IU_FAULT_OVERTEMPERATURE;
  }
  // Any other active WARNING (e.g. cell/temp deviation) still needs to surface. If no specific
  // warning bit was set above, fall back to a generic warning bit so the controller sees it.
  if (get_event_level() == EVENT_LEVEL_WARNING && (flags & IU_FAULT_WARNING_MASK) == 0) {
    flags |= IU_FAULT_CELL_UNDERVOLTAGE;  // generic warning placeholder
  }
  // Battery CAN timeout (battery went offline at node)
  if (status.CAN_battery_still_alive == 0) {
    flags |= IU_FAULT_BATTERY_TIMEOUT;
  }
  // Contactor engaged confirmation
  if (datalayer.system.status.contactors_engaged != 0) {
    flags |= IU_FLAG_CONTACTOR_ENGAGED;
  }
  return flags;
}

void BatteryNodeCan::send_status_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& status = datalayer.battery.status;

  CAN_frame frame = {};
  frame.ID = IU_NODE_STATUS_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  // [0..1] voltage_dV
  frame.data.u8[0] = (status.voltage_dV >> 8) & 0xFF;
  frame.data.u8[1] = status.voltage_dV & 0xFF;
  // [2..3] reported_soc (scaled if soc_scaling_active, otherwise real_soc)
  frame.data.u8[2] = (status.reported_soc >> 8) & 0xFF;
  frame.data.u8[3] = status.reported_soc & 0xFF;
  // [4..5] current_dA (signed)
  uint16_t current_raw = (uint16_t)status.current_dA;
  frame.data.u8[4] = (current_raw >> 8) & 0xFF;
  frame.data.u8[5] = current_raw & 0xFF;
  // [6] temp_max in °C (divide dC by 10, clamp to int8)
  int8_t temp_max = (int8_t)((int16_t)(status.temperature_max_dC / 10));
  frame.data.u8[6] = (uint8_t)temp_max;
  // [7] flags (bit7 = toggle, alternates each frame so controller can detect stale data)
  static bool _toggle = false;
  _toggle = !_toggle;
  frame.data.u8[7] = build_fault_flags() | (_toggle ? IU_FLAG_STATUS_TOGGLE : 0u);

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_power_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& status = datalayer.battery.status;

  CAN_frame frame = {};
  frame.ID = IU_NODE_POWER_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  // Clamp power values to uint16_t range
  uint16_t max_chg = (status.max_charge_power_W > 65535u) ? 65535u : (uint16_t)status.max_charge_power_W;
  uint16_t max_dch = (status.max_discharge_power_W > 65535u) ? 65535u : (uint16_t)status.max_discharge_power_W;
  uint16_t rem_wh = (status.remaining_capacity_Wh > 65535u) ? 65535u : (uint16_t)status.remaining_capacity_Wh;

  // [0..1] max_charge_W
  frame.data.u8[0] = (max_chg >> 8) & 0xFF;
  frame.data.u8[1] = max_chg & 0xFF;
  // [2..3] max_discharge_W
  frame.data.u8[2] = (max_dch >> 8) & 0xFF;
  frame.data.u8[3] = max_dch & 0xFF;
  // [4..5] remaining_Wh
  frame.data.u8[4] = (rem_wh >> 8) & 0xFF;
  frame.data.u8[5] = rem_wh & 0xFF;
  // [6] temp_min in °C
  int8_t temp_min = (int8_t)((int16_t)(status.temperature_min_dC / 10));
  frame.data.u8[6] = (uint8_t)temp_min;
  // [7] node_pflags: IU_NODE_PFLAG_BALANCING only for offline balancing (battery goes to sleep)
  // Online balancing (e.g. BMW IX) does NOT set this flag — node stays in aggregation
  frame.data.u8[7] = status.offline_balancing ? IU_NODE_PFLAG_BALANCING : 0u;

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_info_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& info = datalayer.battery.info;

  CAN_frame frame = {};
  frame.ID = IU_NODE_INFO_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  uint16_t total_wh = (info.total_capacity_Wh > 65535u) ? 65535u : (uint16_t)info.total_capacity_Wh;

  // [0..1] total_capacity_Wh
  frame.data.u8[0] = (total_wh >> 8) & 0xFF;
  frame.data.u8[1] = total_wh & 0xFF;
  // [2..3] max_design_voltage_dV
  frame.data.u8[2] = (info.max_design_voltage_dV >> 8) & 0xFF;
  frame.data.u8[3] = info.max_design_voltage_dV & 0xFF;
  // [4..5] min_design_voltage_dV
  frame.data.u8[4] = (info.min_design_voltage_dV >> 8) & 0xFF;
  frame.data.u8[5] = info.min_design_voltage_dV & 0xFF;
  // [6..7] soh_pptt
  uint16_t soh_pptt = datalayer.battery.status.soh_pptt;
  frame.data.u8[6] = (soh_pptt >> 8) & 0xFF;
  frame.data.u8[7] = soh_pptt & 0xFF;

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_cell_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  CAN_frame frame = {};
  frame.ID = IU_NODE_CELL_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  uint16_t max_mv = datalayer.battery.status.cell_max_voltage_mV;
  uint16_t min_mv = datalayer.battery.status.cell_min_voltage_mV;

  frame.data.u8[0] = (max_mv >> 8) & 0xFF;
  frame.data.u8[1] = max_mv & 0xFF;
  frame.data.u8[2] = (min_mv >> 8) & 0xFF;
  frame.data.u8[3] = min_mv & 0xFF;
  frame.data.u8[4] = 0;
  frame.data.u8[5] = 0;
  frame.data.u8[6] = 0;
  frame.data.u8[7] = 0;

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_ip_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  uint32_t ip = (uint32_t)WiFi.localIP();
  if (ip == 0) {
    return;  // Not connected yet — skip
  }

  CAN_frame frame = {};
  frame.ID = IU_NODE_IP_ID(node_id);
  frame.DLC = 4;
  frame.ext_ID = false;

  // [0..3] IPv4 address, big-endian
  frame.data.u8[0] = (ip >> 24) & 0xFF;
  frame.data.u8[1] = (ip >> 16) & 0xFF;
  frame.data.u8[2] = (ip >> 8) & 0xFF;
  frame.data.u8[3] = ip & 0xFF;

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_ident_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  CAN_frame frame = {};
  frame.ID = IU_NODE_IDENT_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  // [0..1] firmware version: (major<<8)|minor
  uint16_t fw_ver = iu_fw_version_num();
  frame.data.u8[0] = (fw_ver >> 8) & 0xFF;
  frame.data.u8[1] = fw_ver & 0xFF;
  // [2..3] battery type ID (BatteryType enum)
  uint16_t btype = (uint16_t)user_selected_battery_type;
  frame.data.u8[2] = (btype >> 8) & 0xFF;
  frame.data.u8[3] = btype & 0xFF;
  // [4..7] reserved
  frame.data.u8[4] = 0;
  frame.data.u8[5] = 0;
  frame.data.u8[6] = 0;
  frame.data.u8[7] = 0;

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}
