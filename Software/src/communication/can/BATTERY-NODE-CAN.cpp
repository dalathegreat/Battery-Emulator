#include "BATTERY-NODE-CAN.h"

#include <Arduino.h>
#include <WiFi.h>

#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../communication/Transmitter.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

#ifndef SMALL_FLASH_DEVICE

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

// Count of inter-unit frames dropped because their application CRC did not validate.
static uint32_t iu_node_crc_errors = 0;

void BatteryNodeCan::receive_can_frame(CAN_frame* rx_frame) {
  const uint32_t id = rx_frame->ID;
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  // Controller heartbeat broadcast
  if (id == IU_CONTROLLER_HEARTBEAT_ID) {
    // Reject a corrupt heartbeat: do NOT reset the watchdog or schedule a reply, so a garbled
    // (or foreign) frame can never keep us "alive". The heartbeat watchdog runs down and safety
    // opens the contactor — the intended fail-safe.
    if (!iu_crc_valid(id, rx_frame->data.u8, rx_frame->DLC)) {
      iu_node_crc_errors++;
      logging.printf("Battery node CAN: heartbeat CRC mismatch (count=%u) — ignored\n", iu_node_crc_errors);
      return;
    }
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
    // Reset watchdog. Deliberately much shorter than CAN_STILL_ALIVE: on a lost link the controller
    // has already driven the pack to 0 W, and we must open before it lets the others resume
    // (see "Lost-link sequencing" in INTER-UNIT-PROTOCOL.h).
    datalayer.system.status.CAN_controller_still_alive = IU_NODE_CONTROLLER_TIMEOUT_S;
    _heartbeat_count++;

    // Schedule reply after (node_id * 5ms) to avoid CAN collisions
    _reply_due_ms = millis() + IU_NODE_REPLY_DELAY_MS(node_id);
    _reply_pending = true;
    return;
  }

  // Contactor command addressed to this node
  if (id == IU_CONTROLLER_CONTACTOR_ID(node_id)) {
    // Reject a corrupt command: keep the previous contactor state rather than acting on a
    // garbled ALLOW/OPEN. Loss of valid commands is still caught by the heartbeat watchdog.
    if (!iu_crc_valid(id, rx_frame->data.u8, rx_frame->DLC)) {
      iu_node_crc_errors++;
      logging.printf("Battery node CAN: contactor cmd CRC mismatch (count=%u) — ignored\n", iu_node_crc_errors);
      return;
    }
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
    // then only every 10 minutes (600s) — IP can change on a DHCP renew without a reboot.
    if (_heartbeat_count <= 3 || _heartbeat_count % 600 == 0) {
      send_ip_frame();
    }
    // IDENT (FW version + battery type) only changes across a reboot, and the node already
    // re-announces on its first 3 heartbeats — including after a controller-reboot gap, since
    // the heartbeat handler resets _heartbeat_count. Re-sending it every 10 min added nothing
    // but a recurring window for a single garbled frame to raise a false firmware-mismatch
    // warning on the controller, so send IDENT at startup only.
    if (_heartbeat_count <= 3) {
      send_ident_frame();
    }
  }
}

static bool node_event_active(EVENTS_ENUM_TYPE e) {
  const EVENTS_STRUCT_TYPE* p = get_event_pointer(e);
  return p != nullptr && (p->state == EVENT_STATE_ACTIVE || p->state == EVENT_STATE_ACTIVE_LATCHED);
}

/* The node is set up as this unit's inverter, so like every inverter protocol it reports the
   installation behind it (datalayer.aggregate) rather than pack 1 alone. With a single pack the
   aggregate is a plain copy of datalayer.battery, so the frames are unchanged. Double/triple
   battery on a node then reaches the controller as one combined node. */

