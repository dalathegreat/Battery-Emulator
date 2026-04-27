#include "MG-4-BATTERY.h"
#include <soc/soc.h>
#include <cmath>    //For unit test
#include <cstring>  //For unit test
//#include "esp_timer.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

static const uint16_t MAX_CHARGE_POWER_W = 12000;
static const uint16_t CHARGE_TRICKLE_POWER_W = 20;
static const uint16_t DERATE_CHARGE_ABOVE_SOC = 9000;  // in 0.01% units

static const uint16_t MAX_DISCHARGE_POWER_W = 12000;
static const uint16_t DERATE_DISCHARGE_BELOW_SOC = 1500;  // in 0.01% units
static const uint16_t DISCHARGE_MIN_SOC = 1000;

static const uint16_t WORKING_CELL_VOLTAGE_HYSTERESIS_MV = 100;

// 101 voltage values from 0% to 100%
static const uint16_t nmc_voltages[] = {
    2500, 3100, 3246, 3336, 3400, 3448, 3468, 3473, 3478, 3483, 3488, 3494, 3501, 3509, 3519, 3529, 3539,
    3549, 3559, 3568, 3576, 3585, 3595, 3601, 3605, 3609, 3612, 3615, 3618, 3621, 3624, 3627, 3629, 3632,
    3635, 3638, 3641, 3643, 3646, 3649, 3652, 3655, 3659, 3662, 3666, 3669, 3673, 3677, 3682, 3686, 3691,
    3697, 3702, 3709, 3717, 3727, 3739, 3750, 3758, 3766, 3773, 3781, 3789, 3797, 3805, 3814, 3823, 3832,
    3841, 3850, 3860, 3869, 3880, 3889, 3900, 3910, 3920, 3930, 3941, 3951, 3962, 3973, 3983, 3994, 4005,
    4017, 4028, 4039, 4051, 4062, 4074, 4086, 4098, 4110, 4122, 4135, 4147, 4160, 4173, 4186, 4200};

static const uint16_t lfp_voltages[] = {
    2700, 2935, 3033, 3097, 3145, 3175, 3191, 3198, 3201, 3202, 3202, 3204, 3206, 3212, 3218, 3224, 3230,
    3235, 3240, 3246, 3250, 3254, 3257, 3260, 3263, 3267, 3270, 3274, 3278, 3282, 3285, 3286, 3286, 3287,
    3288, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289, 3289,
    3290, 3291, 3292, 3292, 3293, 3294, 3296, 3298, 3303, 3312, 3322, 3327, 3327, 3328, 3328, 3328, 3328,
    3328, 3328, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329, 3329,
    3329, 3329, 3329, 3329, 3329, 3330, 3330, 3330, 3331, 3331, 3332, 3332, 3333, 3336, 3354, 3571};

// inline static uint32_t get_working_max_cell_voltage_mV() {
//   //return datalayer.battery.info.max_cell_voltage_mV - 150;
// }
// inline static uint32_t get_working_min_cell_voltage_mV() {
//   //return datalayer.battery.info.min_cell_voltage_mV + 300;
// }

static uint16_t ocv_to_soc(uint16_t voltage_mV) {
  const uint16_t* voltages;
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    voltages = lfp_voltages;
  } else {
    voltages = nmc_voltages;
  }

  if (voltage_mV < voltages[0]) {
    return 0;
  }
  for (size_t i = 1; i < sizeof(nmc_voltages) / sizeof(nmc_voltages[0]); i++) {
    if (voltage_mV < voltages[i]) {
      // Linear interpolation between points
      uint16_t soc_low = (i - 1) * 100;
      uint16_t soc_high = i * 100;
      uint16_t voltage_low = voltages[i - 1];
      uint16_t voltage_high = voltages[i];
      return soc_low + ((voltage_mV - voltage_low) * (soc_high - soc_low)) / (voltage_high - voltage_low);
    }
  }
  return 10000;
}

static uint16_t soc_to_ocv(uint16_t soc_in_centipercent) {
  const uint16_t* voltages;
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    voltages = lfp_voltages;
  } else {
    voltages = nmc_voltages;
  }

  if (soc_in_centipercent >= 10000) {
    return voltages[100];
  }

  uint16_t index = soc_in_centipercent / 100;
  uint16_t remainder = soc_in_centipercent % 100;

  if (remainder == 0) {
    return voltages[index];
  }

  // Linear interpolation between points
  uint16_t voltage_low = voltages[index];
  uint16_t voltage_high = voltages[index + 1];
  return voltage_low + ((voltage_high - voltage_low) * remainder) / 100;
}

