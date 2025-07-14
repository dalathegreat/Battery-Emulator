#include "../include.h"
#ifdef MG_HS_PHEV_BATTERY_H
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MG-HS-PHEV-BATTERY.h"

/*
Note:
- Charge power/discharge power is estimated for now
- The built-in SoC indicator tops out at 4.1V/cell. To use up to 4.2V/cell, you should define:
  #define MG_HS_PHEV_USE_FULL_CAPACITY true
  in USER_SETTINGS.h
*/

void MgHsPHEVBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second
  if(cellVoltageValidTime > 0) {
    cellVoltageValidTime--;
  }
}

void MgHsPHEVBattery::update_soc(uint16_t soc_times_ten) {
#if MG_HS_PHEV_USE_FULL_CAPACITY
  // The SoC hits 100% at 4.1V/cell. To get the full 4.2V/cell we need to use
  // voltage instead for the last bit.

  if(cellVoltageValidTime == 0) {
    // We don't have a recent cell max voltage reading, so can't do
    // voltage-based SoC.
  } else if(soc_times_ten > 900 && datalayer.battery.status.cell_max_voltage_mV < 4000) {
    // Something is wrong with our max cell voltage reading (it is too low), so
    // don't trust it - we'll just let the SoC hit 100%.
  } else if(soc_times_ten == 1000 && datalayer.battery.status.cell_max_voltage_mV >= 4100) {
    // We've hit 100%, so use voltage-based-SoC calculation for the last bit.

    // We usually hit 92% at ~369V, and the pack max is 378V.

    // Scale so that 100% becomes 92%
    soc_times_ten = (uint16_t)(((uint32_t)soc_times_ten * 9200) / 10000);

    // Add on the last 100mV as the last 8% of SoC.
    soc_times_ten += (uint16_t)((((uint32_t)datalayer.battery.status.cell_max_voltage_mV - 4100) * 800) / 1000);
    if(soc_times_ten > 1000) {
      soc_times_ten = 1000;  // Don't let it go above 100%
    }
  } else {
    // Scale so that 100% becomes 92%
    soc_times_ten = (uint16_t)(((uint32_t)soc_times_ten * 9200) / 10000);
  }
#endif

  // Set the state of charge in the datalayer
  datalayer.battery.status.real_soc = soc_times_ten * 10;

  RealSoC = datalayer.battery.status.real_soc / 100;

  // Calculate the remaining capacity.
  tempfloat = datalayer.battery.info.total_capacity_Wh * (RealSoC - MinSoC) / 100;
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
}

void MgHsPHEVBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  uint16_t soc1, soc2;
  switch (rx_frame.ID) {
    case 0x173:
      // Contains cell min/max voltages

      datalayer.battery.status.cell_max_voltage_mV = (rx_frame.data.u8[4]<<8) | rx_frame.data.u8[5];
      datalayer.battery.status.cell_min_voltage_mV = (rx_frame.data.u8[6]<<8) | rx_frame.data.u8[7];
      cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;

      break;
    case 0x297:
      // Contains battery status in rx_frame.data.u8[1]
      // Presumed mapping:
      // 1 = disconnected
      // 2 = precharge
      // 3 = connected
      // 15 = isolation faault
      // 0/8 = checking

      break;
    case 0x2A2:
      // Contains temperatures.

      // Max cell temp
      datalayer.battery.status.temperature_max_dC = ((rx_frame.data.u8[0] << 8) / 50) - 400;
      // Min cell temp
      datalayer.battery.status.temperature_min_dC = ((rx_frame.data.u8[5] << 8) / 50) - 400;
      // Coolant temp
      // ((rx_frame.data.u8[1] << 8)/50) - 400;

      // There is another unknown temp in [6]/[7]

      break;
    case 0x3AC:
      // Contains SoCs, voltage, current. Is emitted by both CAN1 and CAN2, but
      // the CAN2 version only has one SoC (soc2), the CAN1 version has all four
      // values.

      // Both SoCs top out at about ~4.1V/cell, the first SoC at 92%, and the
      // second at 100%.

      // They are scaled differently, the relationship seems to be:
      // soc2 = (1.392*soc1) - 28.064

      soc1 = (rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1]);
      soc2 = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]);

      // soc2 is present in both CAN1 and CAN2 messages
      update_soc(soc2);

      if (((rx_frame.data.u8[4] << 8) & 0xf00 | rx_frame.data.u8[5]) != 0) {
        // 3AC message contains a valid voltage (so must come from CAN1

        datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[4] << 8) & 0xf00 | rx_frame.data.u8[5]) * 2.5;
        datalayer.battery.status.current_dA = ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) - 20000)  * 0.5;
      }

      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7ED:
      // A response from our CAN2 OBD requests
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Process rx data for incoming responses to "Read data by ID" Tx
      if (rx_frame.data.u8[1] == 0x62) {
        if (rx_frame.data.u8[2] == 0xB0) {                                   //Battery information
          if (rx_frame.data.u8[3] == 0x41 && rx_frame.data.u8[0] == 0x05) {  // Battery bus voltage
            // Battery bus voltage
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 2.5;
          } else if (rx_frame.data.u8[3] == 0x42 && rx_frame.data.u8[0] == 0x05) {
            // Battery voltage
            datalayer.battery.status.voltage_dV = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 2.5;
          } else if (rx_frame.data.u8[3] == 0x43 && rx_frame.data.u8[0] == 0x05) {  
            // Battery current
            datalayer.battery.status.current_dA = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) - 40000) / -4;
          } else if (rx_frame.data.u8[3] == 0x45 && rx_frame.data.u8[0] == 0x05) {
            // Battery resistance
            // rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
          } else if (rx_frame.data.u8[3] == 0x46 && rx_frame.data.u8[0] == 0x05) {
            // The battery SoC, the same as soc2 in 3AC. Hits 100% at 4.1V/cell.
            soc2 = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
            update_soc(soc2);
          } else if (rx_frame.data.u8[3] == 0x47) {
            // BMS error code
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] == 0x48) {
            // BMS status coded
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] == 0x49) {
            // System main relay B status
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] == 0x4A) {
            // System main relay G status
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] ==
                     0x52) {  //    && rx_frame.data.u8[0] == 0x05) {	   // System main relay P status
            // System main relay P status
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] == 0x56 && rx_frame.data.u8[0] == 0x05) {
            // Max cell temperature
            datalayer.battery.status.temperature_max_dC =
                (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x57 && rx_frame.data.u8[0] == 0x05) {
            // Min cell temperature
            datalayer.battery.status.temperature_min_dC =
                (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x58 && rx_frame.data.u8[0] == 0x06) {
            // Max cell voltage
            datalayer.battery.status.cell_max_voltage_mV =
                rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
            cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
          } else if (rx_frame.data.u8[3] == 0x59 && rx_frame.data.u8[0] == 0x06) {  
            // Min cell voltage
            datalayer.battery.status.cell_min_voltage_mV =
                rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
          } else if (rx_frame.data.u8[3] == 0x61 && rx_frame.data.u8[0] == 0x05) {  
            // Battery SoH
            datalayer.battery.status.soh_pptt = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
          }
        }  // data.u8[2]=0xB0
      }  // data.u8[1] = 0x62)

      break;
    default:
      break;
  }
}
void MgHsPHEVBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

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

    switch (transmitIndex) {
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
        transmitIndex = 0;  //Return to the first message index. This goes in the last message entry
        break;
      default:
        break;

    }  //switch

    transmitIndex++;  //Increment the message index

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
  datalayer.battery.info.total_capacity_Wh = BATTERY_WH_MAX;
}

#endif
