#include "MG-HS-PHEV-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/*
MG HS PHEV 16.6kWh battery integration

This may work on other MG batteries, but will need some hardcoded constants
changing.


OPTIONAL SETTINGS

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

static int32_t linear_taper(int32_t input, int32_t input_min, int32_t input_max, int32_t output_min,
                            int32_t output_max) {
  if (input <= input_min) {
    return output_min;
  } else if (input >= input_max) {
    return output_max;
  } else {
    // Linear interpolation
    return output_min + ((output_max - output_min) * (input - input_min)) / (input_max - input_min);
  }
}

void MgHsPHEVBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Calculate the remaining capacity.
  uint32_t remaining =
      (datalayer_battery->info.total_capacity_Wh * (datalayer_battery->status.real_soc - DISCHARGE_MIN_SOC)) /
      (10000 - DISCHARGE_MIN_SOC);
  if (remaining > 0) {
    datalayer_battery->status.remaining_capacity_Wh = remaining;
  } else {
    datalayer_battery->status.remaining_capacity_Wh = 0;
  }

  // Initial limits are max

  int32_t max_charge_power_W = maxChargePowerW;
  int32_t max_discharge_power_W = maxDischargePowerW;

  // Cellvoltage-based power derating

  const int32_t CELL_VOLTAGE_WORKING_MAX_MV = datalayer_battery->info.chemistry == LFP ? 3500 : 4100;
  const int32_t CELL_VOLTAGE_WORKING_MIN_MV = datalayer_battery->info.chemistry == LFP ? 3100 : 3300;

  int32_t cell_max_charge_power_W =
      linear_taper(datalayer_battery->status.cell_max_voltage_mV, CELL_VOLTAGE_WORKING_MAX_MV - 50,
                   CELL_VOLTAGE_WORKING_MAX_MV, maxChargePowerW, 0);

  int32_t cell_max_discharge_power_W =
      linear_taper(datalayer_battery->status.cell_min_voltage_mV, CELL_VOLTAGE_WORKING_MIN_MV,
                   CELL_VOLTAGE_WORKING_MIN_MV + 50, 0, maxDischargePowerW);

  // Hysteresis on cell min voltage: trip at threshold, recover at +25 mV.
  if (voltageAtCellMin && datalayer_battery->status.cell_min_voltage_mV >= CELL_VOLTAGE_WORKING_MIN_MV + 25) {
    voltageAtCellMin = false;
  } else if (!voltageAtCellMin && datalayer_battery->status.cell_min_voltage_mV <= CELL_VOLTAGE_WORKING_MIN_MV) {
    voltageAtCellMin = true;
    cell_max_discharge_power_W = 0;
  }

  // Hysteresis on cell max voltage: trip at threshold, recover at -10 mV.
  if (voltageAtCellMax && datalayer_battery->status.cell_max_voltage_mV <= CELL_VOLTAGE_WORKING_MAX_MV - 10) {
    voltageAtCellMax = false;
  } else if (!voltageAtCellMax && datalayer_battery->status.cell_max_voltage_mV >= CELL_VOLTAGE_WORKING_MAX_MV) {
    voltageAtCellMax = true;
    cell_max_charge_power_W = 0;
  }

  if (cell_max_charge_power_W < max_charge_power_W) {
    max_charge_power_W = cell_max_charge_power_W;
  }
  if (cell_max_discharge_power_W < max_discharge_power_W) {
    max_discharge_power_W = cell_max_discharge_power_W;
  }

  // Temperature-based power derating:

  const int32_t MIN_TEMP_DC = datalayer_battery->info.chemistry == LFP ? 0 : -100;
  const int32_t MIN_WATTS_PER_DC = 280;  // gradient is 14kW per 5dC
  const int32_t MAX_TEMP_DC = 500;
  const int32_t MAX_WATTS_PER_DC = 140;  // gradient is 14kW per 10dC

  int32_t temp_low_max_power_W = (datalayer_battery->status.temperature_min_dC - MIN_TEMP_DC) * MIN_WATTS_PER_DC;
  int32_t temp_high_max_power_W = (MAX_TEMP_DC - datalayer_battery->status.temperature_max_dC) * MAX_WATTS_PER_DC;

  // Limit both charge and discharge power at temp extremes
  if (temp_low_max_power_W < max_discharge_power_W) {
    max_discharge_power_W = temp_low_max_power_W;
  }
  if (temp_low_max_power_W < max_charge_power_W) {
    max_charge_power_W = temp_low_max_power_W;
  }
  if (temp_high_max_power_W < max_discharge_power_W) {
    max_discharge_power_W = temp_high_max_power_W;
  }
  if (temp_high_max_power_W < max_charge_power_W) {
    max_charge_power_W = temp_high_max_power_W;
  }

  // SoC-based power derating

  int32_t soc_max_charge_power_W =
      linear_taper(datalayer_battery->status.real_soc, DERATE_CHARGE_ABOVE_SOC, 10000, maxChargePowerW, 0);

  int32_t soc_max_discharge_power_W = linear_taper(datalayer_battery->status.real_soc, DISCHARGE_MIN_SOC,
                                                   DERATE_DISCHARGE_BELOW_SOC, 0, maxDischargePowerW);

  // SoC updating with optional pinning

  if (false) {
    // Pin SoC at 1%/98% if cells aren't empty/full yet.
    // We ignore the SoC based limiting.
    if (voltageAtCellMin) {
      max_discharge_power_W = 0;  // Extra check for safety
      datalayer_battery->status.real_soc = 0;
    } else if (voltageAtCellMax) {
      max_charge_power_W = 0;  // Extra check for safety
      datalayer_battery->status.real_soc = 10000;
    } else if (soc > 9800) {
      // Cap at 98% to allow full rate charging
      datalayer_battery->status.real_soc = 9800;
    } else if (soc == 0) {
      // Cap at minimum to allow full rate discharging
      datalayer_battery->status.real_soc = 100;
    } else {
      datalayer_battery->status.real_soc = soc;
    }
  } else {
    // Apply SoC limiting
    if (soc_max_charge_power_W < max_charge_power_W) {
      max_charge_power_W = soc_max_charge_power_W;
    }
    if (soc_max_discharge_power_W < max_discharge_power_W) {
      max_discharge_power_W = soc_max_discharge_power_W;
    }

    if (voltageAtCellMin) {
      max_discharge_power_W = 0;  // Extra check for safety
    }
    if (voltageAtCellMax) {
      max_charge_power_W = 0;  // Extra check for safety
    }
    datalayer_battery->status.real_soc = soc;
  }

  datalayer_battery->status.max_charge_power_W = max_charge_power_W > 0 ? max_charge_power_W : 0;
  datalayer_battery->status.max_discharge_power_W = max_discharge_power_W > 0 ? max_discharge_power_W : 0;

  static int counter = 0;
  if (++counter > 300) {
    logging.printf("[MG] CHARGE: SoC: %d, Cell: %d, TempH: %d, TempL: %d, Final: %d\n", soc_max_charge_power_W,
                   cell_max_charge_power_W, temp_high_max_power_W, temp_low_max_power_W, max_charge_power_W);
    logging.printf("[MG] DISCHARGE: SoC: %d, Cell: %d, TempH: %d, TempL: %d, Final: %d\n", soc_max_discharge_power_W,
                   cell_max_discharge_power_W, temp_high_max_power_W, temp_low_max_power_W, max_discharge_power_W);
    counter = 0;
  }

  // Should be called every second
  if (cellVoltageValidTime > 0) {
    cellVoltageValidTime--;
  } else {
    // Pause the battery if we haven't received a cell voltage update in a while
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  }
  if (voltageValidTime > 0) {
    voltageValidTime--;
  }
}

