#include "MG-4-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
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

inline static uint32_t get_working_max_cell_voltage_mV() {
  return datalayer.battery.info.max_cell_voltage_mV - 150;
}
inline static uint32_t get_working_min_cell_voltage_mV() {
  return datalayer.battery.info.min_cell_voltage_mV + 300;
}

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

uint32_t Mg4Battery::calculate_max_discharge_power_W() {
  uint32_t working_min =
      get_working_min_cell_voltage_mV() + (below_working_min ? WORKING_CELL_VOLTAGE_HYSTERESIS_MV : 0);

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

  uint32_t working_max = get_working_max_cell_voltage_mV();

  if (datalayer.battery.status.cell_max_voltage_mV >= working_max) {
    return 0;
  }

  if (datalayer.battery.status.cell_max_voltage_mV > (working_max - 50)) {
    // Taper from MAX down to 0 over the last 50mV
    int32_t mV_above_taper_start = datalayer.battery.status.cell_max_voltage_mV - (working_max - 50);
    int32_t ret = (MAX_CHARGE_POWER_W * (50 - mV_above_taper_start)) / 50;
    return ret > 0 ? ret : 0;
  }

  return MAX_CHARGE_POWER_W;
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

  // cell_voltage_freshness = 10;
  // datalayer.battery.status.cell_min_voltage_mV = 2900;
  // datalayer.battery.status.cell_max_voltage_mV = 2910;
  // datalayer.battery.status.current_dA = 1000;

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

  if (!total_discharge_initialized) {
    // Initialize the total discharge counter based on the min cell voltage
    printf("cell min voltage: %d mV\n", datalayer.battery.status.cell_min_voltage_mV);
    uint16_t initial_soc_centipercent = ocv_to_soc(datalayer.battery.status.cell_min_voltage_mV);
    logging.printf("Initial soc: %d\n", initial_soc_centipercent);
    logging.printf("Capacity mAh: %lu\n", capacity_mAh);

    total_discharge_dC = ((uint64_t)(10000 - initial_soc_centipercent) * capacity_mAh * 36) / 10000;
    logging.printf("Initial total discharge: %lu dC\n", total_discharge_dC);
    total_discharge_initialized = true;
  }

  // Do coulomb count

  if (datalayer.battery.status.current_dA > (int32_t)total_discharge_dC) {
    // We're charging, but discharge is nearly zero - cap at zero
    total_discharge_dC = 0;
  } else {
    // Subtract the (charging) current from the discharge counter
    total_discharge_dC -= datalayer.battery.status.current_dA;
  }

  int32_t soc_centipercent = 10000;
  if (capacity_mAh > 0) {
    // (total_discharge_dC * 10000)
    soc_centipercent = 10000 - (((total_discharge_dC / 9) * 2500) / capacity_mAh);
  }
  if (soc_centipercent > 10000) {
    soc_centipercent = 10000;
  } else if (soc_centipercent < 0) {
    soc_centipercent = 0;
  }

  if (datalayer.battery.status.cell_max_voltage_mV >= (get_working_max_cell_voltage_mV() - 10)) {
    // We're full
    soc_centipercent = 10000;
    total_discharge_dC = 0;
  } else if (datalayer.battery.status.cell_min_voltage_mV <= (get_working_min_cell_voltage_mV() + 10)) {
    // We're empty
    soc_centipercent = 0;
  } else if (soc_centipercent < 100 &&
             datalayer.battery.status.cell_min_voltage_mV > get_working_min_cell_voltage_mV()) {
    // Floor at 1% if we're still not down to the working min
    soc_centipercent = 100;
  } else if (soc_centipercent > 9900 &&
             datalayer.battery.status.cell_max_voltage_mV < get_working_max_cell_voltage_mV()) {
    // Cap at 99% if we're still not up to the working max
    soc_centipercent = 9900;
  }

  update_soc(soc_centipercent);

  if (cell_voltage_freshness > 0)
    cell_voltage_freshness--;
  datalayer.battery.status.max_charge_power_W = calculate_max_charge_power_W();
  datalayer.battery.status.max_discharge_power_W = calculate_max_discharge_power_W();

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
        datalayer.battery.status.voltage_dV = (((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 5) / 2;
        datalayer.battery.status.current_dA = -(((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 20000) / 2;
      } else {
        // Longer FD one
        datalayer.battery.status.voltage_dV = (((rx_frame.data.u8[8] << 4) | (rx_frame.data.u8[9] >> 4)) * 5) / 2;
        datalayer.battery.status.current_dA = -(((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) - 20000) / 2;

        datalayer.battery.status.cell_min_voltage_mV = ((rx_frame.data.u8[30] << 8) | (rx_frame.data.u8[31])) / 8;
        datalayer.battery.status.cell_max_voltage_mV = ((rx_frame.data.u8[32] << 8) | (rx_frame.data.u8[33])) / 8;

        datalayer.battery.status.temperature_max_dC = ((int)rx_frame.data.u8[22] * 5) - 400;
        datalayer.battery.status.temperature_min_dC = ((int)rx_frame.data.u8[23] * 5) - 400;

        cell_voltage_freshness = 10;
      }

      break;
    case 0x401:
      soc_times_ten = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) & 0x3FF;
      if (soc_times_ten <= 1000) {
        //update_soc(soc_times_ten * 10);
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

  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    if (sendPhase == 3 || sendPhase == 13 || sendPhase == 23) {
      // Send a non-FD 4F3 to wake up the PTEXT interface.
      transmit_can_frame(&MG4_4F3);
    }

    // Send the non-FD 047 frame to close contactors on PTEXT.
    MG4_047.data.u8[0] = FOURSEVEN_FIRST_BYTES[sendPhase];
    if (sendPhase >= 0xf) {
      MG4_047.data.u8[1] = sendPhase - 0xf;
    } else {
      MG4_047.data.u8[1] = sendPhase;
    }
    transmit_can_frame(&MG4_047);

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

uint16_t Mg4Battery::handle_pid(uint16_t pid, uint32_t value) {
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
  setup_uds(0x7DF, POLL_BATTERY_VOLTAGE);
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;

  datalayer.battery.info.chemistry = user_selected_battery_chemistry;
  datalayer.battery.info.number_of_cells = 104;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Danger limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_cell_voltage_mV = 3650;
    datalayer.battery.info.min_cell_voltage_mV = 2500;
  } else {
    datalayer.battery.info.max_cell_voltage_mV = 4300;
    datalayer.battery.info.min_cell_voltage_mV = 2800;
  }

  datalayer.battery.info.max_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.max_cell_voltage_mV) / 100;
  datalayer.battery.info.min_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.min_cell_voltage_mV) / 100;
}