uint8_t BatteryNodeCan::build_fault_flags() {
  uint8_t flags = 0;

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
  if (datalayer.aggregate.temperature_max_dC > 550) {  // IU_FAULT_OVERTEMPERATURE is defined as >55°C
    flags |= IU_FAULT_OVERTEMPERATURE;
  }
  // Any other active WARNING (e.g. cell/temp deviation) still needs to surface. If no specific
  // warning bit was set above, fall back to a generic warning bit so the controller sees it.
  if (get_event_level() == EVENT_LEVEL_WARNING && (flags & IU_FAULT_WARNING_MASK) == 0) {
    flags |= IU_FAULT_CELL_UNDERVOLTAGE;  // generic warning placeholder
  }
  // Battery CAN timeout (battery went offline at node)
  if (datalayer.battery.status.CAN_battery_still_alive == 0 ||
      (battery2 && datalayer.battery2.status.CAN_battery_still_alive == 0) ||
      (battery3 && datalayer.battery3.status.CAN_battery_still_alive == 0)) {
    flags |= IU_FAULT_BATTERY_TIMEOUT;
  }
  // The controller decides when this node joins the DC bus, so every pack here must actually
  // follow the contactor command. If one cannot, report a contactor fault: the controller then
  // never allows this node (IU_FAULT_CONTACTOR_FAILED is in IU_FAULT_ERROR_MASK).
  if (!node_contactors_controllable()) {
    set_event(EVENT_NODE_CONTACTOR_UNSUPPORTED, 0);
    flags |= IU_FAULT_CONTACTOR_FAILED;
  }
  // Contactor engaged confirmation: only CLOSED (1). Not 2 (opened and latched after a fault) or
  // 3 (precharge still running) — the controller uses this as the DC bus reference for joins.
  if (datalayer.system.status.contactors_engaged == 1) {
    flags |= IU_FLAG_CONTACTOR_ENGAGED;
  }
  return flags;
}

bool node_contactors_controllable() {
  // Each pack needs either GPIO contactor control for its slot, or a driver that closes over CAN
  // only on command and reports the resulting state (Battery::reports_contactor_state()).
  if (battery == nullptr || !(contactor_control_enabled || battery->reports_contactor_state())) {
    return false;
  }
  if (battery2 && !(contactor_control_enabled_double_battery || battery2->reports_contactor_state())) {
    return false;
  }
  if (battery3 && !(contactor_control_enabled_triple_battery || battery3->reports_contactor_state())) {
    return false;
  }
  return true;
}