void MgHsPHEVBattery::announce_contactor_state(bool state) {
  // Only the primary battery should announce the contactor state
  if (allowed_contactor_closing != nullptr) {
    datalayer.system.status.battery_allows_contactor_closing = state;
  }
}

void MgHsPHEVBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // We start polling with UDS ID 0x7DF, the generic broadcast one. Our first
  // reply will indicate what the BMS-specific one is, which we switch to.
  if (uds_address == 0x7DF && rx_frame.ID == 0x789) {
    setup_uds(0x781, 0, next_pid);
  } else if (uds_address == 0x7DF && rx_frame.ID == 0x7ED) {
    setup_uds(0x7E5, 0, next_pid);
  }

  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  uint32_t v, i, cell_id, soc2;

  switch (rx_frame.ID) {
    case 0x173:
      // Contains cell min/max voltages
      v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (v > 0 && v < 0x2000) {
        datalayer_battery->status.cell_max_voltage_mV = v;
        v = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        if (v > 0 && v < 0x2000) {
          if (v < 3000) {
            logging.printf("[MG] Low cell min: %d mV\n", v);
          }
          datalayer_battery->status.cell_min_voltage_mV = v;
          cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
        }
      }

      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x297:
      // Contains battery status in rx_frame.data.u8[1]
      // Presumed mapping:
      // 1 = disconnected
      // 2 = precharge
      // 3 = connected
      // 15 = fault (eg isolation, or waiting-too-long-before-closing-contactors)
      // 0/8 = checking

      if (rx_frame.data.u8[1] != previousState) {
        logging.printf("[MG] Battery status changed to %d (%d)\n", rx_frame.data.u8[1], rx_frame.data.u8[0]);

        if (resetProgress == WAITING_RESET_COMPLETE) {
          // We were waiting for a reset to complete, now has!
          logging.printf("[MG] Reset complete.\n");
          resetProgress = IDLE;
        }
      }

      // if (rx_frame.data.u8[1] == 0xf && previousState != 0xf) {
      //   // Isolation fault, set event
      //   set_event(EVENT_BATTERY_ISOLATION, rx_frame.data.u8[0]);
      // } else if (rx_frame.data.u8[1] != 0xf && previousState == 0xf) {
      //   // Isolation fault has cleared, clear event
      //   clear_event(EVENT_BATTERY_ISOLATION);
      // }

      if (datalayer.system.status.system_status == FAULT) {
        // If in fault state, don't try resetting things yet as it'll turn the
        // BMS off and we'll lose CAN info
      } else if (!datalayer.system.status.inverter_allows_contactor_closing || batteryType == 0 ||
                 highestSeenCellCount != datalayer_battery->info.number_of_cells) {
        // We haven't requested contactor closing, so we don't care what state
        // the BMS is in.
        announce_contactor_state(false);
      } else if ((rx_frame.data.u8[0] == 0x02 || rx_frame.data.u8[0] == 0x06) && rx_frame.data.u8[1] == 0x01) {
        // A weird 'stuck' state where the battery won't reconnect
        announce_contactor_state(false);
        if (batteryType == BATTERY_TYPE_MG_HS_PHEV) {
          if (!uds_is_busy()) {
            reset_BMS();
            logging.printf("[MG] Stuck, resetting.\n");
          }
        }
      } else if (rx_frame.data.u8[1] == 0xf) {
        // A fault state (likely isolation failure)
        announce_contactor_state(false);
        if (batteryType == BATTERY_TYPE_MG_HS_PHEV) {
          if (!uds_is_busy()) {
            reset_BMS();
            logging.printf("[MG] Fault, resetting.\n");
          }
        }
      } else {
        announce_contactor_state(true);
      }

      previousState = rx_frame.data.u8[1];

      break;
    case 0x2A2:
      // Contains temperatures.

      if (rx_frame.data.u8[0] < 0xfe) {
        // Max cell temp
        datalayer_battery->status.temperature_max_dC = ((rx_frame.data.u8[0] << 8) / 50) - 400;
      }
      if (rx_frame.data.u8[5] < 0xfe) {
        // Min cell temp
        datalayer_battery->status.temperature_min_dC = ((rx_frame.data.u8[5] << 8) / 50) - 400;
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

      //soc1 = (rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1]);
      soc2 = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]);

      // soc2 is present in both CAN1 and CAN2 messages
      if (soc2 < 1022) {
        //update_soc(soc2);
        soc = soc2 * 10;
      }

      // Battery voltage
      v = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]);
      // Current
      i = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);

      if (v > 0 && v < 2400 && i > 16000 && i < 24000) {
        // 3AC message contains a credible voltage and current (so must have come from CAN1)
        // (voltage between 0 and 600V, current between -200A and +200A)

        datalayer_battery->status.voltage_dV = (v * 5) / 2;
        datalayer_battery->status.current_dA = -(i - 20000) / 2;
        voltageValidTime = VOLTAGE_TIMEOUT;
      }

      break;
    case 0x3BE:
      // Per-cell voltages and temps
      cell_id = rx_frame.data.u8[5];
      if (cell_id < datalayer_battery->info.number_of_cells) {
        v = 1000 + ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
        datalayer_battery->status.cell_voltages_mV[cell_id] = v < 10000 ? v : 0;
        if (v < 10000 && cell_id >= highestSeenCellCount) {
          highestSeenCellCount = cell_id + 1;
        }
        // cell temperature is rx_frame.data.u8[1]-40 but BE doesn't use it
      }

      break;
    /*
    These are handled via handle_pid now.
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
            // datalayer_battery->status.voltage_dV = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) * 2.5;
          } else if (rx_frame.data.u8[3] == 0x43 && rx_frame.data.u8[0] == 0x05) {
            // Battery current
            // we won't update this as it differs in rounding from the CAN1 version
            //datalayer_battery->status.current_dA = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) - 40000) / -4;
          } else if (rx_frame.data.u8[3] == 0x45 && rx_frame.data.u8[0] == 0x05) {
            // Battery resistance
            // rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
          } else if (rx_frame.data.u8[3] == 0x46 && rx_frame.data.u8[0] == 0x05) {
            // The battery SoC, the same as soc1 in 3AC.
            //soc1 = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
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
            // datalayer_battery->status.temperature_max_dC =
            //     (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x57 && rx_frame.data.u8[0] == 0x05) {
            // Min cell temperature
            // datalayer_battery->status.temperature_min_dC =
            //     (((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 500) - 40) * 10;
          } else if (rx_frame.data.u8[3] == 0x58 && rx_frame.data.u8[0] == 0x06) {
            // Max cell voltage
            // datalayer_battery->status.cell_max_voltage_mV = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
            // cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
          } else if (rx_frame.data.u8[3] == 0x59 && rx_frame.data.u8[0] == 0x06) {
            // Min cell voltage
            // datalayer_battery->status.cell_min_voltage_mV = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
          } else if (rx_frame.data.u8[3] == 0x61 && rx_frame.data.u8[0] == 0x05) {
            // Battery SoH
            datalayer_battery->status.soh_pptt = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]);
          }
        }  // data.u8[2]=0xB0
      }  // data.u8[1] = 0x62)

      break;
    */
    default:
      break;
  }
}

