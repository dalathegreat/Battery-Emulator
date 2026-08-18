#include "AIWAYS-U5-BATTERY.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"

void AiwaysU5Battery::update_values() {

  datalayer_battery->status.voltage_dV = pid_pack_voltage;

  datalayer_battery->status.current_dA = pid_pack_current;

  /* TODO, find all values
  datalayer_battery->status.real_soc;

  datalayer_battery->status.soh_pptt;

  datalayer_battery->status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer_battery->status.voltage_dV * datalayer_battery->status.current_dA) / 100);

  datalayer_battery->info.total_capacity_Wh;

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_charge_power_W;

  datalayer_battery->status.max_discharge_power_W;

  datalayer_battery->status.temperature_min_dC;

  datalayer_battery->status.temperature_max_dC;

  datalayer_battery->status.cell_min_voltage_mV;

  datalayer_battery->status.cell_max_voltage_mV;
  */
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String AiwaysU5Battery::get_uds_info_html() {
  String content;
  content.reserve(600);

  // clang-format off
  content << "<h4>882D: " << pid_cellvoltage << " mV</h4>";
  // clang-format on

  return content;
}

void AiwaysU5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x6B4 PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }
  switch (rx_frame.ID) {
    case 0x7E1:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

uint16_t AiwaysU5Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_PACK_VOLTAGE:
      pid_pack_voltage = (uint16_t)value;
      break;
    case PID_PACK_CURRENT:
      pid_pack_current = (int16_t)value;  //TODO; correct scaling? 0.1A steps? 1A steps?
      break;
    case PID_CELL_VOLTAGE:
      pid_cellvoltage = (uint16_t)value;  //TODO; correct scaling? 0.1mV steps? 1mV steps?
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void AiwaysU5Battery::transmit_can(unsigned long currentMillis) {

  // Send periodic CAN Messages simulating the car still being attached
  // Send 10ms messages
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

void AiwaysU5Battery::setup(void) {  // Performs one time setup at startup

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 100;  //TODO set
  datalayer_battery->info.total_capacity_Wh = 63000;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  // UDS: send requests to 0x7E1, accept replies from the BMS on ???.
  setup_uds(0x7E1, 0);
  static const uint16_t pid_scan_list[] = {
      PID_PACK_VOLTAGE,
      PID_PACK_CURRENT,
      PID_CELL_VOLTAGE,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
