#include "VOLVO-SPA-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void VolvoSpaBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter
  uint8_t cnt = 0;

  // Update webserver datalayer
  datalayer_extended.VolvoPolestar.soc_bms = SOC_BMS;
  datalayer_extended.VolvoPolestar.soc_calc = SOC_CALC;
  datalayer_extended.VolvoPolestar.soc_rescaled = datalayer.battery.status.reported_soc;
  datalayer_extended.VolvoPolestar.soh_bms = datalayer.battery.status.soh_pptt;

  datalayer_extended.VolvoPolestar.BECMBatteryVoltage = BATT_U;
  datalayer_extended.VolvoPolestar.BECMBatteryCurrent = BATT_I;
  datalayer_extended.VolvoPolestar.BECMUDynMaxLim = MAX_U;
  datalayer_extended.VolvoPolestar.BECMUDynMinLim = MIN_U;

  datalayer_extended.VolvoPolestar.HvBattPwrLimDcha1 = HvBattPwrLimDcha1;
  datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSoft = HvBattPwrLimDchaSoft;
  datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSlowAgi = HvBattPwrLimDchaSlowAgi;
  datalayer_extended.VolvoPolestar.HvBattPwrLimChrgSlowAgi = HvBattPwrLimChrgSlowAgi;

  // Update requests from webserver datalayer
  if (datalayer_extended.VolvoPolestar.UserRequestDTCreset) {
    transmit_can_frame(&VOLVO_DTC_Erase);  //Send global DTC erase command
    datalayer_extended.VolvoPolestar.UserRequestDTCreset = false;
  }
  if (datalayer_extended.VolvoPolestar.UserRequestBECMecuReset) {
    transmit_can_frame(&VOLVO_BECM_ECUreset);  //Send BECM ecu reset command
    datalayer_extended.VolvoPolestar.UserRequestBECMecuReset = false;
  }
  if (datalayer_extended.VolvoPolestar.UserRequestDTCreadout) {
    transmit_can_frame(&VOLVO_DTCreadout);  //Send DTC readout command
    datalayer_extended.VolvoPolestar.UserRequestDTCreadout = false;
  }

  datalayer.battery.status.remaining_capacity_Wh = (datalayer.battery.info.total_capacity_Wh - CHARGE_ENERGY);

  //datalayer.battery.status.real_soc = SOC_BMS;			// Use BMS reported SOC, havent figured out how to get the BMS to calibrate empty/full yet
  // Use calculated SOC based on remaining_capacity
  SOC_CALC = (datalayer.battery.status.remaining_capacity_Wh / (datalayer.battery.info.total_capacity_Wh / 1000));

  datalayer.battery.status.real_soc = SOC_CALC * 10;  //Add one decimal to make it pptt

  if (BATT_U > MAX_U)  // Protect if overcharged
  {
    datalayer.battery.status.real_soc = 10000;
  } else if (BATT_U < MIN_U)  //Protect if undercharged
  {
    datalayer.battery.status.real_soc = 0;
  }

  datalayer.battery.status.voltage_dV = BATT_U * 10;
  datalayer.battery.status.current_dA = BATT_I * 10;

  datalayer.battery.status.max_discharge_power_W = HvBattPwrLimDchaSlowAgi * 1000;  //kW to W
  datalayer.battery.status.max_charge_power_W = HvBattPwrLimChrgSlowAgi * 1000;     //kW to W
  datalayer.battery.status.temperature_min_dC = BATT_T_MIN;
  datalayer.battery.status.temperature_max_dC = BATT_T_MAX;

  datalayer.battery.status.cell_max_voltage_mV = CELL_U_MAX * 10;  // Use min/max reported from BMS
  datalayer.battery.status.cell_min_voltage_mV = CELL_U_MIN * 10;

  //Map all cell voltages to the global array
  for (int i = 0; i < 108; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = cell_voltages[i];
  }

  //If we have enough cell values populated (atleast 96 read) AND number_of_cells not initialized yet
  if (cell_voltages[95] > 0 && datalayer.battery.info.number_of_cells == 0) {
    // We can determine whether we have 96S or 108S battery
    if (datalayer.battery.status.cell_voltages_mV[107] > 0) {
      datalayer.battery.info.number_of_cells = 108;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
      datalayer.battery.info.total_capacity_Wh = 78200;
    } else {
      datalayer.battery.info.number_of_cells = 96;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
      datalayer.battery.info.total_capacity_Wh = 69511;
    }
  }

