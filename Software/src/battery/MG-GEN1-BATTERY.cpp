#include "MG-GEN1-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

// If true, SoC doesn't affect the charge/discharge power, and gets pinned at
// 1%/98% if the cells aren't empty/full yet (and forced to 0%/100 if they are).
// This means that cellvoltages alone become the determiner for charge/discharge
// power and termination.
#ifdef MGGEN1_DISABLE_SOC_TAPERING
static constexpr bool DISABLE_SOC_TAPERING = true;
#else
static constexpr bool DISABLE_SOC_TAPERING = false;
#endif

// SoC-based power derating

static constexpr uint16_t DERATE_CHARGE_ABOVE_SOC = 9500;    // in 0.01% units
static constexpr uint16_t DERATE_DISCHARGE_BELOW_SOC = 500;  // in 0.01% units
static constexpr uint16_t DISCHARGE_MIN_SOC = 0;

// Cell-voltage-based power derating

// Working voltage range
#ifdef MGGEN1_NMC_WORKING_MAX_MV
static const uint16_t NMC_WORKING_MAX_MV = MGGEN1_NMC_WORKING_MAX_MV;
#else
static const uint16_t NMC_WORKING_MAX_MV = 4200;
#endif
static const uint16_t NMC_WORKING_MIN_MV = 3300;
static const uint16_t LFP_WORKING_MAX_MV = 3500;
static const uint16_t LFP_WORKING_MIN_MV = 3000;

static constexpr int32_t CHARGE_TAPER_MV = 50;
static constexpr int32_t CHARGE_HYSTERESIS_MV = 10;

static constexpr int32_t DISCHARGE_TAPER_MV = 50;
static constexpr int32_t DISCHARGE_HYSTERESIS_MV = 25;

// Temperature-based power derating

// Temp thresholds for the two chemistries
static constexpr int32_t MIN_TEMP_NMC_DC = -100;
static constexpr int32_t MIN_TEMP_LFP_DC = 0;
static constexpr int32_t MAX_TEMP_DC = 500;
// Taper down to 0W at min temp with a linear gradient (affects charge only)
static constexpr int32_t MIN_WATTS_PER_DC = 280;  // gradient is 14kW per 5dC
// Taper down to 0W at max temp with a linear gradient (both charge and discharge)
static constexpr int32_t MAX_WATTS_PER_DC = 140;  // gradient is 14kW per 10dC

// UDS sequence timeouts (in 100ms ticks)
static constexpr uint16_t MG_UDS_TIMEOUT_SESSION_CONTROL = 10;
static constexpr uint16_t MG_UDS_TIMEOUT_RESET = 50;

// Pause after a reset (in 100ms ticks)
static constexpr uint16_t MG_UDS_POST_RESET_PAUSE = 50;
// Suppress CAN errors for a while after reset
static constexpr uint32_t MG_BMS_RESET_IGNORE_CAN_ERRORS_MS = 10000;

/* Clamped linear interpolation: output_min at input_min, output_max at
   input_max, linear between. Inputs outside the range are clamped. */