void MgHsPHEVBattery::got_battery_type(uint32_t type) {
  // We've received a battery type code, which we can use to update the battery
  // parameters.

  logging.printf("[MG] Battery type code: %X\n", type);
  batteryType = type;
  if (batteryType == BATTERY_TYPE_MG_HS_PHEV) {
    logging.println("[MG] Detected MG HS PHEV battery (90s)");
    datalayer_battery->info.number_of_cells = 90;
    if (datalayer_battery->info.total_capacity_Wh == 0) {
      datalayer_battery->info.total_capacity_Wh = 16600;
    }
  } else if (batteryType == BATTERY_TYPE_MG_ZS) {
    logging.println("[MG] Detected MG ZS EV battery (108s)");
    maxChargePowerW = 14000;
    maxDischargePowerW = 14000;
    datalayer_battery->info.number_of_cells = 108;
    if (datalayer_battery->info.total_capacity_Wh == 0) {
      datalayer_battery->info.total_capacity_Wh = 44500;
    }
    //} else if (batteryType == BATTERY_TYPE_MG5_61_NMC) {
    //   logging.println("[MG] Detected MG5 61kWh NMC (96s)");
    //   datalayer_battery->info.number_of_cells = 96;
    //   if(datalayer_battery->info.total_capacity_Wh == 0) {
    //     datalayer_battery->info.total_capacity_Wh = 61000;
    //   }
  } else {
    logging.printf("[MG] Assuming MG5 battery (96s)\n");
    batteryType = BATTERY_TYPE_MG5;
    maxChargePowerW = 14000;
    maxDischargePowerW = 14000;
    datalayer_battery->info.number_of_cells = 96;
    if (datalayer_battery->info.total_capacity_Wh == 0) {
      datalayer_battery->info.total_capacity_Wh = 52500;
    }
  }
  datalayer_battery->info.max_design_voltage_dV =
      (MAX_CELL_VOLTAGE_MV * (uint32_t)datalayer_battery->info.number_of_cells) / 100;
  datalayer_battery->info.min_design_voltage_dV =
      (MIN_CELL_VOLTAGE_MV * (uint32_t)datalayer_battery->info.number_of_cells) / 100;
}

