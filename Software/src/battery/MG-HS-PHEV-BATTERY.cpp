#include "../include.h"
#ifdef MG_HS_PHEV_BATTERY_H
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MG-HS-PHEV-BATTERY.h"

/* TODO:
- Get contactor closing working
- Figure out which CAN messages need to be sent towards the battery to keep it alive
- Map all values from battery CAN messages
- Note: Charge power/discharge power is estimated for now

# row3 pin2 needs strobing to 12V (via a 1k) to wake up the BMU
# but contactor won't come on until deasserted
# BMU goes to sleep after after ~18s of no CAN
*/

void MgHsPHEVBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;
}

void MgHsPHEVBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x171:  //Following messages were detected on a MG5 battery BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x172:
      break;
    case 0x173:
      break;
    case 0x293:
      break;
    case 0x295:
      break;
    case 0x297:
      break;
    case 0x29B:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x29C:
      break;
    case 0x2A0:
      break;
    case 0x2A2:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x322:
      break;
    case 0x334:
      break;
    case 0x33F:
      break;
    case 0x391:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x393:
      break;
    case 0x3AB:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3AC:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3B8:
      break;
    case 0x3BA:
      break;
    case 0x3BC:
      break;
    case 0x3BE:
      break;
    case 0x3C0:
      break;
    case 0x3C2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x400:
      break;
    case 0x402:
      break;
    case 0x418:
      break;
    case 0x44C:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x620:
      break;     //This is the last on the list in the MG5 template.
    case 0x3a8:  //This ID is on the MG HS RX WITHOUT ANY TX PRESENT
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x508:  //This ID is a new one MG HS RX WHEN TRANSMITTING 03 22 B0 41 00 00 00 00. Rx data is 00 00 00 00 00 00 00 00
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7ED:  //This ID is the battery BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Process rx data for incoming responses to "Read data by ID" Tx
      if (rx_frame.data.u8[1] == 0x62) {
        if (rx_frame.data.u8[2] == 0xB0) {                                   //Battery information
          if (rx_frame.data.u8[3] == 0x41 && rx_frame.data.u8[0] == 0x05) {  // Battery bus voltage
            //				Serial.print ("Battery Bus voltage frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 2.5;
            //				Serial.print ("Battery Bus Voltage = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }
          if (rx_frame.data.u8[3] == 0x42 && rx_frame.data.u8[0] == 0x05) {  // Battery voltage
            //				Serial.print ("Battery voltage frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.voltage_dV = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 2.5;
            //				Serial.print ("Battery voltage = ");
            //				Serial.println (datalayer.battery.status.voltage_dV);
          }
          if (rx_frame.data.u8[3] == 0x43 && rx_frame.data.u8[0] == 0x05) {  // Battery current
            //				Serial.print ("Battery current frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.current_dA = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) - 40000) / -4;
            //				Serial.print ("Battery current = ");
            //				Serial.println (datalayer.battery.status.current_dA);
          }

          if (rx_frame.data.u8[3] == 0x45 && rx_frame.data.u8[0] == 0x05) {  // Battery resistance
            //				Serial.print ("Battery resistance frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
            //				Serial.print ("Battery resistance = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }
          if (rx_frame.data.u8[3] == 0x46 && rx_frame.data.u8[0] == 0x05) {  // Battery SoC
            //				Serial.print ("Battery SoC frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.real_soc = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 10;
            //				Serial.print ("Battery SoC = ");
            //				Serial.println (datalayer.battery.status.real_soc);
            RealSoC = datalayer.battery.status.real_soc / 100;  // For calculation of charge and discharge rates
          }

          if (rx_frame.data.u8[3] == 0x47) {  //    && rx_frame.data.u8[0] == 0x05) {	   // BMS error code
                                              //				Serial.print ("BMS error code frame = ");
                                              //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            //				Serial.print ("Battery error = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }

          if (rx_frame.data.u8[3] == 0x48) {  //    && rx_frame.data.u8[0] == 0x05) {	   // BMS status code
                                              //				Serial.print ("BMS status code frame = ");
                                              //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            //				Serial.print ("Battery Status = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }

          if (rx_frame.data.u8[3] == 0x49) {  //    && rx_frame.data.u8[0] == 0x05) {	   // System main relay B status
                                              //				Serial.print ("System main relay B status frame = ");
                                              //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            //				Serial.print ("Main relay B status = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }

          if (rx_frame.data.u8[3] == 0x4A) {  //    && rx_frame.data.u8[0] == 0x05) {	   // System main relay G status
                                              //				Serial.print ("System main relay G status frame = ");
                                              //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            //				Serial.print ("Main relay G status = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }

          if (rx_frame.data.u8[3] == 0x52) {  //    && rx_frame.data.u8[0] == 0x05) {	   // System main relay P status
                                              //				Serial.print ("System main relay P status frame = ");
                                              //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            //				datalayer.battery.status.PARAMETER = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            //				Serial.print ("BMain relay P status = ");
            //				Serial.println (datalayer.battery.status.PARAMETER);
          }

          if (rx_frame.data.u8[3] == 0x56 && rx_frame.data.u8[0] == 0x05) {  // Max cell temperature
            //				Serial.print ("Max cell temperature frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.temperature_max_dC =
                (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
            //				Serial.print ("Max cell temperature = ");
            //				Serial.println (datalayer.battery.status.temperature_max_dC);
          }

          if (rx_frame.data.u8[3] == 0x57 && rx_frame.data.u8[0] == 0x05) {  // Min cell temperature
            //				Serial.print ("Min cell temperature frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.temperature_min_dC =
                (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
            //				Serial.print ("Min cell temperature = ");
            //				Serial.println (datalayer.battery.status.temperature_min_dC);
          }

          if (rx_frame.data.u8[3] == 0x58 && rx_frame.data.u8[0] == 0x06) {  // Max cell voltage
            //				Serial.print ("Max cell voltage frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.cell_max_voltage_mV =
                (rx_frame.data.u8[4] << 16 | rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 250;
            //				Serial.print ("Max cell voltage = ");
            //				Serial.println (datalayer.battery.status.cell_max_voltage_mV);
          }

          if (rx_frame.data.u8[3] == 0x59 && rx_frame.data.u8[0] == 0x06) {  // Min cell voltage
            //				Serial.print ("Min cell voltage frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.cell_min_voltage_mV =
                (rx_frame.data.u8[4] << 16 | rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 250;
            //				Serial.print ("Min cell voltage = ");
            //				Serial.println (datalayer.battery.status.cell_min_voltage_mV);
          }

          if (rx_frame.data.u8[3] == 0x61 && rx_frame.data.u8[0] == 0x05) {  // Battery SoH
            //				Serial.print ("Battery SoH frame = ");
            //				print_can_frame_MG5(rx_frame, frameDirection(MSG_RX));
            datalayer.battery.status.soh_pptt = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
            //				Serial.print ("Battery SoH = ");
            //				Serial.println (datalayer.battery.status.soh_pptt);
          }

        }  // data.u8[2]=0xB0
      }  // data.u8[1] = 0x62)

      //Set calculated and derived parameters

      // Calculate the remaining capacity.
      tempfloat = datalayer.battery.info.total_capacity_Wh * (RealSoC - MinSoC) / 100;
      //	Serial.print ("Remaining capacity calculated = ");
      //	Serial.println (tempfloat);
      if (tempfloat > 0) {
        datalayer.battery.status.remaining_capacity_Wh = tempfloat;
      } else {
        datalayer.battery.status.remaining_capacity_Wh = 0;
      }

      // Calculate the maximum charge power. Taper the charge power between 90% and 100% SoC, as 100% SoC is approached
      if (RealSoC < StartChargeTaper) {
        datalayer.battery.status.max_charge_power_W = MaxChargePower;
      } else if (RealSoC >= 100) {
        datalayer.battery.status.max_charge_power_W = TricklePower;
      } else {
        //Taper the charge to the Trickle value. The shape and start point of the taper is set by the constants
        datalayer.battery.status.max_charge_power_W =
            (MaxChargePower * pow(((100 - RealSoC) / (100 - StartChargeTaper)), ChargeTaperExponent)) + TricklePower;
      }

      // Calculate the maximum discharge power. Taper the discharge power between 35% and Min% SoC, as Min% SoC is approached
      if (RealSoC > StartDischargeTaper) {
        datalayer.battery.status.max_discharge_power_W = MaxDischargePower;
      } else if (RealSoC < MinSoC) {
        datalayer.battery.status.max_discharge_power_W = TricklePower;
      } else {
        //Taper the charge to the Trickle value. The shape and start point of the taper is set by the constants
        datalayer.battery.status.max_discharge_power_W =
            (MaxDischargePower * pow(((RealSoC - MinSoC) / (StartDischargeTaper - MinSoC)), DischargeTaperExponent)) +
            TricklePower;
      }

      break;
    default:
      break;
  }
}
void MgHsPHEVBattery::transmit_can(unsigned long currentMillis) {
  // Send 70ms CAN Message
  if (currentMillis - previousMillis70 >= INTERVAL_70_MS) {
    previousMillis70 = currentMillis;

    if (datalayer.battery.status.bms_status == FAULT) {
      //Open contactors!
      MG_HS_8A.data.u8[5] = 0x00;
    } else {  // Not in faulted mode, Close contactors!
      MG_HS_8A.data.u8[5] = 0x02;
    }

    transmit_can_frame(&MG_HS_8A, can_config.battery);
    transmit_can_frame(&MG_HS_1F1, can_config.battery);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (messageindex) {
      case 1:
        transmit_can_frame(&MG_HS_7E5_B0_42, can_config.battery);  //Battery voltage
        break;
      case 2:
        transmit_can_frame(&MG_HS_7E5_B0_43, can_config.battery);  //Battery current
        break;
      case 3:
        transmit_can_frame(&MG_HS_7E5_B0_46, can_config.battery);  //Battery SoC
        break;
      case 4:
        transmit_can_frame(&MG_HS_7E5_B0_47, can_config.battery);  // Get BMS error code
        break;
      case 5:
        transmit_can_frame(&MG_HS_7E5_B0_48, can_config.battery);  // Get BMS status
        break;
      case 6:
        transmit_can_frame(&MG_HS_7E5_B0_49, can_config.battery);  // Get System main relay B status
        break;
      case 7:
        transmit_can_frame(&MG_HS_7E5_B0_4A, can_config.battery);  // Get System main relay G status
        break;
      case 8:
        transmit_can_frame(&MG_HS_7E5_B0_52, can_config.battery);  // Get System main relay P status
        break;
      case 9:
        transmit_can_frame(&MG_HS_7E5_B0_56, can_config.battery);  //Max cell temperature
        break;
      case 10:
        transmit_can_frame(&MG_HS_7E5_B0_57, can_config.battery);  //Min cell temperature
        break;
      case 11:
        transmit_can_frame(&MG_HS_7E5_B0_58, can_config.battery);  //Max cell voltage
        break;
      case 12:
        transmit_can_frame(&MG_HS_7E5_B0_59, can_config.battery);  //Min cell voltage
        break;
      case 13:
        transmit_can_frame(&MG_HS_7E5_B0_61, can_config.battery);  //Battery SoH
        messageindex = 0;  //Return to the first message index. This goes in the last message entry
        break;
      default:
        break;

    }  //switch

    messageindex++;  //Increment the message index

  }  //endif
}

void MgHsPHEVBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}

#endif