void BatteryNodeCan::send_status_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& agg = datalayer.aggregate;

  CAN_frame frame = {};
  frame.ID = IU_NODE_STATUS_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  // [0..1] voltage_dV — pack 1's measured link voltage, NOT agg.voltage_dV: the aggregate swaps in a
  // 370/330 V placeholder while the pack reads 0, which would fool the controller's voltage match.
  const uint16_t voltage_dV = datalayer.battery.status.voltage_dV;
  frame.data.u8[0] = (voltage_dV >> 8) & 0xFF;
  frame.data.u8[1] = voltage_dV & 0xFF;
  // [2] reported_soc (0.01%) scaled to 0.5% steps on the wire (freed a byte for the CRC)
  uint16_t soc_wire = agg.reported_soc / IU_SOC_WIRE_SCALE;
  frame.data.u8[2] = (soc_wire > 255u) ? 255u : (uint8_t)soc_wire;
  // [3..4] current_dA (signed), sum of every pack
  uint16_t current_raw = (uint16_t)agg.current_dA;
  frame.data.u8[3] = (current_raw >> 8) & 0xFF;
  frame.data.u8[4] = current_raw & 0xFF;
  // [5] temp_max in °C (divide dC by 10, clamp to int8)
  int8_t temp_max = (int8_t)((int16_t)(agg.temperature_max_dC / 10));
  frame.data.u8[5] = (uint8_t)temp_max;
  // [6] flags (bit7 = toggle, alternates each frame so controller can detect stale data)
  static bool _toggle = false;
  _toggle = !_toggle;
  frame.data.u8[6] = build_fault_flags() | (_toggle ? IU_FLAG_STATUS_TOGGLE : 0u);
  // [7] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_power_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& agg = datalayer.aggregate;

  CAN_frame frame = {};
  frame.ID = IU_NODE_POWER_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  // Power limits travel in 10 W steps, rounded down (never over-report) and clamped to uint16_t
  const uint32_t chg_wire = agg.max_charge_power_W / IU_POWER_W_WIRE_SCALE;
  const uint32_t dch_wire = agg.max_discharge_power_W / IU_POWER_W_WIRE_SCALE;
  uint16_t max_chg = (chg_wire > 65535u) ? 65535u : (uint16_t)chg_wire;
  uint16_t max_dch = (dch_wire > 65535u) ? 65535u : (uint16_t)dch_wire;

  // [0..1] max_charge_W / 10
  frame.data.u8[0] = (max_chg >> 8) & 0xFF;
  frame.data.u8[1] = max_chg & 0xFF;
  // [2..3] max_discharge_W / 10
  frame.data.u8[2] = (max_dch >> 8) & 0xFF;
  frame.data.u8[3] = max_dch & 0xFF;
  // [4..5] rem_word: bits0..14 = remaining_Wh in 10 Wh steps, bit15 = offline-balancing flag.
  // (The old dedicated pflags byte was reclaimed for the CRC.) Online balancing (e.g. BMW IX)
  // does NOT set the flag — that node stays in aggregation.
  const uint32_t rem_wire = agg.remaining_capacity_Wh / IU_REM_WH_WIRE_SCALE;
  uint16_t rem_word = (rem_wire > IU_NODE_REM_VALUE_MASK) ? IU_NODE_REM_VALUE_MASK : (uint16_t)rem_wire;
  if (datalayer.battery.status.offline_balancing || (battery2 && datalayer.battery2.status.offline_balancing) ||
      (battery3 && datalayer.battery3.status.offline_balancing)) {
    rem_word |= IU_NODE_REM_BALANCING_BIT;
  }
  frame.data.u8[4] = (rem_word >> 8) & 0xFF;
  frame.data.u8[5] = rem_word & 0xFF;
  // [6] temp_min in °C
  int8_t temp_min = (int8_t)((int16_t)(agg.temperature_min_dC / 10));
  frame.data.u8[6] = (uint8_t)temp_min;
  // [7] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_info_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;
  const auto& agg = datalayer.aggregate;

  CAN_frame frame = {};
  frame.ID = IU_NODE_INFO_ID(node_id);
  frame.DLC = 8;
  frame.ext_ID = false;

  const uint32_t cap_wire = agg.total_capacity_Wh / IU_CAP_WH_WIRE_SCALE;
  uint16_t total_wire = (cap_wire > 65535u) ? 65535u : (uint16_t)cap_wire;

  // [0..1] total_capacity_Wh in 10 Wh steps
  frame.data.u8[0] = (total_wire >> 8) & 0xFF;
  frame.data.u8[1] = total_wire & 0xFF;
  // [2..3] max_design_voltage_dV
  frame.data.u8[2] = (agg.max_design_voltage_dV >> 8) & 0xFF;
  frame.data.u8[3] = agg.max_design_voltage_dV & 0xFF;
  // [4..5] min_design_voltage_dV
  frame.data.u8[4] = (agg.min_design_voltage_dV >> 8) & 0xFF;
  frame.data.u8[5] = agg.min_design_voltage_dV & 0xFF;
  // [6] soh_pptt (0.01%) scaled to 0.5% steps on the wire (freed a byte for the CRC)
  uint16_t soh_wire = agg.soh_pptt / IU_SOH_WIRE_SCALE;
  frame.data.u8[6] = (soh_wire > 255u) ? 255u : (uint8_t)soh_wire;
  // [7] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

void BatteryNodeCan::send_cell_frame() {
  const uint8_t node_id = datalayer.system.status.battery_node_id;

  CAN_frame frame = {};
  frame.ID = IU_NODE_CELL_ID(node_id);
  frame.DLC = 5;
  frame.ext_ID = false;

  uint16_t max_mv = datalayer.aggregate.cell_max_voltage_mV;
  uint16_t min_mv = datalayer.aggregate.cell_min_voltage_mV;

  frame.data.u8[0] = (max_mv >> 8) & 0xFF;
  frame.data.u8[1] = max_mv & 0xFF;
  frame.data.u8[2] = (min_mv >> 8) & 0xFF;
  frame.data.u8[3] = min_mv & 0xFF;
  // [4] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

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
  frame.DLC = 5;
  frame.ext_ID = false;

  // [0..3] IPv4 address, big-endian
  frame.data.u8[0] = (ip >> 24) & 0xFF;
  frame.data.u8[1] = (ip >> 16) & 0xFF;
  frame.data.u8[2] = (ip >> 8) & 0xFF;
  frame.data.u8[3] = ip & 0xFF;
  // [4] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

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
  // [4] protocol version — the controller ignores an IDENT from a different wire layout
  frame.data.u8[4] = IU_PROTOCOL_VERSION;
  // [5..6] reserved (must be 0 — controller validates this)
  frame.data.u8[5] = 0;
  frame.data.u8[6] = 0;
  // [7] CRC
  iu_crc_stamp(frame.ID, frame.data.u8, frame.DLC);

  transmit_can_frame_to_interface(&frame, can_config.inverter);
}

#endif  // SMALL_FLASH_DEVICE