uint32_t Mg4Battery::calculate_max_discharge_power_W() {
  uint32_t working_min = working_cell_min_mV + (below_working_min ? WORKING_CELL_VOLTAGE_HYSTERESIS_MV : 0);

  if (cell_voltage_freshness <= 0) {
    return 0;
  }

  if (datalayer.battery.status.cell_min_voltage_mV < working_min) {
    // Below working min, stop discharge
    below_working_min = true;
    return 0;
  } else if (datalayer.battery.status.cell_min_voltage_mV > working_min) {
    // Above working min, discharge allowed
    below_working_min = false;
  }

  return MAX_DISCHARGE_POWER_W;
}

uint32_t Mg4Battery::calculate_max_charge_power_W() {
  if (cell_voltage_freshness <= 0) {
    return 0;
  }

  uint32_t working_max = working_cell_max_mV;  //get_working_max_cell_voltage_mV();

  if (datalayer.battery.status.cell_max_voltage_mV >= working_max) {
    return 0;
  }

  int32_t charge_power_W = MAX_CHARGE_POWER_W;

  if (datalayer.battery.status.cell_max_voltage_mV > (working_max - 50)) {
    // Taper from MAX down to 0 over the last 50mV
    int32_t mV_above_taper_start = datalayer.battery.status.cell_max_voltage_mV - (working_max - 50);
    int32_t power = (MAX_CHARGE_POWER_W * (50 - mV_above_taper_start)) / 50;
    if (power >= 0 && power < charge_power_W) {
      charge_power_W = power;
    }
  }

  if (datalayer.battery.status.real_soc > 9900) {
    // Taper from current power down to CHARGE_TRICKLE_POWER_W between 99% and 100%
    int32_t soc_above_99 = datalayer.battery.status.real_soc - 9900;
    int32_t power =
        CHARGE_TRICKLE_POWER_W + ((MAX_CHARGE_POWER_W - CHARGE_TRICKLE_POWER_W) * (100 - soc_above_99 / 100)) / 100;
    if (power >= 0 && power < charge_power_W) {
      charge_power_W = power;
    }
  }

  return charge_power_W;
}

// uint32_t Mg4Battery::update_pack_max_voltage_limits() {
//   if(cell_voltage_freshness <= 0) {
//     return 0;
//   }

//   uint32_t max_voltage_limit_dV = (get_working_max_cell_voltage_mV() * datalayer.battery.info.number_of_cells) / 100;
//   uint32_t min_voltage_limit_dV = (get_working_min_cell_voltage_mV() * datalayer.battery.info.number_of_cells) / 100;

//   // Calculate the mean cell voltage based on the pack voltage
//   int32_t mean_cell_voltage_mV = (datalayer.battery.status.voltage_dV * 100) / datalayer.battery.info.number_of_cells;

//   // How far is the highest cell above the mean?
//   int32_t deviation_max_mV = datalayer.battery.status.cell_max_voltage_mV - mean_cell_voltage_mV;

//   // Calculate how much to reduce the voltage limit by to account for this
//   // deviation, to avoid overcharging the highest cell.
//   int32_t deviation_reduction_mV = deviation_max_mV * datalayer.battery.info.number_of_cells;
//   if(deviation_reduction_mV > 0) {
//       max_voltage_limit_dV -= deviation_reduction_mV / 100;
//   }
// }

static void update_soc(uint16_t soc_in_centipercent) {
  datalayer.battery.status.real_soc = soc_in_centipercent;

  uint32_t remaining_soc =
      datalayer.battery.status.real_soc > DISCHARGE_MIN_SOC ? datalayer.battery.status.real_soc - DISCHARGE_MIN_SOC : 0;
  uint32_t remaining = (datalayer.battery.info.total_capacity_Wh * remaining_soc) / (10000 - DISCHARGE_MIN_SOC);
  if (remaining > 0) {
    datalayer.battery.status.remaining_capacity_Wh = remaining;
  } else {
    datalayer.battery.status.remaining_capacity_Wh = 0;
  }

  // datalayer.battery.status.max_charge_power_W = taper_charge_power_linear(
  //     datalayer.battery.status.real_soc, MAX_CHARGE_POWER_W, CHARGE_TRICKLE_POWER_W, DERATE_CHARGE_ABOVE_SOC);

  // datalayer.battery.status.max_discharge_power_W = taper_discharge_power_linear(
  //     datalayer.battery.status.real_soc, MAX_DISCHARGE_POWER_W, DISCHARGE_MIN_SOC, DERATE_DISCHARGE_BELOW_SOC);
}

