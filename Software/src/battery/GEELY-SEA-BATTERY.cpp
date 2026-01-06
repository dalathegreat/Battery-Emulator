#include "GEELY-SEA-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void GeelySeaBattery::

    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter

  // Update requests from webserver datalayer
  if (datalayer_extended.GeelySEA.UserRequestDTCreset) {
    transmit_can_frame(&SEA_DTC_Erase);  //Send global DTC erase command
    datalayer_extended.GeelySEA.UserRequestDTCreset = false;
  }
  if (datalayer_extended.GeelySEA.UserRequestBECMecuReset) {
    transmit_can_frame(&SEA_BECM_ECUreset);  //Send BECM ecu reset command
    datalayer_extended.GeelySEA.UserRequestBECMecuReset = false;
  }
  if (datalayer_extended.GeelySEA.UserRequestDTCreadout) {
    transmit_can_frame(&SEA_DTC_Req);  //Send DTC readout command
    datalayer_extended.GeelySEA.UserRequestDTCreadout = false;
  }

  /*if (datalayer_extended.GeelySEA.BECMBatteryVoltage > 0) {
    datalayer.battery.status.voltage_dV = datalayer_extended.GeelySEA.BECMBatteryVoltage / 10;  // We use the value from the CAN stream instead now
  }*/

  if (datalayer_extended.GeelySEA.soc_bms > 0) {
    datalayer.battery.status.real_soc = datalayer_extended.GeelySEA.soc_bms;
  }

  if (datalayer_extended.GeelySEA.soh_bms > 0) {
    datalayer.battery.status.soh_pptt = datalayer_extended.GeelySEA.soh_bms;
  }

  if (datalayer_extended.GeelySEA.CellTempHighest > 0) {
    datalayer.battery.status.temperature_max_dC = ((datalayer_extended.GeelySEA.CellTempHighest / 100.0) - 50.0) * 10;
  }

  if (datalayer_extended.GeelySEA.CellTempLowest > 0) {
    datalayer.battery.status.temperature_min_dC = ((datalayer_extended.GeelySEA.CellTempLowest / 100.0) - 50.0) * 10;
  }

  /*
  datalayer.battery.status.current_dA;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;
  */

  if (datalayer.battery.info.chemistry == LFP) {  //If configured LFP in use (or we autodetected it), switch to it
    datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_LFP_WH;  //EX30 51kWh LFP battery
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_LFP_120S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_LFP_120S_DV;
  } else if (datalayer.battery.info.chemistry ==
             NMC) {  //If configured NCM in use (or we autodetected it), switch to it
    if (datalayer.battery.info.number_of_cells == 107) {  //EX30 69kWh battery
      datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_NCM_107S_WH;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_NCM_107S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_NCM_107S_DV;
    } else if (datalayer.battery.info.number_of_cells == 110) {  //Zeekr 100kWh battery
      datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_NCM_110S_WH;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_NCM_110S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_NCM_110S_DV;
    }
  }

  /* Check safeties */
  if (datalayer_extended.GeelySEA.BECMsupplyVoltage > 0) {
    if (datalayer_extended.GeelySEA.BECMsupplyVoltage < 11800) {  // 11.8 V
      set_event(EVENT_12V_LOW, 0);
    } else {
      clear_event(EVENT_12V_LOW);
    }
  }
}

void GeelySeaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x53:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x140:  //20ms EX30 only 00 00 00 00 01 7F F9 03
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x141:  //20ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame0 counter, 0x00 -> 0x0D and repeating
      //frame1 CRC most likely
      break;
    case 0x142:  //20ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == 0x00) {
        datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 3;
      } else if (rx_frame.data.u8[0] == 0x01) {
        datalayer.battery.status.cell_max_voltage_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 3;
      } else if (rx_frame.data.u8[0] == 0x02) {
        datalayer.battery.status.cell_min_voltage_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 3;
      } else if (rx_frame.data.u8[0] > 0x02) {
        datalayer.battery.status.cell_voltages_mV[rx_frame.data.u8[2] - 1] =
            ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 3;
        if (rx_frame.data.u8[2] > datalayer.battery.info.number_of_cells)  // Detect number of cells
        {
          datalayer.battery.info.number_of_cells = rx_frame.data.u8[2];
        }
      }
      break;
    case 0x143:  //20ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x145:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x175:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x17B:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x178:  //70ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x182:  //100ms Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x183:  //100ms EX30
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18A:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x286:  //120ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x288:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x290:  //100ms EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x293:  //EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x295:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x296:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x298:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x301:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x302:  //EX30
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x309:  //EX30
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x313:  //EX30
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x315:  //EX30+Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x316:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x331:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x341:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x342:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x345:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x346:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x347:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x478:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x479:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x489:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x493:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x494:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x499:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x49C:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x519:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C0:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C1:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5D0:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5D1:  //Zeekr
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x635:  //Diag
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0xEE) &&
          (rx_frame.data.u8[3] == 0x02))  // BECM module voltage supply
      {
        datalayer_extended.GeelySEA.BECMsupplyVoltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        transmit_can_frame(&SEA_HV_Voltage_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x48) &&
                 (rx_frame.data.u8[3] == 0x03))  // High voltage battery voltage response frame
      {
        datalayer_extended.GeelySEA.BECMBatteryVoltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        transmit_can_frame(&SEA_SOC_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x48) &&
                 (rx_frame.data.u8[3] == 0x01))  // SOC response frame
      {
        datalayer_extended.GeelySEA.soc_bms = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 5;
        transmit_can_frame(&SEA_SOH_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x48) &&
                 (rx_frame.data.u8[3] == 0x9A))  // SOH response frame
      {
        datalayer_extended.GeelySEA.soh_bms = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        transmit_can_frame(&SEA_HighestCellTemp_Req);
      } else if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0x45))  // Highest cell temp response frame
      {
        datalayer_extended.GeelySEA.CellTempHighest = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        transmit_can_frame(&SEA_AverageCellTemp_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0x1B))  // Average cell temp response frame
      {
        datalayer_extended.GeelySEA.CellTempAverage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        transmit_can_frame(&SEA_LowestCellTemp_Req);
      } else if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0xA1))  // Lowest cell temp response frame
      {
        datalayer_extended.GeelySEA.CellTempLowest = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        transmit_can_frame(&SEA_Interlock_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0x1A))  // Interlock response frame
      {
        datalayer_extended.GeelySEA.Interlock = rx_frame.data.u8[4];
        transmit_can_frame(&SEA_HighestCellVolt_Req);
      } else if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0x07))  // Highest cell volt response frame
      {
        datalayer_extended.GeelySEA.CellVoltHighest = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        transmit_can_frame(&SEA_LowestCellVolt_Req);
      } else if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
                 (rx_frame.data.u8[3] == 0x08))  // Lowest cell volt response frame
      {
        datalayer_extended.GeelySEA.CellVoltLowest = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        transmit_can_frame(&SEA_BatteryCurrent_Req);
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x48) &&
                 (rx_frame.data.u8[3] == 0x02))  // Battery current response frame
      {
        datalayer_extended.GeelySEA.BatteryCurrent = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        transmit_can_frame(&SEA_DTC_Req);
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[2] == 0x59) &&
                 (rx_frame.data.u8[3] == 0x03))  // First response frame for DTC with more than one code
      {
        datalayer_extended.GeelySEA.DTCcount = ((rx_frame.data.u8[1] - 2) / 4);
        transmit_can_frame(&SEA_Flowcontrol);  // Send flow control
      } else if ((rx_frame.data.u8[1] == 0x59) &&
                 (rx_frame.data.u8[2] == 0x03))  // Response frame for DTC with 0 or 1 code
      {
        if (rx_frame.data.u8[0] != 0x02) {
          datalayer_extended.GeelySEA.DTCcount = 1;
        } else {
          datalayer_extended.GeelySEA.DTCcount = 0;
        }
      }

      break;
    default:
      break;
  }
}

void GeelySeaBattery::readDiagData() {
  transmit_can_frame(&SEA_BECMsupplyVoltage_Req);
}

void GeelySeaBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&SEA_536);  //Send 0x536 Network managing frame to keep BMS alive
  }
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;
    if (!startedUp) {
      readDiagData();
      startedUp = true;
    }
  }
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    readDiagData();
  }
}

void GeelySeaBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_NCM_110S_WH;          //Startout in NCM mode
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_NCM_110S_DV;  //Startout with max allowed range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_NCM_110S_DV;  //Startout with min allowed range
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
