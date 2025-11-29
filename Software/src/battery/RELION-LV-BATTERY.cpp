#include "RELION-LV-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
/*CAN Type:CAN2.0(Extended)
BPS:250kbps
Data Length: 8
Data Encoded Format:Motorola*/

uint16_t RelionBattery::estimateSOCfromCellvoltage(uint16_t cellVoltage) {
  for (int i = 1; i < numPoints; ++i) {
    if (cellVoltage >= cellVoltageLookup[i]) {
      // Cast to float for proper division
      float t = (float)(cellVoltage - cellVoltageLookup[i]) / (float)(cellVoltageLookup[i - 1] - cellVoltageLookup[i]);

      // Calculate interpolated SOC value
      uint16_t socDiff = SOC[i - 1] - SOC[i];
      uint16_t interpolatedValue = SOC[i] + (uint16_t)(t * socDiff);

      return interpolatedValue;
    }
  }
  return 0;  // Default return for safety, should never reach here
}

uint16_t RelionBattery::estimateSOC() {

  if (max_cell_voltage >= cellVoltageLookup[0]) {
    return 10000;  //One cell is in overvoltage, report 100.00% SOC
  }

  if (user_selected_max_cell_voltage_mV > 0) {  //If value configured by user
    if (max_cell_voltage >= user_selected_max_cell_voltage_mV) {
      return 10000;  //One cell is in overvoltage from user specified limit, report 100.00% SOC
    }
  }

  if (min_cell_voltage <= cellVoltageLookup[numPoints - 1]) {
    return 0;  //Cell overvoltage, report 0% SOC
  }

  if (user_selected_min_cell_voltage_mV > 0) {  //If value configured  by user
    if (min_cell_voltage <= user_selected_min_cell_voltage_mV) {
      return 0;  //Cell overvoltage from user specified limit, report 0% SOC
    }
  }

  SOC_from_max_cell_voltage = estimateSOCfromCellvoltage(max_cell_voltage);
  SOC_from_min_cell_voltage = estimateSOCfromCellvoltage(min_cell_voltage);

  if (max_cell_voltage > 3380) {  // We are in the higher end of SOC, use SOC% based on highest cell reading
    return SOC_from_max_cell_voltage;
  } else {  //We are in the lower end of SOC, use SOC% based on lowest cell reading
    return SOC_from_min_cell_voltage;
  }

  return 0;  // Default return for safety, should never reach here
}