void Mg4Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second

  if (coulombCounting) {
    if (cell_voltage_freshness <= 0) {
      // No valid cell voltage received yet, can't update
      datalayer.battery.status.max_charge_power_W = 0;
      datalayer.battery.status.max_discharge_power_W = 0;
      return;
    }

    uint32_t nominal_cell_voltage_mV = (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) ? 3300 : 3700;
    uint32_t capacity_mAh =
        ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.info.number_of_cells) * 1000000) /
        nominal_cell_voltage_mV;
    uint32_t one_percent_dC = (capacity_mAh * (9 + 1)) / 25;  // 1.11% of capacity in deci-Coulombs

    if (!total_discharge_initialized) {
      if (nonvolatile_cookie != 0 && *nonvolatile_cookie == NONVOLATILE_COOKIE_VALUE) {
        // Non-volatile memory contains a valid previous discharge value, use it
        total_discharge_dC = *nonvolatile_total_discharge_dC;
        logging.printf("[MG4] Restored total discharge from non-volatile memory: %lu dC\n", total_discharge_dC);
      } else {
        // No previous value, Initialize the total discharge counter based on the min cell voltage
        uint16_t initial_soc_centipercent = ocv_to_soc(datalayer.battery.status.cell_min_voltage_mV);
        logging.printf("[MG4] Initial soc: %d\n", initial_soc_centipercent);
        logging.printf("[MG4] Capacity mAh: %lu\n", capacity_mAh);

        total_discharge_dC = ((uint64_t)(10000 - initial_soc_centipercent) * capacity_mAh * 36) / 10000;
      }
      logging.printf("[MG4] Initial total discharge: %lu dC\n", total_discharge_dC);
      total_discharge_initialized = true;
    }

    // 1. Do coulomb count
    if (datalayer.battery.status.current_dA > (int32_t)total_discharge_dC) {
      // We're charging, but discharge is nearly zero - cap at zero
      total_discharge_dC = 0;
    } else {
      // Subtract the (charging) current from the discharge counter
      total_discharge_dC -= datalayer.battery.status.current_dA;
    }

    // 2. State correction to handle drift
    if (datalayer.battery.status.cell_max_voltage_mV >= (working_cell_max_mV - 10)) {
      // We're full
      total_discharge_dC = 0;
    } else if (datalayer.battery.status.cell_max_voltage_mV < working_cell_recharge_threshold_mV &&
               total_discharge_dC < one_percent_dC) {
      // We've drifted - the voltage is low, but the counter is empty.
      // Force charge to 99%-SoC-equivalent to keep the battery charging.
      total_discharge_dC = one_percent_dC;
    }

    // 3. Save to NVRAM
    if (nonvolatile_total_discharge_dC != 0) {
      // Store the total discharged amount in non-volatile memory
      (*nonvolatile_total_discharge_dC) = total_discharge_dC;
      (*nonvolatile_cookie) = NONVOLATILE_COOKIE_VALUE;
    }

    // 4. Calculate SoC Centipercent
    int32_t soc_centipercent = 10000;
    if (capacity_mAh > 0) {
      soc_centipercent = 10000 - ((((uint64_t)total_discharge_dC * 2500) / 9) / capacity_mAh);
    }

    // 5. Cap the SoC to 0-100%
    if (soc_centipercent > 10000) {
      soc_centipercent = 10000;
    } else if (soc_centipercent < 0) {
      soc_centipercent = 0;
    }

    // 6. Handle empty/near-empty case
    if (datalayer.battery.status.cell_min_voltage_mV <= (working_cell_min_mV + 10)) {
      // Battery is empty
      soc_centipercent = 0;
    } else if (soc_centipercent < 100 && datalayer.battery.status.cell_min_voltage_mV > working_cell_min_mV) {
      // Count indicates empty but battery still has charge left, floor at 1%
      soc_centipercent = 100;
    }

    update_soc(soc_centipercent);

    if (cell_voltage_freshness > 0)
      cell_voltage_freshness--;
    datalayer.battery.status.max_charge_power_W = calculate_max_charge_power_W();
    datalayer.battery.status.max_discharge_power_W = calculate_max_discharge_power_W();
  } else {
    if (soc_freshness > 0) {
      soc_freshness--;
    }

    if (soc_freshness <= 0) {
      datalayer.battery.status.max_charge_power_W = 0;
      datalayer.battery.status.max_discharge_power_W = 0;
    } else {
      datalayer.battery.status.max_charge_power_W = taper_charge_power_linear(
          datalayer.battery.status.real_soc, MAX_CHARGE_POWER_W, CHARGE_TRICKLE_POWER_W, DERATE_CHARGE_ABOVE_SOC);

      datalayer.battery.status.max_discharge_power_W = taper_discharge_power_linear(
          datalayer.battery.status.real_soc, MAX_DISCHARGE_POWER_W, DISCHARGE_MIN_SOC, DERATE_DISCHARGE_BELOW_SOC);
    }
  }

  // Taper charge/discharge power at high/low SoC
  // if (datalayer.battery.status.real_soc > 9000) {
  //   datalayer.battery.status.max_charge_power_W =
  //       max_power_W * (1.0f - ((datalayer.battery.status.real_soc - 9000) / 1000.0f));
  // } else {
  //   datalayer.battery.status.max_charge_power_W = max_power_W;
  // }
  // if (datalayer.battery.status.real_soc < 1000) {
  //   datalayer.battery.status.max_discharge_power_W = max_power_W * (datalayer.battery.status.real_soc / 1000.0f);
  // } else {
  //   datalayer.battery.status.max_discharge_power_W = max_power_W;
  // }
}

