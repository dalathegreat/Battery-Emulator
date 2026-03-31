#include "MICROVAST-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void MicrovastBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_ppt * 10);  //increase SOC range from 0-1000 -> 100.00

  datalayer.battery.status.soh_pptt = (SOH_ppt * 10);  //increase SOH range from 0-1000 -> 100.00

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = discharge_current_max * (battery_voltage / 10);

  datalayer.battery.status.max_charge_power_W = charge_current_max * (battery_voltage / 10);

  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max_mv;

  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min_mv;

  datalayer.battery.status.temperature_min_dC = lowest_cell_temperature_C * 10;

  datalayer.battery.status.temperature_max_dC = highest_cell_temperature_C * 10;
}

void MicrovastBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x18ebfff3:  //DM1_Msg 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18ecfff3:  //DM1_LinkReq 1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18fecaf3:  //DM1_SiggleData 1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x189dd0f3:  //BMS29_HardVsn1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x189bd0f3:  //BMS28_SoftVsn2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x189cd0f3:  //BMS27_SoftVsn1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18a0d0f3:  //BMS26_StrPackID
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x189ad0f3:  //BMS25_StrFlt2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1899d0f3:  //BMS24_StrFlt1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1898d0f3:  //BMS23_StrUCe
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1897d0f3:  //BMS22_StrTemp
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1896d0f3:  //BMS21_StrSts
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1895d0f3:  //BMS20_StrInf5
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1894d0f3:  //BMS19_StrInf4
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1893d0f3:  //BMS18_StrInf3
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1892d0f3:  //BMS17_StrInf2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x187fd0f3:  //BMS15_Flt2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x187ed0f3:  //BMS14_Flt1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x187dd0f3:  //BMS13_TMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x187ad0f3:  //BMS12_ExtChg
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1879d0f3:  //BMS11_Para2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1878d0f3:  //BMS10_Para1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1877d0f3:  //BMS9_CeTemp
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      highest_cell_temperature_C = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 40;
      lowest_cell_temperature_C = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 40;
      break;
    case 0x1876d0f3:  //BMS8_CeVolt
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellvoltage_max_mv = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      cellvoltage_min_mv = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      break;
    case 0x1875d0f3:  //BMS7_SysSts
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18a1d0f3:  //BMS6_Inf6
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1874d0f3:  //BMS5_Inf5
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1873d0f3:  //BMS4_Inf4
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOH_ppt = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
      break;
    case 0x1872d0f3:  //BMS4_Inf3
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1871d0f3:  //BMS4_Inf2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      discharge_current_max = (((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 3200);
      charge_current_max = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 3200;
      break;
    case 0x1870d0f3:  //BMS4_Inf1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      system_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      battery_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 3200;
      SOC_ppt = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
      break;
    case 0x1880d0f3:  //BMS32_Flt
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x189ed0f3:  //BMS30_HardVsn2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1860d0f3:  //BMS31_RlySts
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}
void MicrovastBattery::transmit_can(unsigned long currentMillis) {

  //TODO, 10ms cee06e message most likely needed

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    MICRO_084.data.u8[2] = 0x00;  //0 = init, 1 = sleep, 2 = drive, 3 = diagnostic

    transmit_can_frame(&MICRO_084);  //Standard frame
  }

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    counter_1s = (counter_1s + 1) % 16;  // counter_1s cycles between 0-1-2-3..15-0-1...

    //MICRO_cee00a0 TODO: Add CRC to frame0, and update counter in frame1
    MICRO_cee00a0.data.u8[0] = 0x00;  //CRC placeholder
    MICRO_cee00a0.data.u8[1] = counter_1s;

    transmit_can_frame(&MICRO_cee00a0);  //Extended frame
  }
}

void MicrovastBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
