#include "MG-HS-PHEV-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/*
MG HS PHEV 16.6kWh battery integration

This may work on other MG batteries, but will need some hardcoded constants
changing.


OPTIONAL SETTINGS

// This will scale the SoC so the batteries top out at 4.2V/cell instead of
4.1V/cell. The car only seems to use up to 4.1V/cell in service. 
#define MG_HS_PHEV_USE_FULL_CAPACITY true

// If you have bypassed the contactors, you can avoid them being activated //
(which also disables isolation resistance measuring). 
#define MG_HS_PHEV_DISABLE_CONTACTORS true


CAN CONNECTIONS

Battery Emulator should be connected via CAN to either:

- CAN1 (pins 1+2 on the LV connector)

This provides efficient data updates including individual cell voltages, and
allows control over the contactors.

- CAN1 and CAN2 (pins 3+4) in parallel

This adds extra information (currently just SoH), and works in practice despite
the potential problems with connecting CAN buses in parallel.

NOTES

- Charge power/discharge power is estimated for now

*/

void MgHsPHEVBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second
  if (cellVoltageValidTime > 0) {
    cellVoltageValidTime--;
  }
}

void MgHsPHEVBattery::update_soc(uint16_t soc_times_ten) {
#if MG_HS_PHEV_USE_FULL_CAPACITY
  // The SoC hits 100% at 4.1V/cell. To get the full 4.2V/cell we need to use
  // voltage instead for the last bit.

  if (cellVoltageValidTime == 0) {
    // We don't have a recent cell max voltage reading, so can't do
    // voltage-based SoC.
  } else if (soc_times_ten > 900 && datalayer.battery.status.cell_max_voltage_mV < 4000) {
    // Something is wrong with our max cell voltage reading (it is too low), so
    // don't trust it - we'll just let the SoC hit 100%.
  } else if (soc_times_ten == 1000 && datalayer.battery.status.cell_max_voltage_mV >= 4100) {
    // We've hit 100%, so use voltage-based-SoC calculation for the last bit.

    // We usually hit 92% at ~369V, and the pack max is 378V.

    // Scale so that 100% becomes 92%
    soc_times_ten = (uint16_t)(((uint32_t)soc_times_ten * 9200) / 10000);

    // Add on the last 100mV as the last 8% of SoC.
    soc_times_ten += (uint16_t)((((uint32_t)datalayer.battery.status.cell_max_voltage_mV - 4100) * 800) / 1000);
    if (soc_times_ten > 1000) {
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
  switch (rx_frame.ID) {
    case 0x173:
      // Contains cell min/max voltages
      v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (v > 0 && v < 0x2000) {
        datalayer.battery.status.cell_max_voltage_mV = v;
        v = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        if (v > 0 && v < 0x2000) {
          datalayer.battery.status.cell_min_voltage_mV = v;
          cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
        }
      }

      break;
    case 0x297:
      // Contains battery status in rx_frame.data.u8[1]
      // Presumed mapping:
      // 1 = disconnected
      // 2 = precharge
      // 3 = connected
      // 15 = isolation fault
      // 0/8 = checking

      if (rx_frame.data.u8[1] != previousState) {
        logging.printf("MG_HS_PHEV: Battery status changed to %d (%d)\n", rx_frame.data.u8[1], rx_frame.data.u8[0]);
      }

      if (rx_frame.data.u8[1] == 0xf && previousState != 0xf) {
        // Isolation fault, set event
        set_event(EVENT_BATTERY_ISOLATION, rx_frame.data.u8[0]);
      } else if (rx_frame.data.u8[1] != 0xf && previousState == 0xf) {
        // Isolation fault has cleared, clear event
        clear_event(EVENT_BATTERY_ISOLATION);
      }

      if (datalayer.battery.status.bms_status == FAULT) {
        // If in fault state, don't try resetting things yet as it'll turn the
        // BMS off and we'll lose CAN info
      } else if ((rx_frame.data.u8[0] == 0x02 || rx_frame.data.u8[0] == 0x06) && rx_frame.data.u8[1] == 0x01) {
        // A weird 'stuck' state where the battery won't reconnect
        datalayer.system.status.battery_allows_contactor_closing = false;
        if (!datalayer.system.status.BMS_startup_in_progress) {
          logging.printf("MG_HS_PHEV: Stuck, resetting.\n");
          start_bms_reset();
        }
      } else if (rx_frame.data.u8[1] == 0xf) {
        // A fault state (likely isolation failure)
        datalayer.system.status.battery_allows_contactor_closing = false;
        if (!datalayer.system.status.BMS_startup_in_progress) {
          logging.printf("MG_HS_PHEV: Fault, resetting.\n");
          start_bms_reset();
        }
      } else {
        datalayer.system.status.battery_allows_contactor_closing = true;
      }

      previousState = rx_frame.data.u8[1];

      break;
    case 0x2A2:
      // Contains temperatures.

      if (rx_frame.data.u8[0] < 0xfe) {
        // Max cell temp
        datalayer.battery.status.temperature_max_dC = ((rx_frame.data.u8[0] << 8) / 50) - 400;
      }
      if (rx_frame.data.u8[5] < 0xfe) {
        // Min cell temp
        datalayer.battery.status.temperature_min_dC = ((rx_frame.data.u8[5] << 8) / 50) - 400;
      }
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
      if (soc2 < 1022) {
        update_soc(soc2);
        datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      }

      if ((((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]) != 0) {
        // 3AC message contains a nonzero voltage (so must have come from CAN1)
        v = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]);
        if (v > 0 && v < 4000) {
          datalayer.battery.status.voltage_dV = v * 2.5;
        }
        // Current
        v = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        if (v > 0 && v < 0xf000) {
          datalayer.battery.status.current_dA = -(v - 20000) * 0.5;
        }
      }

      break;
    case 0x3BE:
      // Per-cell voltages and temps
      cell_id = rx_frame.data.u8[5];
      if (cell_id < 90) {
        v = 1000 + ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
        datalayer.battery.status.cell_voltages_mV[cell_id] = v < 10000 ? v : 0;
        // cell temperature is rx_frame.data.u8[1]-40 but BE doesn't use it
      }

      break;
    case 0x7ED:
      // A response from our CAN2 OBD requests
      // We mostly ignore these, apart from SoH, and also the voltage as a
      // safety measure (in case CAN1 misbehaves).
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
            // we won't update this as it differs in rounding from the CAN1 version
            //datalayer.battery.status.current_dA = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) - 40000) / -4;
          } else if (rx_frame.data.u8[3] == 0x45 && rx_frame.data.u8[0] == 0x05) {
            // Battery resistance
            // rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
          } else if (rx_frame.data.u8[3] == 0x46 && rx_frame.data.u8[0] == 0x05) {
            // The battery SoC, the same as soc1 in 3AC.
            soc1 = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
            // We won't use since we're using soc2
          } else if (rx_frame.data.u8[3] == 0x47) {
            // BMS error code
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
          } else if (rx_frame.data.u8[3] == 0x48) {
            // BMS status coded
            // (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);	// HOLD: What to do with this data
            // This is the same as 297[1]
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
            // datalayer.battery.status.temperature_max_dC =
            //     (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x57 && rx_frame.data.u8[0] == 0x05) {
            // Min cell temperature
            // datalayer.battery.status.temperature_min_dC =
            //     (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x58 && rx_frame.data.u8[0] == 0x06) {
            // Max cell voltage
            // datalayer.battery.status.cell_max_voltage_mV = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
            // cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
          } else if (rx_frame.data.u8[3] == 0x59 && rx_frame.data.u8[0] == 0x06) {
            // Min cell voltage
            // datalayer.battery.status.cell_min_voltage_mV = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
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
  if (datalayer.system.status.BMS_reset_in_progress || datalayer.system.status.BMS_startup_in_progress) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    return;
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

#if MG_HS_PHEV_DISABLE_CONTACTORS
    // Leave the contactors open
    MG_HS_8A.data.u8[5] = 0x00;
#else
    if (datalayer.battery.status.bms_status == FAULT) {
      // Fault, so open contactors!
      MG_HS_8A.data.u8[5] = 0x00;
    } else if (!datalayer.system.status.inverter_allows_contactor_closing) {
      // Inverter requests contactor opening
      MG_HS_8A.data.u8[5] = 0x00;
    } else {
      // Everything ready, close contactors
      MG_HS_8A.data.u8[5] = 0x02;
    }
#endif  // MG_HS_PHEV_DISABLE_CONTACTORS

    transmit_can_frame(&MG_HS_8A);
    transmit_can_frame(&MG_HS_1F1);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (transmitIndex) {
      case 1:
        transmit_can_frame(&MG_HS_7E5_B0_42);  //Battery voltage
        break;
      case 2:
        transmit_can_frame(&MG_HS_7E5_B0_61);  //Battery SoH
        transmitIndex = 0;                     //Return to the first message index. This goes in the last message entry
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
  datalayer.battery.info.number_of_cells = 90;
}
