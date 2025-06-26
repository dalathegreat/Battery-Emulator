#include "VOLVO-SPA-HYBRID-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "../include.h"

void VolvoSpaHybridBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter
  uint8_t cnt = 0;

  // Update webserver datalayer
  datalayer_extended.VolvoHybrid.soc_bms = SOC_BMS;
  datalayer_extended.VolvoHybrid.soc_calc = SOC_CALC;
  datalayer_extended.VolvoHybrid.soc_rescaled = datalayer.battery.status.reported_soc;
  datalayer_extended.VolvoHybrid.soh_bms = datalayer.battery.status.soh_pptt;

  datalayer_extended.VolvoHybrid.BECMBatteryVoltage = BATT_U;
  datalayer_extended.VolvoHybrid.BECMBatteryCurrent = BATT_I;
  datalayer_extended.VolvoHybrid.BECMUDynMaxLim = MAX_U;
  datalayer_extended.VolvoHybrid.BECMUDynMinLim = MIN_U;

  datalayer_extended.VolvoHybrid.HvBattPwrLimDcha1 = HvBattPwrLimDcha1;
  datalayer_extended.VolvoHybrid.HvBattPwrLimDchaSoft = HvBattPwrLimDchaSoft;
  //datalayer_extended.VolvoHybrid.HvBattPwrLimDchaSlowAgi = HvBattPwrLimDchaSlowAgi;
  //datalayer_extended.VolvoHybrid.HvBattPwrLimChrgSlowAgi = HvBattPwrLimChrgSlowAgi;

  // Update requests from webserver datalayer
  if (datalayer_extended.VolvoHybrid.UserRequestDTCreset) {
    transmit_can_frame(&VOLVO_DTC_Erase, can_config.battery);  //Send global DTC erase command
    datalayer_extended.VolvoHybrid.UserRequestDTCreset = false;
  }
  if (datalayer_extended.VolvoHybrid.UserRequestBECMecuReset) {
    transmit_can_frame(&VOLVO_BECM_ECUreset, can_config.battery);  //Send BECM ecu reset command
    datalayer_extended.VolvoHybrid.UserRequestBECMecuReset = false;
  }
  if (datalayer_extended.VolvoHybrid.UserRequestDTCreadout) {
    transmit_can_frame(&VOLVO_DTCreadout, can_config.battery);  //Send DTC readout command
    datalayer_extended.VolvoHybrid.UserRequestDTCreadout = false;
  }

  remaining_capacity = (18830 - CHARGE_ENERGY);

  //datalayer.battery.status.real_soc = SOC_BMS;			// Use BMS reported SOC, havent figured out how to get the BMS to calibrate empty/full yet
  SOC_CALC = remaining_capacity / 19;  // Use calculated SOC based on remaining_capacity

  datalayer.battery.status.real_soc = SOC_CALC * 10;

  if (BATT_U > MAX_U)  // Protect if overcharged
  {
    datalayer.battery.status.real_soc = 10000;
  } else if (BATT_U < MIN_U)  //Protect if undercharged
  {
    datalayer.battery.status.real_soc = 0;
  }

  datalayer.battery.status.voltage_dV = BATT_U * 10;
  datalayer.battery.status.current_dA = BATT_I * 10;
  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity;

  datalayer.battery.status.max_discharge_power_W = 6600;  //default power on charge connector
  datalayer.battery.status.max_charge_power_W = 6600;     //default power on charge connector
  datalayer.battery.status.temperature_min_dC = BATT_T_MIN;
  datalayer.battery.status.temperature_max_dC = BATT_T_MAX;

  datalayer.battery.status.cell_max_voltage_mV = CELL_U_MAX;  // Use min/max reported from BMS
  datalayer.battery.status.cell_min_voltage_mV = CELL_U_MIN;

  //Map all cell voltages to the global array
  for (int i = 0; i < 102; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = cell_voltages[i];
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
  logging.print("Remaining Energy: ");
  logging.println(remaining_capacity);
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
  logging.println(CELL_U_MAX);
  logging.print("BMS Lowest cell voltage: ");
  logging.println(CELL_U_MIN);
  logging.print("BMS Highest cell nr: ");
  logging.println(CELL_ID_U_MAX);
  logging.print("Highest cell voltage: ");
  logging.println(min_max_voltage[1]);
  logging.print("Lowest cell voltage: ");
  logging.println(min_max_voltage[0]);
  logging.print("Cell voltage,");
  while (cnt < 102) {
    logging.print(cell_voltages[cnt++]);
    logging.print(",");
  }
  logging.println(";");
#endif
}

void VolvoSpaHybridBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
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
        datalayer_extended.VolvoHybrid.HVSysRlySts = ((rx_frame.data.u8[0] & 0x30) >> 4);
      else
        datalayer_extended.VolvoHybrid.HVSysRlySts = 0xFF;

      if ((rx_frame.data.u8[2] & 0x40) == 0x40)
        datalayer_extended.VolvoHybrid.HVSysDCRlySts1 = ((rx_frame.data.u8[2] & 0x30) >> 4);
      else
        datalayer_extended.VolvoHybrid.HVSysDCRlySts1 = 0xFF;
      if ((rx_frame.data.u8[2] & 0x80) == 0x80)
        datalayer_extended.VolvoHybrid.HVSysDCRlySts2 = ((rx_frame.data.u8[4] & 0x30) >> 4);
      else
        datalayer_extended.VolvoHybrid.HVSysDCRlySts2 = 0xFF;
      if ((rx_frame.data.u8[0] & 0x80) == 0x80)
        datalayer_extended.VolvoHybrid.HVSysIsoRMonrSts = ((rx_frame.data.u8[4] & 0xC0) >> 6);
      else
        datalayer_extended.VolvoHybrid.HVSysIsoRMonrSts = 0xFF;

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
        //HvBattPwrLimDchaSlowAgi = (((rx_frame.data.u8[4] & 0x07) * 256 + rx_frame.data.u8[5]) >> 2);
        ;
      } else {
        //HvBattPwrLimDchaSlowAgi = 0;
        ;
      }
      if ((rx_frame.data.u8[2] & 0x08) == 0x08) {
        //HvBattPwrLimChrgSlowAgi = (((rx_frame.data.u8[2] & 0x07) * 256 + rx_frame.data.u8[3]) >> 2);
        ;
      } else {
        //HvBattPwrLimChrgSlowAgi = 0;
        ;
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
        //CELL_U_MAX = ((rx_frame.data.u8[2] & 0x01) * 256 + rx_frame.data.u8[3]);
        ;
      else {
        //CELL_U_MAX = 0;
        ;
#ifdef DEBUG_LOG
        logging.println("CELL_U_MAX not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x02) == 0x02)
        //CELL_U_MIN = ((rx_frame.data.u8[0] & 0x01) * 256.0 + rx_frame.data.u8[1]);
        ;
      else {
        //CELL_U_MIN = 0;
        ;
#ifdef DEBUG_LOG
        logging.println("CELL_U_MIN not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08)
        //CELL_ID_U_MAX = ((rx_frame.data.u8[4] & 0x01) * 256.0 + rx_frame.data.u8[5]);
        ;
      else {
        //CELL_ID_U_MAX = 0;
        ;
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
        transmit_can_frame(&VOLVO_BECMsupplyVoltage_Req, can_config.battery);  //Send BECM supply voltage req
      } else if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0xF4) &&
                 (rx_frame.data.u8[3] == 0x42))  // BECM module voltage supply
      {
        datalayer_extended.VolvoHybrid.BECMsupplyVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[1] == 0xCF) && (rx_frame.data.u8[2] == 0x62) &&
                 (rx_frame.data.u8[3] == 0x48) &&
                 (rx_frame.data.u8[4] == 0x06))  // First response frame of cell voltages //changed
      {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
        rxConsecutiveFrames = 1;
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[2] == 0x59) &&
                 (rx_frame.data.u8[3] == 0x03))  // First response frame for DTC with more than one code
      {
        transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x21) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x22) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x23) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x24) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x25) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x26) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x27) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x28) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x29) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2A) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2B) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2C) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2D) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2E) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2F) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
        rxConsecutiveFrames = 2;
      } else if ((rx_frame.data.u8[0] == 0x20) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x21) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x22) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x23) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x24) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x25) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x26) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x27) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x28) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x29) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2A) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2B) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2C) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
      } else if ((rx_frame.data.u8[0] == 0x2D) && (rxConsecutiveFrames == 2)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        //cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control
        //transmit_can_frame(&VOLVO_FlowControl, can_config.battery);  // Send flow control

        if (false)  // Run until last pack is read
        {
          //VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
          //transmit_can_frame(&VOLVO_CELL_U_Req, can_config.battery);  //Send cell voltage read request for next module
          ;
        } else {
          min_max_voltage[0] = 9999;
          min_max_voltage[1] = 0;
          for (cellcounter = 0; cellcounter < 102; cellcounter++) {
            if (min_max_voltage[0] > cell_voltages[cellcounter])
              min_max_voltage[0] = cell_voltages[cellcounter];
            if (min_max_voltage[1] < cell_voltages[cellcounter]) {
              min_max_voltage[1] = cell_voltages[cellcounter];
              CELL_ID_U_MAX = cellcounter;
            }
          }
          CELL_U_MAX = min_max_voltage[1];
          CELL_U_MIN = min_max_voltage[0];

          transmit_can_frame(&VOLVO_SOH_Req, can_config.battery);  //Send SOH read request
        }
        rxConsecutiveFrames = 0;
      }
      break;
    default:
      break;
  }
}

void VolvoSpaHybridBattery::readCellVoltages() {
  battery_request_idx = 0;
  //batteryModuleNumber = 0x10;
  rxConsecutiveFrames = 0;
  //VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
  transmit_can_frame(&VOLVO_CELL_U_Req, can_config.battery);  //Send cell voltage read request for first module
}

void VolvoSpaHybridBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&VOLVO_536, can_config.battery);  //Send 0x536 Network managing frame to keep BMS alive
    transmit_can_frame(&VOLVO_372, can_config.battery);  //Send 0x372 ECMAmbientTempCalculated

    if ((datalayer.battery.status.bms_status == ACTIVE) && startedUp) {
      datalayer.system.status.battery_allows_contactor_closing = true;
      //transmit_can_frame(&VOLVO_140_CLOSE, can_config.battery);  //Send 0x140 Close contactors message
    } else {  //datalayer.battery.status.bms_status == FAULT , OR inverter requested opening contactors, OR system not started yet
      datalayer.system.status.battery_allows_contactor_closing = false;
      transmit_can_frame(&VOLVO_140_OPEN, can_config.battery);  //Send 0x140 Open contactors message
    }
  }
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    if (!startedUp) {
      transmit_can_frame(&VOLVO_DTC_Erase, can_config.battery);  //Erase any DTCs preventing startup
      DTC_reset_counter++;
      if (DTC_reset_counter > 1) {  // Performed twice before starting
        startedUp = true;
      }
    }
  }
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;
    if (true) {
      readCellVoltages();
#ifdef DEBUG_LOG
      logging.println("Requesting cell voltages");
#endif
    }
  }
}

void VolvoSpaHybridBattery::setup(void) {                     // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);  //changed
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 102;  //was 108, changed
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