void Mg4Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  uint32_t soc_times_ten;

  switch (rx_frame.ID) {
    case 0x12C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if (rx_frame.DLC == 8) {
        if (!reportsFDVoltages) {
          datalayer.battery.status.voltage_dV = (((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 5) / 2;
          datalayer.battery.status.current_dA = -(((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 20000) / 2;
        }
      } else {
        // Longer FD one
        datalayer.battery.status.voltage_dV = (((rx_frame.data.u8[8] << 4) | (rx_frame.data.u8[9] >> 4)) * 5) / 2;
        uint16_t current_raw = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
        if (current_raw <= 40000) {
          // Only allow plausible values (-1000A to +1000A)
          datalayer.battery.status.current_dA = -((current_raw - 20000) / 2);
        }

        datalayer.battery.status.cell_min_voltage_mV = ((rx_frame.data.u8[30] << 8) | (rx_frame.data.u8[31])) / 8;
        datalayer.battery.status.cell_max_voltage_mV = ((rx_frame.data.u8[32] << 8) | (rx_frame.data.u8[33])) / 8;

        datalayer.battery.status.temperature_max_dC = ((int)rx_frame.data.u8[19] * 5) - 400;
        datalayer.battery.status.temperature_min_dC = ((int)rx_frame.data.u8[22] * 5) - 400;

        cell_voltage_freshness = 10;
        reportsFDVoltages = true;
      }

      break;
    case 0x159:
      if (rx_frame.DLC > 8) {
        // Cellvoltages/temps are only in the FD message

        // Loop through the subframes in the message
        for (int i = 0; i < rx_frame.DLC; i += 12) {
          uint8_t length = rx_frame.data.u8[i + 3];
          if (length != 8) {
            // Unexpected length, give up
            break;
          }
          // Get the subframe address and data
          uint32_t addr = (rx_frame.data.u8[i] << 16) | (rx_frame.data.u8[i + 1] << 8) | rx_frame.data.u8[i + 2];
          const uint8_t* sub = &rx_frame.data.u8[i + 4];

          if (addr == 0x509 || addr == 0x510) {
            // Cell voltages

            uint8_t mux = sub[7];
            // 0x509 frames cover cells 1-80, 0x510 frames cover cells 81-104
            int celloffset = (addr == 0x509) ? (mux - 1) : 20 + (mux - 1);

            // Unpack the 4 cell voltages
            uint16_t c0 = (sub[0] << 5) | ((sub[1] & 0xF8) >> 3);
            uint16_t c1 = (sub[2] << 5) | ((sub[3] & 0xF8) >> 3);
            uint16_t c2 = ((sub[3] & 0x07) << 10) | (sub[4] << 2) | ((sub[5] & 0xC0) >> 6);
            uint16_t c3 = ((sub[5] & 0x1F) << 8) | sub[6];

            int idx = celloffset * 4;
            if (idx + 3 < MAX_AMOUNT_CELLS) {
              // This seemingly arbitrary ordering is guessed based on the
              // min/max cell indices given in the 12C messages.
              datalayer.battery.status.cell_voltages_mV[idx + 0] = c2;
              datalayer.battery.status.cell_voltages_mV[idx + 1] = c3;
              datalayer.battery.status.cell_voltages_mV[idx + 2] = c1;
              datalayer.battery.status.cell_voltages_mV[idx + 3] = c0;
            }
          } else if (addr == 0x511) {
            // Temps

            uint8_t mux = sub[7];
            int module_idx = mux - 1;

            for (int j = 0; j < 6; j++) {
              int16_t temp_dC = (sub[j + 1] - 40) * 10;  // Convert from -40..215 range to dC
              int idx = (module_idx * 6) + j;
              if (idx < 12) {
                module_temperatures_dC[idx] = temp_dC;
              }
            }

            if (module_temps_received == module_idx) {
              // Record that we have valid temps for this module. Will saturate
              // at 2 once we have them all.
              module_temps_received++;
            }

            if (mux == 2 && module_temps_received == 2) {
              // Update the overall min/max temps based on the module temps once we have them all
              int16_t temp_min_dC = module_temperatures_dC[0];
              int16_t temp_max_dC = module_temperatures_dC[0];
              for (int j = 0; j < 12; j++) {
                if (module_temperatures_dC[j] < temp_min_dC) {
                  temp_min_dC = module_temperatures_dC[j];
                }
                if (module_temperatures_dC[j] > temp_max_dC) {
                  temp_max_dC = module_temperatures_dC[j];
                }
              }
              datalayer.battery.status.temperature_min_dC = temp_min_dC;
              datalayer.battery.status.temperature_max_dC = temp_max_dC;
            }
          }
          // Cell module temps are in 0x511
        }
      }
      break;
    case 0x401:
      soc_times_ten = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) & 0x3FF;

      if (soc_times_ten <= 1000) {
        if (!coulombCounting) {
          update_soc(soc_times_ten * 10);
          soc_freshness = 10;
        }
        reportsSoC = true;
      }

      break;
    default:
      break;
  }
}

static const uint8_t FOURSEVEN_FIRST_BYTES[] = {
    0x81, 0xDC, 0xB4, 0xE9, 0xE8, 0xB5, 0xDD, 0x0F, 0x53, 0x81, 0x66, 0xB4, 0x3A, 0x67, 0x0F,
    0x81, 0x53, 0x3B, 0x66, 0xE8, 0xB5, 0xDD, 0x0F, 0x53, 0x0E, 0x66, 0xB4, 0x3A, 0xE8, 0x0F,
};

static const uint8_t FOURSEVEN_FD_CYCLE_1[] = {0xF4, 0xA9, 0x4E, 0x13, 0x9D, 0xC0, 0x27, 0x7A,
                                               0x26, 0x7B, 0x9C, 0xC1, 0x4F, 0x12, 0xF5};

static const uint8_t FOURSEVEN_FD_CYCLE_2[] = {0x61, 0x3C, 0xDB, 0x86, 0x08, 0x55, 0xB2, 0xEF,
                                               0xB3, 0xEE, 0x09, 0x54, 0xDA, 0x87, 0x60};

void Mg4Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis10 = currentMillis;
    previousMillis200 = currentMillis;
    return;
  }

  auto ptext_transmit = [&](CAN_frame* frame) {
    if (shunt && user_selected_shunt_type == ShuntType::BatterySecondInterface) {
      static_cast<BatterySecondInterfaceShunt*>(shunt)->transmit_can_frame(frame);
    } else {
      transmit_can_frame(frame);
    }
  };

  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    if (sendPhase == 3 || sendPhase == 13 || sendPhase == 23) {
      // Send a non-FD 4F3 to wake up the PTEXT interface.
      ptext_transmit(&MG4_4F3);
    }

    // Send the non-FD 047 frame to close contactors on PTEXT.
    MG4_047.data.u8[0] = FOURSEVEN_FIRST_BYTES[sendPhase];
    if (sendPhase >= 0xf) {
      MG4_047.data.u8[1] = sendPhase - 0xf;
    } else {
      MG4_047.data.u8[1] = sendPhase;
    }
    ptext_transmit(&MG4_047);

    if (sendPhase == 2 || sendPhase == 12 || sendPhase == 22) {
      // Send a FD 4F3 to wake up the FD interface.
      transmit_can_frame(&MG4_4F3_FD);
    }

    sendPhase++;
    if (sendPhase >= 30) {
      sendPhase = 0;
    }
  }

  transmit_uds_can(currentMillis);
}