void RelionBattery::update_values() {

  if (user_selected_use_estimated_SOC) {
    // Use the simplified pack-based SOC estimation with proper compensation
    datalayer.battery.status.real_soc = estimateSOC();
  } else {
    datalayer.battery.status.real_soc = battery_soc * 100;
  }

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = battery_soh * 100;

  datalayer.battery.status.voltage_dV = battery_total_voltage;

  datalayer.battery.status.current_dA = -battery_total_current;  //Charging negative, discharge positive

  /* Shows 0A all the time?
  datalayer.battery.status.max_charge_power_W =
      ((battery_total_voltage / 10) * charge_current_A);  //90A recommended charge current

  datalayer.battery.status.max_discharge_power_W =
      ((battery_total_voltage / 10) * discharge_current_A);  //150A max continous discharge current
  */
  datalayer.battery.status.max_discharge_power_W =
      datalayer.battery.status.override_discharge_power_W;  //TODO, fix when value is found
  if (datalayer.battery.status.cell_min_voltage_mV < MIN_CELL_VOLTAGE_MV) {
    datalayer.battery.status.max_discharge_power_W = 0;
  }

  //We have not found allowed charge power yet. Estimate it for now absed on UI setting. TODO. remove this once found
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if ((datalayer.battery.status.real_soc / 10) >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        RAMPDOWNPOWERALLOWED * (1 - ((battery_soc / 10) - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery.status.cell_max_voltage_mV > (MAX_CELL_VOLTAGE_MV - FLOAT_START_MV)) {
      datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  datalayer.battery.status.temperature_min_dC = max_cell_temperature * 10;

  datalayer.battery.status.temperature_max_dC = max_cell_temperature * 10;

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;
}

void RelionBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x02018100:  //ID1 (Example frame 10 08 01 F0 00 00 00 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_total_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      break;
    case 0x02028100:  //ID2 (Example frame 00 00 00 63 64 10 00 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_total_current = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      system_state = rx_frame.data.u8[2];
      battery_soc = rx_frame.data.u8[3];
      battery_soh = rx_frame.data.u8[4];
      most_serious_fault = rx_frame.data.u8[5];
      break;
    case 0x02038100:  //ID3 (Example frame 0C F9 01 04 0C A7 01 0F)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_cell_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      min_cell_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x02648100:  //Charging limitis
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      charge_current_A = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) - 800;
      regen_charge_current_A = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 800;
      discharge_current_A = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 800;
      break;
    case 0x02048100:  ///Temperatures min/max 2048100 [8] 47 01 01 47 01 01 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_cell_temperature = rx_frame.data.u8[0] - 50;
      min_cell_temperature = rx_frame.data.u8[2] - 50;
      break;
    case 0x02468100:  ///Raw temperatures 2468100 [8] 47 47 47 47 47 47 47 47
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02478100:  ///? 2478100 [8] 32 32 32 32 32 32 32 32
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
      //ID6 = 0x02108100 ~ 0x023F8100****** Cell Voltage 1~192******
    case 0x02108100:  ///Cellvoltages 1 2108100 [8] 0C F9 0C F8 0C F8 0C F9
      datalayer.battery.status.cell_voltages_mV[0] = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      datalayer.battery.status.cell_voltages_mV[1] = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      datalayer.battery.status.cell_voltages_mV[2] = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      datalayer.battery.status.cell_voltages_mV[3] = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02118100:  ///Cellvoltages 2 2118100 [8] 0C F8 0C F8 0C F9 0C F8
      datalayer.battery.status.cell_voltages_mV[4] = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      datalayer.battery.status.cell_voltages_mV[5] = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      datalayer.battery.status.cell_voltages_mV[6] = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      datalayer.battery.status.cell_voltages_mV[7] = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02128100:  ///Cellvoltages 3 2128100 [8] 0C F8 0C F8 0C F9 0C F8
      datalayer.battery.status.cell_voltages_mV[8] = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      datalayer.battery.status.cell_voltages_mV[9] = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      datalayer.battery.status.cell_voltages_mV[10] = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      datalayer.battery.status.cell_voltages_mV[11] = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02138100:  ///Cellvoltages 4 2138100 [8] 0C F9 0C CD 0C A7 00 00
      datalayer.battery.status.cell_voltages_mV[12] = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      datalayer.battery.status.cell_voltages_mV[13] = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      datalayer.battery.status.cell_voltages_mV[14] = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      datalayer.battery.status.cell_voltages_mV[15] = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02058100:  ///? 2058100 [8] 00 0C 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02068100:  ///? 2068100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02148100:  ///? 2148100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02508100:  ///? 2508100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02518100:  ///? 2518100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02528100:  ///? 2528100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02548100:  ///? 2548100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x024A8100:  ///? 24A8100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02558100:  ///? 2558100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02538100:  ///? 2538100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02568100:  ///? 2568100 [8] 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void RelionBattery::transmit_can(unsigned long currentMillis) {
  // No periodic sending for this protocol
}

void RelionBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.chemistry = LFP;
  datalayer.battery.info.number_of_cells = 16;
  if (user_selected_max_pack_voltage_dV > 0) {
    datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  } else {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  }

  if (user_selected_min_pack_voltage_dV > 0) {
    datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  } else {
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  }

  if (user_selected_max_cell_voltage_mV > 0) {
    datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  } else {
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  }

  if (user_selected_min_cell_voltage_mV > 0) {
    datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  } else {
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  }
  datalayer.system.status.battery_allows_contactor_closing = true;
}