uint32_t MgHsPHEVBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length,
                                     UdsStatus status) {

  if (status == UdsStatus::OK) {
    if (pid >= 0xF100 || length > 4) {
      logging.printf("[MG] PID %X:", pid);
      for (int i = 0; i < length; i++) {
        logging.printf(" %02X", data[i]);
      }
      logging.printf("\n");
    }
  }

  switch (pid) {
    // case POLL_BATTERY_VOLTAGE:
    //   // We don't actually use this value
    //   return POLL_BATTERY_SOH;
    case POLL_BATTERY_SOH:  // Battery SoH
      if (status == UdsStatus::OK) {
        datalayer_battery->status.soh_pptt = value;
      }
      break;
      //return 0xb099;  //deliberately invalid PID
      //case 0xb099:
      //  break;

    case POLL_BATTERY_TYPE:  // Battery type
      if (status != UdsStatus::OK || value == 0) {
        return POLL_BATTERY_TYPE;  // Try again until we get a valid response
      }
      got_battery_type(value);
      memcpy(pid_f18a, data, length > sizeof(pid_f18a) ? sizeof(pid_f18a) : length);
      return 0xF120;
    case 0xF120:
      memcpy(pid_f120, data, length > sizeof(pid_f120) ? sizeof(pid_f120) : length);
      return 0xB18C;
    case 0xB18C:
      memcpy(pid_b18c, data, length > sizeof(pid_b18c) ? sizeof(pid_b18c) : length);
      return 0xF183;
    case 0xF183:
      memcpy(pid_f183, data, length > sizeof(pid_f183) ? sizeof(pid_f183) : length);
      return 0xF18B;
    case 0xF18B:
      memcpy(pid_f18b, data, length > sizeof(pid_f18b) ? sizeof(pid_f18b) : length);
      return 0xF190;
    case 0xF190:
      memcpy(pid_f190, data, length > sizeof(pid_f190) ? sizeof(pid_f190) : length);
      return 0xF191;
    case 0xF191:
      memcpy(pid_f191, data, length > sizeof(pid_f191) ? sizeof(pid_f191) : length);
      return 0xF192;
    case 0xF192:
      memcpy(pid_f192, data, length > sizeof(pid_f192) ? sizeof(pid_f192) : length);
      return 0xF194;
    case 0xF194:
      memcpy(pid_f194, data, length > sizeof(pid_f194) ? sizeof(pid_f194) : length);
      return 0xF1A2;
    case 0xF1A2:
      memcpy(pid_f1a2, data, length > sizeof(pid_f1a2) ? sizeof(pid_f1a2) : length);
      return 0xF1AA;
    case 0xF1AA:
      memcpy(pid_f1aa, data, length > sizeof(pid_f1aa) ? sizeof(pid_f1aa) : length);
      // Finished reading identifiers, start normal polling
      setup_uds(uds_address, 0, POLL_BATTERY_SOH);
      break;
  }
  return 0;  // Continue normal PID cycling
}