#ifdef DEBUG_LOG
  logging.print("BMS reported SOC%: ");
  logging.println(SOC_BMS);
  logging.print("Calculated SOC%: ");
  logging.println(SOC_CALC);
  logging.print("Rescaled SOC%: ");
  logging.println(datalayer.battery.status.reported_soc / 100);
  logging.print("Battery current: ");
  logging.println(BATT_I);
  logging.print("Battery voltage: ");
  logging.println(BATT_U);
  logging.print("Battery maximum voltage limit: ");
  logging.println(MAX_U);
  logging.print("Battery minimum voltage limit: ");
  logging.println(MIN_U);
  logging.print("Discharge limit: ");
  logging.println(HvBattPwrLimDchaSoft);
  logging.print("Battery Error Indication: ");
  logging.println(BATT_ERR_INDICATION);
  logging.print("Maximum battery temperature: ");
  logging.println(BATT_T_MAX / 10);
  logging.print("Minimum battery temperature: ");
  logging.println(BATT_T_MIN / 10);
  logging.print("Average battery temperature: ");
  logging.println(BATT_T_AVG / 10);
  logging.print("BMS Highest cell voltage: ");
  logging.println(CELL_U_MAX * 10);
  logging.print("BMS Lowest cell voltage: ");
  logging.println(CELL_U_MIN * 10);
  logging.print("BMS Highest cell nr: ");
  logging.println(CELL_ID_U_MAX);
  logging.print("Highest cell voltage: ");
  logging.println(min_max_voltage[1]);
  logging.print("Lowest cell voltage: ");
  logging.println(min_max_voltage[0]);
  logging.print("Cell voltage,");
  while (cnt < 108) {
    logging.print(cell_voltages[cnt++]);
    logging.print(",");
  }
  logging.println(";");
#endif
}

void VolvoSpaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x3A:
      if ((rx_frame.data.u8[6] & 0x80) == 0x80)
        BATT_I = (0 - ((((rx_frame.data.u8[6] & 0x7F) * 256.0 + rx_frame.data.u8[7]) * 0.1) - 1638));
      else {
        BATT_I = 0;
#ifdef DEBUG_LOG
        logging.println("BATT_I not valid");
#endif
      }

      if ((rx_frame.data.u8[2] & 0x08) == 0x08)
        MAX_U = (((rx_frame.data.u8[2] & 0x07) * 256.0 + rx_frame.data.u8[3]) * 0.25);
      else {
        //MAX_U = 0;
        //logging.println("MAX_U not valid");	// Value toggles between true/false from BMS
      }

      if ((rx_frame.data.u8[4] & 0x08) == 0x08)
        MIN_U = (((rx_frame.data.u8[4] & 0x07) * 256.0 + rx_frame.data.u8[5]) * 0.25);
      else {
        //MIN_U = 0;
        //logging.println("MIN_U not valid");	// Value toggles between true/false from BMS
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08)
        BATT_U = (((rx_frame.data.u8[0] & 0x07) * 256.0 + rx_frame.data.u8[1]) * 0.25);
      else {
        BATT_U = 0;
#ifdef DEBUG_LOG
        logging.println("BATT_U not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x40) == 0x40)
        datalayer_extended.VolvoPolestar.HVSysRlySts = ((rx_frame.data.u8[0] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysRlySts = 0xFF;

      if ((rx_frame.data.u8[2] & 0x40) == 0x40)
        datalayer_extended.VolvoPolestar.HVSysDCRlySts1 = ((rx_frame.data.u8[2] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysDCRlySts1 = 0xFF;
      if ((rx_frame.data.u8[2] & 0x80) == 0x80)
        datalayer_extended.VolvoPolestar.HVSysDCRlySts2 = ((rx_frame.data.u8[4] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysDCRlySts2 = 0xFF;
      if ((rx_frame.data.u8[0] & 0x80) == 0x80)
        datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts = ((rx_frame.data.u8[4] & 0xC0) >> 6);
      else
        datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts = 0xFF;

      break;
    case 0x1A1:
      if ((rx_frame.data.u8[4] & 0x10) == 0x10)
        CHARGE_ENERGY = ((((rx_frame.data.u8[4] & 0x0F) * 256.0 + rx_frame.data.u8[5]) * 50) - 500);
      else {
        CHARGE_ENERGY = 0;
        set_event(EVENT_KWH_PLAUSIBILITY_ERROR, CHARGE_ENERGY);
      }
      break;
    case 0x413:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80)
        BATT_ERR_INDICATION = ((rx_frame.data.u8[0] & 0x40) >> 6);
      else {
        BATT_ERR_INDICATION = 0;
#ifdef DEBUG_LOG
        logging.println("BATT_ERR_INDICATION not valid");
#endif
      }
      if ((rx_frame.data.u8[0] & 0x20) == 0x20) {
        BATT_T_MAX = ((rx_frame.data.u8[2] & 0x1F) * 256.0 + rx_frame.data.u8[3]);
        BATT_T_MIN = ((rx_frame.data.u8[4] & 0x1F) * 256.0 + rx_frame.data.u8[5]);
        BATT_T_AVG = ((rx_frame.data.u8[0] & 0x1F) * 256.0 + rx_frame.data.u8[1]);
      } else {
        BATT_T_MAX = 0;
        BATT_T_MIN = 0;
        BATT_T_AVG = 0;
#ifdef DEBUG_LOG
        logging.println("BATT_T not valid");
#endif
      }
      break;
    case 0x369:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80) {
        HvBattPwrLimDchaSoft = (((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[6]) >> 2);
      } else {
        HvBattPwrLimDchaSoft = 0;
#ifdef DEBUG_LOG
        logging.println("HvBattPwrLimDchaSoft not valid");
#endif
      }
      break;
    case 0x175:
      if ((rx_frame.data.u8[4] & 0x80) == 0x80) {
        HvBattPwrLimDcha1 = (((rx_frame.data.u8[2] & 0x07) * 256 + rx_frame.data.u8[3]) >> 2);
      } else {
        HvBattPwrLimDcha1 = 0;
      }
      break;
    case 0x177:
      if ((rx_frame.data.u8[4] & 0x08) == 0x08) {
        HvBattPwrLimDchaSlowAgi = (((rx_frame.data.u8[4] & 0x07) * 256 + rx_frame.data.u8[5]) >> 2);
      } else {
        HvBattPwrLimDchaSlowAgi = 0;
      }
      if ((rx_frame.data.u8[2] & 0x08) == 0x08) {
        HvBattPwrLimChrgSlowAgi = (((rx_frame.data.u8[2] & 0x07) * 256 + rx_frame.data.u8[3]) >> 2);
      } else {
        HvBattPwrLimChrgSlowAgi = 0;
      }
      break;
    case 0x37D:
      if ((rx_frame.data.u8[0] & 0x40) == 0x40) {
        SOC_BMS = ((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[7]);
      } else {
        SOC_BMS = 0;
#ifdef DEBUG_LOG
        logging.println("SOC_BMS not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x04) == 0x04)
        CELL_U_MAX = ((rx_frame.data.u8[2] & 0x01) * 256 + rx_frame.data.u8[3]);
      else {
        CELL_U_MAX = 0;
#ifdef DEBUG_LOG
        logging.println("CELL_U_MAX not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x02) == 0x02)
        CELL_U_MIN = ((rx_frame.data.u8[0] & 0x01) * 256.0 + rx_frame.data.u8[1]);
      else {
        CELL_U_MIN = 0;
#ifdef DEBUG_LOG
        logging.println("CELL_U_MIN not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08)
        CELL_ID_U_MAX = ((rx_frame.data.u8[4] & 0x01) * 256.0 + rx_frame.data.u8[5]);
      else {
        CELL_ID_U_MAX = 0;
#ifdef DEBUG_LOG
        logging.println("CELL_ID_U_MAX not valid");
#endif
      }
      break;
    case 0x635:  // Diag request response
      if ((rx_frame.data.u8[0] == 0x07) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
          (rx_frame.data.u8[3] == 0x6D))  // SOH response frame
      {
        datalayer.battery.status.soh_pptt = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
        transmit_can_frame(&VOLVO_BECMsupplyVoltage_Req);  //Send BECM supply voltage req
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0xF4) &&
                 (rx_frame.data.u8[3] == 0x42))  // BECM module voltage supply
      {
        datalayer_extended.VolvoPolestar.BECMsupplyVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[1] == 0x0B) && (rx_frame.data.u8[2] == 0x62) &&
                 (rx_frame.data.u8[3] == 0x4B))  // First response frame of cell voltages
      {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        transmit_can_frame(&VOLVO_FlowControl);  // Send flow control
        rxConsecutiveFrames = 1;
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[2] == 0x59) &&
                 (rx_frame.data.u8[3] == 0x03))  // First response frame for DTC with more than one code
      {
        transmit_can_frame(&VOLVO_FlowControl);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x21) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];

        if (batteryModuleNumber <= 0x2A)  // Run until last pack is read
        {
          VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
          transmit_can_frame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for next module
        } else {
          min_max_voltage[0] = 9999;
          min_max_voltage[1] = 0;
          for (cellcounter = 0; cellcounter < 108; cellcounter++) {
            if (min_max_voltage[0] > cell_voltages[cellcounter])
              min_max_voltage[0] = cell_voltages[cellcounter];
            if (min_max_voltage[1] < cell_voltages[cellcounter])
              min_max_voltage[1] = cell_voltages[cellcounter];
          }
          transmit_can_frame(&VOLVO_SOH_Req);  //Send SOH read request
        }
        rxConsecutiveFrames = 0;
      }
      break;
    default:
      break;
  }
}

void VolvoSpaBattery::readCellVoltages() {
  battery_request_idx = 0;
  batteryModuleNumber = 0x10;
  rxConsecutiveFrames = 0;
  VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
  transmit_can_frame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for first module
}

void VolvoSpaBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&VOLVO_536);  //Send 0x536 Network managing frame to keep BMS alive
    transmit_can_frame(&VOLVO_372);  //Send 0x372 ECMAmbientTempCalculated

    if ((datalayer.battery.status.bms_status == ACTIVE) && startedUp) {
      datalayer.system.status.battery_allows_contactor_closing = true;
      transmit_can_frame(&VOLVO_140_CLOSE);  //Send 0x140 Close contactors message
    } else {  //datalayer.battery.status.bms_status == FAULT , OR inverter requested opening contactors, OR system not started yet
      datalayer.system.status.battery_allows_contactor_closing = false;
      transmit_can_frame(&VOLVO_140_OPEN);  //Send 0x140 Open contactors message
    }
  }
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    if (!startedUp) {
      transmit_can_frame(&VOLVO_DTC_Erase);  //Erase any DTCs preventing startup
      DTC_reset_counter++;
      if (DTC_reset_counter > 1) {  // Performed twice before starting
        startedUp = true;
      }
    }
  }
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;
    if (datalayer.battery.status.bms_status == ACTIVE) {
      readCellVoltages();
    }
  }
}

void VolvoSpaBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 0;        // Initializes when all cells have been read
  datalayer.battery.info.total_capacity_Wh = 78200;  //Startout in 78kWh mode (This value used for SOC calc)
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Startout with max allowed range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;   //Startout with min allowed range
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
