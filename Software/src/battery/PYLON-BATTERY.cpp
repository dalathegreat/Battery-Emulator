#include "PYLON-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

void PylonBattery::update_values() {

  datalayer_battery->status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer_battery->status.soh_pptt = (SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer_battery->status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer_battery->status.max_charge_power_W = (max_charge_current * (voltage_dV / 10));

  datalayer_battery->status.max_discharge_power_W = (-max_discharge_current * (voltage_dV / 10));

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.cell_max_voltage_mV = cellvoltage_max_mV;
  datalayer_battery->status.cell_voltages_mV[0] = cellvoltage_max_mV;

  datalayer_battery->status.cell_min_voltage_mV = cellvoltage_min_mV;
  datalayer_battery->status.cell_voltages_mV[1] = cellvoltage_min_mV;

  datalayer_battery->status.temperature_min_dC = celltemperature_min_dC;

  datalayer_battery->status.temperature_max_dC = celltemperature_max_dC;

  datalayer_battery->info.max_design_voltage_dV = charge_cutoff_voltage;

  datalayer_battery->info.min_design_voltage_dV = discharge_cutoff_voltage;
}

void PylonBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x7310:
    case 0x7311:
      ensemble_info_ack = true;
      // This message contains software/hardware version info. No interest to us
      break;
    case 0x7320:
    case 0x7321:
      ensemble_info_ack = true;
      battery_module_quantity = rx_frame.data.u8[0];
      battery_modules_in_series = rx_frame.data.u8[2];
      cell_quantity_in_module = rx_frame.data.u8[3];
      voltage_level = rx_frame.data.u8[4];
      ah_number = rx_frame.data.u8[6];
      break;
    case 0x4210:
    case 0x4211:
      voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 30000;
      SOC = rx_frame.data.u8[6];
      SOH = rx_frame.data.u8[7];
      break;
    case 0x4220:
    case 0x4221:
      charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      discharge_cutoff_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      max_charge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.1) - 3000;
      max_discharge_current = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) * 0.1) - 3000;
      break;
    case 0x4230:
    case 0x4231:
      cellvoltage_max_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      cellvoltage_min_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x4240:
    case 0x4241:
      celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 1000;
      celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 1000;
      break;
    case 0x4250:
    case 0x4251:
      //Byte0 Basic Status
      //Byte1-2 Cycle Period
      //Byte3 Error
      //Byte4-5 Alarm
      //Byte6-7 Protection
      break;
    case 0x4260:
    case 0x4261:
      //Byte0-1 Module Max Voltage
      //Byte2-3 Module Min Voltage
      //Byte4-5 Module Max. Voltage Number
      //Byte6-7 Module Min. Voltage Number
      break;
    case 0x4270:
    case 0x4271:
      //Byte0-1 Module Max. Temperature
      //Byte2-3 Module Min. Temperature
      //Byte4-5 Module Max. Temperature Number
      //Byte6-7 Module Min. Temperature Number
      break;
    case 0x4280:
    case 0x4281:
      charge_forbidden = rx_frame.data.u8[0];
      discharge_forbidden = rx_frame.data.u8[1];
      break;
    case 0x4290:
    case 0x4291:
      break;
    default:
      break;
  }
}

void PylonBattery::transmit_can(unsigned long currentMillis) {
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    transmit_can_frame(&PYLON_3010, can_config.battery);  // Heartbeat
    transmit_can_frame(&PYLON_4200, can_config.battery);  // Ensemble OR System equipment info, depends on frame0
    transmit_can_frame(&PYLON_8200, can_config.battery);  // Control device quit sleep status
    transmit_can_frame(&PYLON_8210, can_config.battery);  // Charge command

    if (ensemble_info_ack) {
      PYLON_4200.data.u8[0] = 0x00;  //Request system equipment info
    }
  }
}

void PylonBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Pylon compatible battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 2;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;

  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