void MgHsPHEVBattery::transmit_can(unsigned long currentMillis) {
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis10 = currentMillis;
    previousMillis20 = currentMillis;
    previousMillis100 = currentMillis;
    return;
  }

  static int8_t send_phase = -1;
  if (++send_phase > 2) {
    send_phase = 0;
  }

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS && send_phase == 0) {
    previousMillis10 = currentMillis;

#if MG_HS_PHEV_DISABLE_CONTACTORS
    // Leave the contactors open
    MG_HS_8A.data.u8[5] = 0x00;
#else
    // It can take up to 30s to establish the cell count. During this period we
    // won't send any open-contactor messages (unless there is a FAULT or
    // inverter requests opening) - if the contactors were already closed,
    // they'll remain so until the BMS times out.
    // This allows us to maintain closed contactors after reboots.
    static constexpr uint32_t STARTUP_GRACE_PERIOD_MS = 30000;  // 30 seconds

    // We've got the battery type and have seen the expected number of cells
    const bool identified_battery = batteryType != 0 && highestSeenCellCount == datalayer_battery->info.number_of_cells;
    // Open contactors if fault
    const bool must_open_contactors = datalayer.system.status.system_status == FAULT;
    // Open contactors if inverter requests it, or we haven't identified the
    // battery yet, or we don't have a recent voltage reading, or if we're a
    // secondary battery and haven't been given permission to close yet.
    const bool should_open_contactors = !datalayer.system.status.inverter_allows_contactor_closing ||
                                        !identified_battery || voltageValidTime == 0 ||
                                        (allowed_contactor_closing != nullptr && !*allowed_contactor_closing);

    if (must_open_contactors || (should_open_contactors && currentMillis > STARTUP_GRACE_PERIOD_MS)) {

      if (announcedContactorsClosed) {
        logging.printf("[MG] Open contactors, iacc: %d, hSCC: %d, bT: %d, accnull: %d, acc: %d\n",
                       datalayer.system.status.inverter_allows_contactor_closing, highestSeenCellCount, batteryType,
                       allowed_contactor_closing == nullptr,
                       allowed_contactor_closing != nullptr ? *allowed_contactor_closing : 0);
        announcedContactorsClosed = false;
      }

      MG_HS_8A.data.u8[5] = 0x00;
      //MG_391.data.u8[4] = 0xB0;
      contactorCloseReset = false;
      warmupCounter = 0;

      MG_HS_8A.data.u8[6] = 0x10 | eightAcycle;
    } else if (should_open_contactors) {
      // We are still in the startup grace period - don't send anything, if the
      // contactors are still closed, they can remain so until the battery times
      // out.
    } else {
      // Everything ready, close contactors
      MG_HS_8A.data.u8[5] = 0x02;
      //MG_391.data.u8[4] = 0xD0;

      if (!announcedContactorsClosed) {
        logging.printf("[MG] Close contactors, iacc: %d, hSCC: %d, bT: %d, accnull: %d, acc: %d\n",
                       datalayer.system.status.inverter_allows_contactor_closing, highestSeenCellCount, batteryType,
                       allowed_contactor_closing == nullptr,
                       allowed_contactor_closing != nullptr ? *allowed_contactor_closing : 0);
        announcedContactorsClosed = true;
      }

      if (warmupCounter < 1100) {
        // Keep the 1 asserted for 110 messages
        MG_HS_8A.data.u8[6] = 0x10 | eightAcycle;
        warmupCounter += 10;
      } else {
        MG_HS_8A.data.u8[6] = 0x30 | eightAcycle;
      }

      if (!contactorCloseReset && (batteryType != BATTERY_TYPE_MG_HS_PHEV)) {
        // MG5/ZS requires DTCs clearing to get contactors to close
        logging.printf("[MG] Resetting DTCs\n");
        reset_DTC();
        contactorCloseReset = true;
      }
    }
#endif  // MG_HS_PHEV_DISABLE_CONTACTORS

    // Basic XOR checksum
    MG_HS_8A.data.u8[7] = (MG_HS_8A.data.u8[0] ^ MG_HS_8A.data.u8[1] ^ MG_HS_8A.data.u8[2] ^ MG_HS_8A.data.u8[3] ^
                           MG_HS_8A.data.u8[4] ^ MG_HS_8A.data.u8[5] ^ MG_HS_8A.data.u8[6]);
    eightAcycle = (eightAcycle + 1) & 0xF;

    transmit_can_frame(&MG_HS_8A);
  }
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS && send_phase == 1) {
    previousMillis20 = currentMillis;

    //transmit_can_frame(&MG_391);
    transmit_can_frame(&MG_HS_1F1);
  }

  transmit_uds_can(currentMillis);
}

void MgHsPHEVBattery::setup(void) {  // Performs one time setup at startup
  setup_uds(0x7DF, 0, POLL_BATTERY_TYPE);
  dtc = &datalayer_battery->dtc;

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  announce_contactor_state(false);
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.number_of_cells = 90;

  // Start off with wide range until we detect the battery type
  datalayer_battery->info.max_design_voltage_dV = ((uint32_t)108 * MAX_CELL_VOLTAGE_MV) / 100;
  datalayer_battery->info.min_design_voltage_dV = ((uint32_t)90 * MIN_CELL_VOLTAGE_MV) / 100;
}
