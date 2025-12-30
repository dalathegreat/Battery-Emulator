#include "PYLON-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"

/*Based on CAN-Bus-Protocol-Pylon-high-voltage-V1.26-20210903.pdf , which is the Pylontech 1.26 std */

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
  switch (rx_frame.ID) {
    case 0x7310:  //System equipment info
    case 0x7311:
      // This message contains software/hardware version info. No interest to us
      break;
    case 0x7320:
    case 0x7321:
      battery_module_quantity = rx_frame.data.u8[0];
      battery_modules_in_series = rx_frame.data.u8[2];
      cell_quantity_in_module = rx_frame.data.u8[3];
      voltage_level = rx_frame.data.u8[4];
      ah_number = rx_frame.data.u8[6];
      break;
    case 0x4210:
    case 0x4211:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 30000;
      BMS_temperature_dC = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4])) - 1000;
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

    PYLON_8200.data.u8[0] = 0xAA;  //AA = Quit sleep, 55 = Goto sleep

    PYLON_8210.data.u8[0] = 0xAA;  //TODO: how should we control this?
    /*Charge Command: When the battery is in under-voltage protection, the contactors are open. When
    we are about to charge the battery, send this command, then the battery will close contactors. 
    If the battery is in sleep status, wake up first then use this command.*/

    PYLON_8210.data.u8[1] = 0x00;  //TODO: how should we control this?
    /*Discharge Command: When the battery is in over-voltage protection, the contactors are open. When
    we are about to discharge the battery, send this command, then the battery will close contactors. 
    If the battery is in sleep status, wake up first then use this command.*/

    transmit_can_frame(&PYLON_3010);  // Heartbeat
    transmit_can_frame(&PYLON_4200);  // Ensemble OR System equipment info, depends on frame0
    transmit_can_frame(&PYLON_8200);  // Control device quit sleep status
    transmit_can_frame(&PYLON_8210);  // Charge command

    //transmit_can_frame(&PYLON_8240);  // Emergency Charge command
    //TODO: Implement? This message can be used to force battery on for 5 minutes, ignoring ext comm errors

    mux = (mux + 1) % 3;  // mux cycles between 0-1-2-0-1...
    PYLON_4200.data.u8[0] = mux;
    /*00 Request Ensamble Information (Battery will respond 0x42XX messages)
    01 Request Cellvoltages (Battery will respond 0x5XXX messages)
    02 Request System equipment info (Battery will respond 0x73XX messages)*/
  }
}

void PylonBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Pylon compatible battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 2;
  datalayer_battery->info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer_battery->info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer_battery->info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer_battery->info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