static int32_t battery_linear_taper(int32_t input, int32_t input_min, int32_t input_max, int32_t output_min,
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

/* SoC based charge derating: full max_charge_power up to derate_above_soc (in
   0.01% units), then a linear taper down to trickle_charge_power at 100%. */
static uint32_t battery_charge_power_by_soc(uint32_t soc, uint32_t max_charge_power, uint32_t trickle_charge_power,
                                            uint16_t derate_above_soc) {
  if (soc <= derate_above_soc) {
    return max_charge_power;
  } else if (soc >= 10000) {
    return trickle_charge_power;
  } else {
    // Linear derate
    return max_charge_power -
           ((max_charge_power - trickle_charge_power) * (soc - derate_above_soc)) / (10000 - derate_above_soc);
  }
}

/* SoC based discharge derating: full max_discharge_power down to
   derate_below_soc (in 0.01% units), then a linear taper down to 0 at min_soc. */
static uint32_t battery_discharge_power_by_soc(uint32_t soc, uint32_t max_discharge_power, uint16_t min_soc,
                                               uint16_t derate_below_soc) {
  if (soc >= derate_below_soc) {
    return max_discharge_power;
  } else if (soc <= min_soc) {
    return 0;
  } else {
    // Linear derate
    return max_discharge_power - ((max_discharge_power * (derate_below_soc - soc)) / (derate_below_soc - min_soc));
  }
}

/* Cell voltage based charge derating with hysteresis. Tapers linearly to 0 as
   cell_max_mV rises from (working_max_mV - taper_mV) to working_max_mV. Once
   tripped at working_max_mV the limit stays 0 until cell_max_mV drops back to
   working_max_mV - recover_mV. The tripped flag persists between calls. */
static int32_t battery_charge_power_by_cell_max(uint32_t cell_max_mV, uint32_t working_max_mV, uint32_t taper_mV,
                                                uint32_t recover_mV, int32_t max_charge_power_W, bool* tripped) {
  if (*tripped && cell_max_mV <= working_max_mV - recover_mV) {
    *tripped = false;
  } else if (!*tripped && cell_max_mV >= working_max_mV) {
    *tripped = true;
  }
  int32_t power = battery_linear_taper((int32_t)cell_max_mV, (int32_t)(working_max_mV - taper_mV),
                                       (int32_t)working_max_mV, max_charge_power_W, 0);
  return *tripped ? 0 : power;
}

/* Cell voltage based discharge derating with hysteresis. Tapers linearly to 0
   as cell_min_mV falls from (working_min_mV + taper_mV) to working_min_mV.
   Once tripped at working_min_mV the limit stays 0 until cell_min_mV recovers
   to working_min_mV + recover_mV. The tripped flag persists between calls. */
static int32_t battery_discharge_power_by_cell_min(uint32_t cell_min_mV, uint32_t working_min_mV, uint32_t taper_mV,
                                                   uint32_t recover_mV, int32_t max_discharge_power_W, bool* tripped) {
  if (*tripped && cell_min_mV >= working_min_mV + recover_mV) {
    *tripped = false;
  } else if (!*tripped && cell_min_mV <= working_min_mV) {
    *tripped = true;
  }
  int32_t power = battery_linear_taper((int32_t)cell_min_mV, (int32_t)working_min_mV,
                                       (int32_t)(working_min_mV + taper_mV), 0, max_discharge_power_W);
  return *tripped ? 0 : power;
}

/* Temperature based derating, low side: power scales linearly from 0 at
   min_temp_dC upwards at watts_per_dC per degree C. Below min_temp_dC the value
   goes negative (meaning "no power allowed" once intersected). */
static int32_t battery_power_by_low_temp(int32_t temp_min_dC, int32_t min_temp_dC, int32_t watts_per_dC) {
  return (temp_min_dC - min_temp_dC) * watts_per_dC;
}

/* Temperature based derating, high side: power scales linearly from 0 at
   max_temp_dC downwards at watts_per_dC per degree C. Above max_temp_dC the
   value goes negative (meaning "no power allowed" once intersected). */
static int32_t battery_power_by_high_temp(int32_t temp_max_dC, int32_t max_temp_dC, int32_t watts_per_dC) {
  return (max_temp_dC - temp_max_dC) * watts_per_dC;
}

// Renders characters if printable, otherwise as [xx] hex.
static void print_chars_or_hex(char* buf, const uint8_t* data, uint16_t length) {
  int ptr = 0;
  for (int i = 0; i < length && ptr < 62; i++) {
    if (data[i] >= 32 && data[i] <= 126) {
      buf[ptr++] = (char)data[i];
    } else {
      int written = sprintf(buf + ptr, "[%02x]", data[i]);
      ptr += written;
    }
  }
  buf[ptr] = '\0';
}

String MgGen1Battery::get_uds_info_html() {
  String ret = String();
  ret.reserve(512);  //Pre-allocate some memory to avoid fragmentation

  char buf[128];

  ret += "UDS address: ";
  ret += String(uds_address, 16);
  ret += "<br>VIN: ";
  print_chars_or_hex(buf, pid_vin, 17);
  ret += buf;
  ret += "<br>MfrDate: ";
  sprintf(buf, "20%02X-%02X-%02X", pid_mfr_date[0], pid_mfr_date[1], pid_mfr_date[2]);
  ret += buf;
  ret += "<br>Fingerprint: ";
  print_chars_or_hex(buf, pid_fingerprint, 10);
  ret += buf;
  ret += "<br>VehHWNo: ";
  print_chars_or_hex(buf, pid_vehicle_hw_number, 5);
  ret += buf;
  ret += "<br>SysHWNo: ";
  print_chars_or_hex(buf, pid_system_hw_number, 10);
  ret += buf;
  ret += "<br>SysSWNo: ";
  print_chars_or_hex(buf, pid_system_sw_number, 10);
  ret += buf;
  ret += "<br>F18A: ";
  print_chars_or_hex(buf, pid_f18a, 8);
  ret += buf;
  ret += "<br>F120: ";
  print_chars_or_hex(buf, pid_f120, 16);
  ret += buf;
  ret += "<br>B18C: ";
  print_chars_or_hex(buf, pid_b18c, 24);
  ret += buf;
  ret += "<br>F1A2: ";
  print_chars_or_hex(buf, pid_f1a2, 8);
  ret += buf;
  ret += "<br>F1AA: ";
  print_chars_or_hex(buf, pid_f1aa, 5);
  ret += buf;
  ret += "<br>";

  return ret;
}

void MgGen1Battery::update_values() {

  // Calculate the remaining capacity.
  uint32_t remaining =
      (datalayer_battery->info.total_capacity_Wh * (datalayer_battery->status.real_soc - DISCHARGE_MIN_SOC)) /
      (10000 - DISCHARGE_MIN_SOC);
  if (remaining > 0) {
    datalayer_battery->status.remaining_capacity_Wh = remaining;
  } else {
    datalayer_battery->status.remaining_capacity_Wh = 0;
  }

  // Initial limits are the maximum

  int32_t max_charge_power_W = maxChargePowerW;
  int32_t max_discharge_power_W = maxDischargePowerW;

  // Cellvoltage-based power derating

  const uint32_t CELL_VOLTAGE_WORKING_MAX_MV =
      datalayer_battery->info.chemistry == LFP ? LFP_WORKING_MAX_MV : NMC_WORKING_MAX_MV;
  const uint32_t CELL_VOLTAGE_WORKING_MIN_MV =
      datalayer_battery->info.chemistry == LFP ? LFP_WORKING_MIN_MV : NMC_WORKING_MIN_MV;

  // Taper linearly to zero over a 50mV window, then latch at zero (with
  // hysteresis) until the cell voltage recovers past the trip threshold.
  const int32_t cell_max_charge_power_W =
      battery_charge_power_by_cell_max(datalayer_battery->status.cell_max_voltage_mV, CELL_VOLTAGE_WORKING_MAX_MV,
                                       CHARGE_TAPER_MV, CHARGE_HYSTERESIS_MV, maxChargePowerW, &voltageAtCellMax);
  const int32_t cell_max_discharge_power_W = battery_discharge_power_by_cell_min(
      datalayer_battery->status.cell_min_voltage_mV, CELL_VOLTAGE_WORKING_MIN_MV, DISCHARGE_TAPER_MV,
      DISCHARGE_HYSTERESIS_MV, maxDischargePowerW, &voltageAtCellMin);

  if (cell_max_charge_power_W < max_charge_power_W) {
    max_charge_power_W = cell_max_charge_power_W;
  }
  if (cell_max_discharge_power_W < max_discharge_power_W) {
    max_discharge_power_W = cell_max_discharge_power_W;
  }

  // Temperature-based power derating:

  const int32_t MIN_TEMP_DC = datalayer_battery->info.chemistry == LFP ? MIN_TEMP_LFP_DC : MIN_TEMP_NMC_DC;

  const int32_t temp_high_max_power_W =
      battery_power_by_high_temp(datalayer_battery->status.temperature_max_dC, MAX_TEMP_DC, MAX_WATTS_PER_DC);
  const int32_t temp_low_max_power_W =
      battery_power_by_low_temp(datalayer_battery->status.temperature_min_dC, MIN_TEMP_DC, MIN_WATTS_PER_DC);

  // Limit both charge and discharge power at high temperature
  if (temp_high_max_power_W < max_discharge_power_W) {
    max_discharge_power_W = temp_high_max_power_W;
  }
  if (temp_high_max_power_W < max_charge_power_W) {
    max_charge_power_W = temp_high_max_power_W;
  }

  // Limit charge power only at low temperature
  if (temp_low_max_power_W < max_charge_power_W) {
    max_charge_power_W = temp_low_max_power_W;
  }

  // SoC-based power derating

  const int32_t soc_max_charge_power_W =
      battery_charge_power_by_soc(datalayer_battery->status.real_soc, maxChargePowerW, 0, DERATE_CHARGE_ABOVE_SOC);

  const int32_t soc_max_discharge_power_W = battery_discharge_power_by_soc(
      datalayer_battery->status.real_soc, maxDischargePowerW, DISCHARGE_MIN_SOC, DERATE_DISCHARGE_BELOW_SOC);

  // SoC updating with optional pinning

  if (DISABLE_SOC_TAPERING) {
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

    // Note: the cell-voltage helpers above latch the power to zero while
    // tripped, so no extra voltage checks are needed here.
    datalayer_battery->status.real_soc = soc;
  }

  datalayer_battery->status.max_charge_power_W = max_charge_power_W > 0 ? max_charge_power_W : 0;
  datalayer_battery->status.max_discharge_power_W = max_discharge_power_W > 0 ? max_discharge_power_W : 0;

  if (++limit_message_counter > 600) {
    logging.printf("[MG] CHARGE: SoC: %d, Cell: %d, TempH: %d, TempL: %d, Final: %d\n", soc_max_charge_power_W,
                   cell_max_charge_power_W, temp_high_max_power_W, temp_low_max_power_W, max_charge_power_W);
    logging.printf("[MG] DISCHARGE: SoC: %d, Cell: %d, TempH: %d, TempL: %d, Final: %d\n", soc_max_discharge_power_W,
                   cell_max_discharge_power_W, temp_high_max_power_W, temp_low_max_power_W, max_discharge_power_W);
    limit_message_counter = 0;
  }

  if (cellVoltageValidTime > 0) {
    // Decay by one per second
    cellVoltageValidTime--;
  } else {
    // Pause the battery if we haven't received a cell voltage update in a while
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  }

  if (datalayer.system.status.system_status == UPDATING) {
    logging.printf("[MG] rx: %d, tx: %d, cellvtime: %d, vtime: %d\n", rx_count, tx_count, cellVoltageValidTime,
                   voltageValidTime);
    rx_count = 0;
    tx_count = 0;
  }

  if (voltageValidTime > 0) {
    // Decay by one each second.
    voltageValidTime--;
  }
}

void MgGen1Battery::announce_contactor_state(bool state) {
  // Only the primary battery should announce the contactor state
  if (allowed_contactor_closing == nullptr) {
    datalayer.system.status.battery_allows_contactor_closing = state;
  }
}

void MgGen1Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // We start polling with UDS ID 0x7DF, the generic broadcast one. Our first
  // reply will indicate what the BMS-specific one is, which we switch to.
  if (uds_address == 0x7DF && rx_frame.ID == 0x789) {
    setup_uds(0x781, 0);
  } else if (uds_address == 0x7DF && rx_frame.ID == 0x7ED) {
    setup_uds(0x7E5, 0);
  }

  rx_count++;

  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  uint32_t v, i, cell_id, soc2;

  switch (rx_frame.ID) {
    case 0x173:
      // Contains cell min/max voltages
      v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (v > 0 && v < 0x2000) {
        // Is plausible
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
      }

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
        // Reset the voltage timeout counter
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
        // cell temperature is rx_frame.data.u8[1]-40 but we don't use it
      }

      break;
    /*
    Redundant PID checks

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

void MgGen1Battery::got_battery_type(uint32_t type) {
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
  } else if (vehicleHardwareNumber == 0x11054259) {
    // The 50.3kWh LFP and 61kWh NMC MG5 batteries have the same battery type
    // code (the obviously fake 00010203), so we need to distinguish by
    // something else. The vehicle hardware number seems a good candidate.

    // Seen values:
    // 50.3kWh LFP:  11 05 42 59 01
    // 52kWh NMC x2: 10 95 22 20 01
    // 61kWh NMC:    11 01 61 90 01
    // 61kWh NMC:    11 06 01 58 ..

    logging.println("[MG] Detected MG5 50kWh LFP (120s)");
    batteryType = BATTERY_TYPE_MG5_50_LFP;
    maxChargePowerW = 14000;
    maxDischargePowerW = 14000;
    datalayer_battery->info.number_of_cells = 120;
    // Force the chemistry to LFP (for safety)
    datalayer_battery->info.chemistry = LFP;
    datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP_MV;
    datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP_MV;
    if (datalayer_battery->info.total_capacity_Wh == 0) {
      datalayer_battery->info.total_capacity_Wh = 50300;
    }
  } else {
    logging.printf("[MG] Assuming MG5 battery (96s)\n");
    batteryType = BATTERY_TYPE_MG5;
    maxChargePowerW = 14000;
    maxDischargePowerW = 14000;
    datalayer_battery->info.number_of_cells = 96;
    if (datalayer_battery->info.total_capacity_Wh == 0) {
      // It might also be the 61kWh NMC, but the user can override if needed.
      datalayer_battery->info.total_capacity_Wh = 52500;
    }
  }

  datalayer_battery->info.max_design_voltage_dV =
      (datalayer_battery->info.max_cell_voltage_mV * (uint32_t)datalayer_battery->info.number_of_cells) / 100;
  datalayer_battery->info.min_design_voltage_dV =
      (datalayer_battery->info.min_cell_voltage_mV * (uint32_t)datalayer_battery->info.number_of_cells) / 100;
}

uint16_t MgGen1Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {

  if (pid >= 0xF100 || length > 4) {
    logging.printf("[MG] PID %X:", pid);
    for (int i = 0; i < length; i++) {
      logging.printf(" %02X", data[i]);
    }
    logging.printf("\n");
  }

  switch (pid) {
    case POLL_BATTERY_SOH:  // Battery SoH
      datalayer_battery->status.soh_pptt = value;
      break;

    case POLL_BATTERY_VEHICLE_HW_NUMBER:
      if (value == 0) {
        // Retry until we get a valid vehicle hardware number (0 is invalid)
        return POLL_BATTERY_VEHICLE_HW_NUMBER;
      }
      vehicleHardwareNumber = value;
      memcpy(pid_vehicle_hw_number, data,
             length > sizeof(pid_vehicle_hw_number) ? sizeof(pid_vehicle_hw_number) : length);
      break;
    case POLL_BATTERY_TYPE:  // Battery type
      if (value == 0) {
        // Retry until we get a valid battery type (0 is invalid)
        return POLL_BATTERY_TYPE;
      }
      got_battery_type(value);
      memcpy(pid_f18a, data, length > sizeof(pid_f18a) ? sizeof(pid_f18a) : length);
      break;
    case 0xF120:
      memcpy(pid_f120, data, length > sizeof(pid_f120) ? sizeof(pid_f120) : length);
      break;
    case 0xB18C:
      memcpy(pid_b18c, data, length > sizeof(pid_b18c) ? sizeof(pid_b18c) : length);
      break;
    case POLL_BATTERY_FINGERPRINT:
      memcpy(pid_fingerprint, data, length > sizeof(pid_fingerprint) ? sizeof(pid_fingerprint) : length);
      break;
    case POLL_BATTERY_MFR_DATE:
      memcpy(pid_mfr_date, data, length > sizeof(pid_mfr_date) ? sizeof(pid_mfr_date) : length);
      break;
    case POLL_BATTERY_VIN:
      memcpy(pid_vin, data, length > sizeof(pid_vin) ? sizeof(pid_vin) : length);
      break;
    case POLL_BATTERY_SYSTEM_HW_NUMBER:
      memcpy(pid_system_hw_number, data, length > sizeof(pid_system_hw_number) ? sizeof(pid_system_hw_number) : length);
      break;
    case POLL_BATTERY_SYSTEM_SW_NUMBER:
      memcpy(pid_system_sw_number, data, length > sizeof(pid_system_sw_number) ? sizeof(pid_system_sw_number) : length);
      break;
    case 0xF1A2:
      memcpy(pid_f1a2, data, length > sizeof(pid_f1a2) ? sizeof(pid_f1a2) : length);
      break;
    case 0xF1AA:
      memcpy(pid_f1aa, data, length > sizeof(pid_f1aa) ? sizeof(pid_f1aa) : length);
      // Finished reading the static identifiers; switch to steady-state polling.
      set_pid_scan_list(UDS_STEADY_PID_LIST, sizeof(UDS_STEADY_PID_LIST) / sizeof(UDS_STEADY_PID_LIST[0]));
      return UDS_STEADY_PID_LIST[0];  // Jump to the first steady PID (the scan list was just reset)
  }
  return 0;
}

void MgGen1Battery::reset_BMS() {
  start_sequence(MG_STATE_RESET_START);
}

bool MgGen1Battery::supports_reset_BMS() {
  return true;
}

void MgGen1Battery::on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) {
  // Called by the superclass when a response in a UDS sequence is received.
  switch (state) {
    case MG_STATE_RESET_START:
      // Start the reset. First we suppress the inevitable CAN errors that will occur.
      ignore_can_errors_for(can_interface, MG_BMS_RESET_IGNORE_CAN_ERRORS_MS);
      // Then we enter an extended diagnostic session.
      send_sequence_message(MG_STATE_RESET_DIAG, SID::DiagnosticSessionControl, (const uint8_t*)"\x03", 1,
                            MG_UDS_TIMEOUT_SESSION_CONTROL, 2);
      break;
    case MG_STATE_RESET_DIAG:
      // Extended diagnostic session entered, do the reset itself.
      send_sequence_message(MG_STATE_RESET_SEND, SID::ECUReset, (const uint8_t*)"\x01", 1, MG_UDS_TIMEOUT_RESET, 2);
      break;
    case MG_STATE_RESET_SEND:
      logging.println("[MG] UDS ECUReset successful");
      // Pause for a while (to avoid sending any more resets)
      pause_uds(MG_UDS_POST_RESET_PAUSE, UdsPriority::Sequence);
      break;
  }
}

void MgGen1Battery::transmit_can(unsigned long currentMillis) {
  static int8_t send_phase = -1;
  if (++send_phase > 2) {
    send_phase = 0;
  }

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS && send_phase == 0) {
    previousMillis10 = currentMillis;

    tx_count++;

    // It can take up to 30s to establish the cell count. During this period we
    // won't send any contactor-control messages (unless there is a FAULT or
    // inverter requests opening) - if the contactors were already closed,
    // they'll remain so until the BMS times out. This allows us to maintain
    // closed contactors during reboots.
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

    bool send_8a = true;
    if (must_open_contactors || (should_open_contactors && currentMillis > STARTUP_GRACE_PERIOD_MS)) {

      if (announcedContactorsClosed) {
        logging.printf("[MG] Open contactors, iacc: %d, hSCC: %d, bT: %d, accnull: %d, acc: %d, vvt: %d\n",
                       datalayer.system.status.inverter_allows_contactor_closing, highestSeenCellCount, batteryType,
                       allowed_contactor_closing == nullptr,
                       allowed_contactor_closing != nullptr ? *allowed_contactor_closing : 0, voltageValidTime);
        announcedContactorsClosed = false;
      }

      MG_HS_8A.data.u8[5] = 0x00;
      contactorCloseReset = false;
      warmupCounter = 0;

      MG_HS_8A.data.u8[6] = 0x10 | eightAcycle;
    } else if (should_open_contactors) {
      // We are still in the startup grace period - don't send anything, if the
      // contactors are still closed, they can remain so until the battery times
      // out.
      send_8a = false;
    } else {
      // Everything ready, close contactors
      MG_HS_8A.data.u8[5] = 0x02;

      if (!announcedContactorsClosed) {
        logging.printf("[MG] Close contactors, iacc: %d, hSCC: %d, bT: %d, accnull: %d, acc: %d\n",
                       datalayer.system.status.inverter_allows_contactor_closing, highestSeenCellCount, batteryType,
                       allowed_contactor_closing == nullptr,
                       allowed_contactor_closing != nullptr ? *allowed_contactor_closing : 0);
        announcedContactorsClosed = true;
      }

      if (warmupCounter < 1100) {
        // Keep the 1 asserted for 1.1s
        MG_HS_8A.data.u8[6] = 0x10 | eightAcycle;
        warmupCounter += INTERVAL_10_MS;
      } else {
        // After that we go to the 3
        MG_HS_8A.data.u8[6] = 0x30 | eightAcycle;
      }

      if (!contactorCloseReset && (batteryType != BATTERY_TYPE_MG_HS_PHEV)) {
        // MG5/ZS requires DTCs clearing to get contactors to close
        logging.printf("[MG] Resetting DTCs\n");
        reset_DTC();
        contactorCloseReset = true;
      }
    }

    // Basic XOR checksum
    MG_HS_8A.data.u8[7] = (MG_HS_8A.data.u8[0] ^ MG_HS_8A.data.u8[1] ^ MG_HS_8A.data.u8[2] ^ MG_HS_8A.data.u8[3] ^
                           MG_HS_8A.data.u8[4] ^ MG_HS_8A.data.u8[5] ^ MG_HS_8A.data.u8[6]);
    eightAcycle = (eightAcycle + 1) & 0xF;

    if (send_8a) {
      transmit_can_frame(&MG_HS_8A);
    }
  }

  if (currentMillis - previousMillis20 >= INTERVAL_20_MS && send_phase == 1) {
    previousMillis20 = currentMillis;

    transmit_can_frame(&MG_HS_1F1);
  }

  transmit_uds_can(currentMillis);
}

void MgGen1Battery::setup(void) {
  // Setup UDS on the generic broadcast address, we'll switch to the
  // BMS-specific one (varies by battery) when we get a reply.
  setup_uds(0x7DF, 0);
  set_pid_scan_list(UDS_BOOT_PID_LIST, sizeof(UDS_BOOT_PID_LIST) / sizeof(UDS_BOOT_PID_LIST[0]));
  dtc = &datalayer_battery->dtc;

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  announce_contactor_state(false);
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NMC_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NMC_MV;
  datalayer_battery->info.number_of_cells = 90;

  if (datalayer_battery->info.chemistry == battery_chemistry_enum::LFP) {
    datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP_MV;
    datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP_MV;
  }

  // Start off with a wide range until we detect the battery type
  datalayer_battery->info.max_design_voltage_dV = ((uint32_t)108 * MAX_CELL_VOLTAGE_NMC_MV) / 100;
  datalayer_battery->info.min_design_voltage_dV = ((uint32_t)90 * MIN_CELL_VOLTAGE_NMC_MV) / 100;
}