uint32_t Mg4Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length, UdsStatus status) {
  // Currently unused

  switch (pid) {
    case POLL_BATTERY_VOLTAGE:
      //datalayer.battery.status.voltage_dV = (value * 5) / 2;
      return POLL_BATTERY_CURRENT;
    case POLL_BATTERY_CURRENT:
      //datalayer.battery.status.current_dA = (value - 40000) / -4;
      return POLL_BATTERY_SOC;
    case POLL_BATTERY_SOC:
      // Only use SoC from PIDs if we don't get it from 401 messages.
      if (!reportsSoC) {
        //update_soc(value * 10);
      }
      return POLL_MIN_CELL_TEMPERATURE;
    case POLL_MIN_CELL_TEMPERATURE:
      datalayer.battery.status.temperature_min_dC = ((int32_t)value - 20000) / 50;
      return POLL_MAX_CELL_TEMPERATURE;
    case POLL_MAX_CELL_TEMPERATURE:
      datalayer.battery.status.temperature_max_dC = ((int32_t)value - 20000) / 50;
      break;  // End of cycle
  }
  return 0;  // Continue normal PID cycling
}

void Mg4Battery::setup(void) {  // Performs one time setup at startup
  setup_uds(0x7DF, 0);          //POLL_BATTERY_VOLTAGE);
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;

  datalayer.battery.info.chemistry = user_selected_battery_chemistry;
  datalayer.battery.info.number_of_cells = 104;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Danger limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_cell_voltage_mV = 3700;
    datalayer.battery.info.min_cell_voltage_mV = 2500;
  } else {
    datalayer.battery.info.max_cell_voltage_mV = 4250;
    datalayer.battery.info.min_cell_voltage_mV = 2700;
  }

  working_cell_max_mV = datalayer.battery.info.max_cell_voltage_mV - 150;
  working_cell_min_mV = datalayer.battery.info.min_cell_voltage_mV + 300;
  working_cell_recharge_threshold_mV = working_cell_max_mV - 100;
  coulombCounting = user_selected_use_estimated_SOC;
  if (coulombCounting) {
    static const uint32_t MINIMUM_WORKING_RANGE_MV = 200;
    if (user_selected_max_cell_voltage_mV > (datalayer.battery.info.min_cell_voltage_mV + MINIMUM_WORKING_RANGE_MV) &&
        user_selected_max_cell_voltage_mV <= datalayer.battery.info.max_cell_voltage_mV) {
      working_cell_max_mV = user_selected_max_cell_voltage_mV;
      // Calculate threshold as 1% lower SoC than max, using ocv_to_soc
      working_cell_recharge_threshold_mV = soc_to_ocv(ocv_to_soc(working_cell_max_mV) - 100);
    } else {
      logging.printf("[MG4] Invalid user-selected max cell voltage, using default of %d mV\n", working_cell_max_mV);
    }
    if (user_selected_min_cell_voltage_mV >= datalayer.battery.info.min_cell_voltage_mV &&
        user_selected_min_cell_voltage_mV <= (working_cell_max_mV - MINIMUM_WORKING_RANGE_MV)) {
      working_cell_min_mV = user_selected_min_cell_voltage_mV;
    } else {
      logging.printf("[MG4] Invalid user-selected min cell voltage, using default of %d mV\n", working_cell_min_mV);
    }
    logging.printf("[MG4] Working cell voltage range: %d mV - %d mV, recharge threshold: %d mV\n", working_cell_min_mV,
                   working_cell_max_mV, working_cell_recharge_threshold_mV);
  }

  datalayer.battery.info.max_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.max_cell_voltage_mV) / 100;
  datalayer.battery.info.min_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.min_cell_voltage_mV) / 100;

  // Manually allocate addresses in the 512 bytes of ULP-reserved RTC slow
  // memory for storing the total discharge counter and a cookie to verify its
  // validity. This data survives OTA and software resets (but not hardware
  // resets).

  int nvram_base = (int)SOC_RTC_DATA_LOW + 400;
  if (this == battery2) {
    nvram_base += 16;
  } else if (this == battery3) {
    nvram_base += 32;
  }

  nonvolatile_cookie = (uint32_t*)(nvram_base);
  nonvolatile_total_discharge_dC = (uint32_t*)(nvram_base + 4);
  // logging.printf("Nonvolatile cookie value is %lu\n", *nonvolatile_cookie);
  // // set to a random value
  // *nonvolatile_cookie = esp_timer_get_time();
  // logging.printf("Nonvolatile cookie set to %lu\n", *nonvolatile_cookie);
}
