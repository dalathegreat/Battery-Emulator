#include "SAMSUNG-SDI-LV-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/*
- Baud rate: 500kbps
- Format: CAN2.0A 11 bit identifier
- Data Length: 8byte
- CAN data is transmitted with encoding in little endian – low byte first – unless stated otherwise.
- Broadcasting period: 500ms

TODO: Implement the error bit handling for easier visualization if the battery stops operating
- alarms_frame0
- alarms_frame1
- protection_frame2
- protection_frame3

*/

void SamsungSdiLVBattery::update_values() {

  datalayer.battery.status.real_soc = system_SOC * 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = system_SOH * 100;

  datalayer.battery.status.voltage_dV = system_voltage / 10;

  datalayer.battery.status.current_dA = system_current * 10;

  datalayer.battery.status.max_charge_power_W = system_voltage * charge_current_limit;

  datalayer.battery.status.max_discharge_power_W = system_voltage * discharge_current_limit;

  datalayer.battery.status.temperature_min_dC = (int16_t)(minimum_cell_temperature * 10);

  datalayer.battery.status.temperature_max_dC = (int16_t)(maximum_cell_temperature * 10);

  datalayer.battery.status.cell_max_voltage_mV = maximum_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = minimum_cell_voltage;

  datalayer.battery.info.max_design_voltage_dV = battery_charge_voltage;

  datalayer.battery.info.min_design_voltage_dV = battery_discharge_voltage;
}

void SamsungSdiLVBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x500:  //Voltage, current, SOC, SOH
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      system_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      system_current = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      system_SOC = rx_frame.data.u8[4];
      system_SOH = rx_frame.data.u8[5];
      break;
    case 0x501:
      alarms_frame0 = rx_frame.data.u8[0];
      alarms_frame1 = rx_frame.data.u8[1];
      protection_frame2 = rx_frame.data.u8[2];
      protection_frame3 = rx_frame.data.u8[3];
      break;
    case 0x502:
      battery_charge_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      charge_current_limit = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      discharge_current_limit = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      battery_discharge_voltage = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      break;
    case 0x503:
      maximum_cell_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      minimum_cell_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x504:
      maximum_cell_temperature = rx_frame.data.u8[5];
      minimum_cell_temperature = rx_frame.data.u8[6];
      break;
    case 0x505:
      system_permanent_failure_status_dry_contact = rx_frame.data.u8[2];
      system_permanent_failure_status_fuse_open = rx_frame.data.u8[3];
      break;
    default:
      break;
  }
}

void SamsungSdiLVBattery::transmit_can(unsigned long currentMillis) {
  //No periodic sending required
}

void SamsungSdiLVBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.total_capacity_Wh = 5000;
}
