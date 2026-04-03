#include "BYD-ATTO-3-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"

// BYD UDS 0x27 Seed-to-Key Algorithm (Endian-Safe)
uint16_t byd_generate_key(uint16_t seed, uint32_t keyK) {
  // Step 1: XOR mixing
  // By keeping everything in standard integer variables,
  // bitwise shifts act on the logical value, ignoring hardware endianness.
  uint32_t a = seed ^ (seed >> 1);
  uint32_t b = keyK ^ (seed >> 2);

  // Step 2: Calculate the result
  uint32_t result = b ^ (a << 3);

  // Step 3: Return the lower 16 bits
  return (uint16_t)(result & 0xFFFF);
}

uint8_t compute441Checksum(const uint8_t* u8)  // Computes the 441 checksum byte
{
  int sum = 0;
  for (int i = 0; i < 7; ++i) {
    sum += u8[i];
  }
  uint8_t lsb = static_cast<uint8_t>(sum & 0xFF);
  return static_cast<uint8_t>(~lsb & 0xFF);
}

void BydAttoBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  if (battery_voltage > 0) {
    datalayer_battery->status.voltage_dV = battery_voltage * 10;  //Value from periodic CAN data prioritized
  } else if (BMS_voltage > 0) {
    datalayer_battery->status.voltage_dV = BMS_voltage * 10;  //Polled value fallback
  }

  // We assume pack is not crashed, and use periodically transmitted SOC
  datalayer_battery->status.real_soc = battery_highprecision_SOC * 10;

  datalayer_battery->status.soh_pptt = BMS_SOH * 100;

  datalayer_battery->status.current_dA = -BMS_current;

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = BMS_allowed_discharge_power * 100;

  datalayer_battery->status.max_charge_power_W = BMS_allowed_charge_power * 100;

  datalayer_battery->status.cell_max_voltage_mV = BMS_highest_cell_voltage_mV;

  datalayer_battery->status.cell_min_voltage_mV = BMS_lowest_cell_voltage_mV;

  // AC-like top-of-charge taper

  // Tune thresholds here
  const uint16_t V_TAPER_START_mV = 3420;  // begin tapering here
  const uint16_t V_TAPER_END_mV = 3500;    // reach tail current by here (stay below hard clamp region)

  const uint16_t D_TAPER_START_mV = 40;  // begin tapering if delta exceeds this
  const uint16_t D_TAPER_END_mV = 80;    // reach tail current by here

  const uint16_t TAIL_CURRENT_dA = 2;  // 0.2A tail (deci-amps). You can set to 1 for 0.1A.

  // Slew limits to make taper gradual
  const uint16_t DOWN_RATE_dA_per_s = 2;  // ramp down at 0.2A/s  (change to 5 for 0.5A/s)
  const uint16_t UP_RATE_dA_per_s = 1;    // ramp up at 0.1A/s

  const uint16_t cell_max_mV = datalayer_battery->status.cell_max_voltage_mV;
  const uint16_t cell_min_mV = datalayer_battery->status.cell_min_voltage_mV;
  const uint16_t delta_mV = (cell_max_mV > cell_min_mV) ? (cell_max_mV - cell_min_mV) : 0;

  // Start from the lower of: user manual limit, or what the BMS says it will accept.
  // This ensures the taper always honours the BMS allowed charge power, even if it
  // drops to zero (e.g. cell delta cutoff, overvoltage protection).
  uint16_t user_cap_dA = datalayer_battery->settings.max_user_set_charge_dA;
  if (BMS_allowed_charge_power > 0 && datalayer_battery->status.voltage_dV > 0) {
    uint16_t bms_cap_dA = (uint16_t)((uint32_t)BMS_allowed_charge_power * 10 /
                                     datalayer_battery->status.voltage_dV);
    if (bms_cap_dA < user_cap_dA)
      user_cap_dA = bms_cap_dA;
  } else if (BMS_allowed_charge_power == 0) {
    user_cap_dA = 0;
  }
  if (user_cap_dA < TAIL_CURRENT_dA)
    user_cap_dA = TAIL_CURRENT_dA;

  // Compute taper progress 0..1 from voltage and delta; take whichever is "worse".
  auto clamp01 = [](float x) -> float {
    if (x < 0.0f)
      return 0.0f;
    if (x > 1.0f)
      return 1.0f;
    return x;
  };

  float vprog = 0.0f;
  if (cell_max_mV > V_TAPER_START_mV) {
    const uint16_t denom = (V_TAPER_END_mV > V_TAPER_START_mV) ? (V_TAPER_END_mV - V_TAPER_START_mV) : 1;
    vprog = float(cell_max_mV - V_TAPER_START_mV) / float(denom);
  }

  // Gate delta-taper on voltage: only allow cell spread to trigger taper when
  // cell_max is already above V_TAPER_START_mV. This prevents low-SOC spread
  // from incorrectly restricting current.
  float dprog = 0.0f;
  if (cell_max_mV > V_TAPER_START_mV && delta_mV > D_TAPER_START_mV) {
    const uint16_t denom = (D_TAPER_END_mV > D_TAPER_START_mV) ? (D_TAPER_END_mV - D_TAPER_START_mV) : 1;
    dprog = float(delta_mV - D_TAPER_START_mV) / float(denom);
  }

  const float prog = clamp01((vprog > dprog) ? vprog : dprog);

  // Desired current cap (deci-amps): linearly reduce from user_cap -> tail as prog goes 0 -> 1
  uint16_t cap_target_dA = user_cap_dA;
  if (prog > 0.0f) {
    const float span = float(user_cap_dA - TAIL_CURRENT_dA);
    cap_target_dA = uint16_t(float(TAIL_CURRENT_dA) + (1.0f - prog) * span);
    if (cap_target_dA < TAIL_CURRENT_dA)
      cap_target_dA = TAIL_CURRENT_dA;
  }

  // Slew-limit the cap so it changes smoothly over time
  static uint16_t cap_slewed_dA = 0;
  static uint32_t last_ms = 0;
  static bool taper_initialized = false;  // explicit flag avoids cap_slewed_dA starting at 0

  const uint32_t now_ms = (uint32_t)millis64();
  if (!taper_initialized) {
    last_ms = now_ms;
    cap_slewed_dA = user_cap_dA;  // seed slewer at full current, not zero
    taper_initialized = true;
  }

  uint32_t dt_ms = now_ms - last_ms;
  last_ms = now_ms;
  if (dt_ms == 0)
    dt_ms = 1;

  uint32_t down_step = (uint32_t)DOWN_RATE_dA_per_s * dt_ms / 1000;
  uint32_t up_step = (uint32_t)UP_RATE_dA_per_s * dt_ms / 1000;
  if (down_step < 1)
    down_step = 1;
  if (up_step < 1)
    up_step = 1;

  if (cap_target_dA < cap_slewed_dA) {
    const uint16_t diff = cap_slewed_dA - cap_target_dA;
    const uint16_t step = (down_step >= diff) ? diff : (uint16_t)down_step;
    cap_slewed_dA -= step;
  } else if (cap_target_dA > cap_slewed_dA) {
    const uint16_t diff = cap_target_dA - cap_slewed_dA;
    const uint16_t step = (up_step >= diff) ? diff : (uint16_t)up_step;
    cap_slewed_dA += step;
  }

  // Convert current cap (dA) -> power cap (W): P = I(dA) * V(dV) / 100
  const uint32_t power_cap_W = (uint32_t(cap_slewed_dA) * uint32_t(datalayer_battery->status.voltage_dV)) / 100;

  // Apply taper by capping the allowed charge power reported to the rest of BE/inverter logic.
  if (datalayer_battery->status.max_charge_power_W > power_cap_W) {
    datalayer_battery->status.max_charge_power_W = power_cap_W;
  }
  // End taper

  datalayer_battery->status.total_discharged_battery_Wh = BMS_total_discharged_kwh * 1000;
  datalayer_battery->status.total_charged_battery_Wh = BMS_total_charged_kwh * 1000;

  // Count detected cells based on which cell voltage readings are nonzero, up to the max supported by datalayer
  for (uint8_t cell_num = 0; cell_num < MAX_AMOUNT_CELLS; cell_num++) {
    if (battery_cellvoltages[cell_num] > 0) {
      datalayer_battery->info.number_of_cells = cell_num + 1;
    } else {
      break;  // Stop counting at the first zero reading, assuming cells are numbered sequentially from 1
    }
  }

  //Map all cell voltages to the global array
  if (datalayer_battery->info.number_of_cells < MAX_AMOUNT_CELLS) {  //Sanity check
    memcpy(datalayer_battery->status.cell_voltages_mV, battery_cellvoltages,
           datalayer_battery->info.number_of_cells * sizeof(uint16_t));
  }

  //After some time has passed after startup, we assume we have read all cellvoltages at least once, so we can trust the cell count and calculated design voltage limits.
  //Before that, we keep the limits wide to avoid cutting off the battery in case cell count is not read correctly yet.
  if (secondsSinceStartup > 120) {
    //Based on the number of cells, calculate the max and min design voltage of the pack.
    if (datalayer_battery->info.number_of_cells >
        80) {  //Sanity check to avoid setting wrong limits in case cell count is not read correctly
      datalayer_battery->info.max_design_voltage_dV =
          (datalayer_battery->info.number_of_cells * MAX_CELL_VOLTAGE_MV) / 100;
      datalayer_battery->info.min_design_voltage_dV =
          (datalayer_battery->info.number_of_cells * MIN_CELL_VOLTAGE_MV) / 100;
    }
  } else {
    secondsSinceStartup++;
  }

  if ((BMS_lowest_cell_temperature != 0) && (BMS_highest_cell_temperature != 0)) {
    //Avoid triggering high delta if only one of the values is available
    datalayer_battery->status.temperature_min_dC = BMS_lowest_cell_temperature * 10;
    datalayer_battery->status.temperature_max_dC = BMS_highest_cell_temperature * 10;
  }

  // Update webserver datalayer
  if (datalayer_bydatto) {
    datalayer_bydatto->SOC_highprec = battery_highprecision_SOC;
    datalayer_bydatto->SOC_polled = BMS_SOC;
    datalayer_bydatto->voltage_periodic = battery_voltage;
    datalayer_bydatto->voltage_polled = BMS_voltage;
    datalayer_bydatto->battery_temperatures[0] = battery_daughterboard_temperatures[0];
    datalayer_bydatto->battery_temperatures[1] = battery_daughterboard_temperatures[1];
    datalayer_bydatto->battery_temperatures[2] = battery_daughterboard_temperatures[2];
    datalayer_bydatto->battery_temperatures[3] = battery_daughterboard_temperatures[3];
    datalayer_bydatto->battery_temperatures[4] = battery_daughterboard_temperatures[4];
    datalayer_bydatto->battery_temperatures[5] = battery_daughterboard_temperatures[5];
    datalayer_bydatto->battery_temperatures[6] = battery_daughterboard_temperatures[6];
    datalayer_bydatto->battery_temperatures[7] = battery_daughterboard_temperatures[7];
    datalayer_bydatto->battery_temperatures[8] = battery_daughterboard_temperatures[8];
    datalayer_bydatto->battery_temperatures[9] = battery_daughterboard_temperatures[9];
    datalayer_bydatto->battery_temperatures[10] = battery_daughterboard_temperatures[10];
    datalayer_bydatto->battery_temperatures[11] = battery_daughterboard_temperatures[11];
    datalayer_bydatto->battery_temperatures[12] = battery_daughterboard_temperatures[12];
    datalayer_bydatto->BMS_capacity_original_calibration = BMS_capacity_original_calibration;
    datalayer_bydatto->BMC_SOC_original_calibration = BMC_SOC_original_calibration;
    datalayer_bydatto->BMS_capacity_current_calibration = BMS_capacity_current_calibration;
    datalayer_bydatto->BMC_SOC_current_calibration = BMC_SOC_current_calibration;
    datalayer_bydatto->chargePower = BMS_allowed_charge_power;
    datalayer_bydatto->charge_times = BMS_charge_times;
    datalayer_bydatto->dischargePower = BMS_allowed_discharge_power;
    datalayer_bydatto->total_charged_ah = BMS_total_charged_ah;
    datalayer_bydatto->total_discharged_ah = BMS_total_discharged_ah;
    datalayer_bydatto->total_charged_kwh = BMS_total_charged_kwh;
    datalayer_bydatto->total_discharged_kwh = BMS_total_discharged_kwh;
    datalayer_bydatto->times_full_power = BMS_times_full_power;
    datalayer_bydatto->BMS_min_cell_voltage_number = BMS_min_cell_voltage_number;
    datalayer_bydatto->BMS_min_temp_module_number = BMS_min_temp_module_number;
    datalayer_bydatto->BMS_max_cell_voltage_number = BMS_max_cell_voltage_number;
    datalayer_bydatto->BMS_max_temp_module_number = BMS_max_temp_module_number;
    datalayer_bydatto->discharge_status = discharge_status;
    datalayer_bydatto->seed = seed;
    datalayer_bydatto->solvedKey = solvedKey;
    datalayer_bydatto->servicemode = servicemode;

    // Update requests from webserver datalayer
    if (datalayer_bydatto->UserRequestCrashReset && stateMachineClearCrash == NOT_RUNNING) {
      stateMachineClearCrash = STARTED;
      datalayer_bydatto->UserRequestCrashReset = false;
    }

    if (datalayer_bydatto->UserRequestCalibrateSOC && stateMachineCalibrateSOC == NOT_RUNNING) {
      stateMachineCalibrateSOC = STARTED;
      datalayer_bydatto->UserRequestCalibrateSOC = false;
    }
  }
}

void BydAttoBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x244:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == 0x01) {
        battery_temperature_ambient = (rx_frame.data.u8[4] - 40);
      }
      break;
    case 0x286:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x334:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x338:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x344:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      discharge_status = (rx_frame.data.u8[1] & 0x0F);
      break;
    case 0x345:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x347:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x34A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x35E:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x36C:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x438:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43C:
      if (rx_frame.data.u8[0] == 0x00) {  //Mux
        battery_daughterboard_temperatures[0] = (rx_frame.data.u8[1] - 40);
        battery_daughterboard_temperatures[1] = (rx_frame.data.u8[2] - 40);
        battery_daughterboard_temperatures[2] = (rx_frame.data.u8[3] - 40);
        battery_daughterboard_temperatures[3] = (rx_frame.data.u8[4] - 40);
        battery_daughterboard_temperatures[4] = (rx_frame.data.u8[5] - 40);
        battery_daughterboard_temperatures[5] = (rx_frame.data.u8[6] - 40);
      }
      if (rx_frame.data.u8[0] == 0x01) {  //Mux
        //Some packs have unpopulated modules at the end, (0xFF), we dont show those in webserver
        battery_daughterboard_temperatures[6] = (rx_frame.data.u8[1] - 40);
        battery_daughterboard_temperatures[7] = (rx_frame.data.u8[2] - 40);
        battery_daughterboard_temperatures[8] = (rx_frame.data.u8[3] - 40);
        battery_daughterboard_temperatures[9] = (rx_frame.data.u8[4] - 40);
        battery_daughterboard_temperatures[10] = (rx_frame.data.u8[5] - 40);
        battery_daughterboard_temperatures[11] = (rx_frame.data.u8[6] - 40);
      }
      break;
    case 0x43D:  //Cellvoltage monitoring, 54 frames for 160cells
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_frame_index = rx_frame.data.u8[0];

      if (battery_frame_index < (MAX_AMOUNT_CELLS / 3)) {
        uint8_t base_index = battery_frame_index * 3;
        for (uint8_t i = 0; i < 3; i++) {
          uint16_t cell_voltage = (((rx_frame.data.u8[2 * (i + 1)] & 0x0F) << 8) | rx_frame.data.u8[2 * i + 1]);
          if (cell_voltage != 0xFFF) {  //Some packs have unpopulated modules at the end
            battery_cellvoltages[base_index + i] = cell_voltage;
          }
        }
      }
      break;
    case 0x444:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[0];
      BMS_SOH = rx_frame.data.u8[4];
      //battery_temperature_something = rx_frame.data.u8[7] - 40; resides in frame 7
      BMS_voltage_available = true;
      break;
    case 0x445:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x446:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x447:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_highprecision_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4];  // 03 E0 = 992 = 99.2%
      battery_lowest_temperature = (rx_frame.data.u8[1] - 40);                                //Best guess for now
      battery_highest_temperature = (rx_frame.data.u8[3] - 40);                               //Best guess for now
      break;
    case 0x47B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x524:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EF:  //OBD2 PID reply from battery
      if ((rx_frame.data.u8[0] == 0x04) && (rx_frame.data.u8[1] == 0x67) && (rx_frame.data.u8[2] == 0x01)) {
        seed = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        solvedKey = byd_generate_key(seed, 0x63);  //For now key can be either 0xbd or 0x63, 50/50 of guessing right
      }
      if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F)) {
        servicemode = REJECTED;
      }
      if ((rx_frame.data.u8[0] == 0x02) && (rx_frame.data.u8[1] == 0x67) && (rx_frame.data.u8[2] == 0x02) &&
          (rx_frame.data.u8[3] == 0xAA)) {
        servicemode = APPROVED;
      }

      if (rx_frame.data.u8[0] == 0x10) {
        transmit_can_frame(&ATTO_3_7E7_ACK);  //Send next line request
      }
      pid_reply = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      switch (pid_reply) {
        case POLL_FOR_BATTERY_SOC:
          BMS_SOC = rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_VOLTAGE:
          BMS_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CURRENT:
          BMS_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 5000;
          break;
        case POLL_FOR_LOWEST_TEMP_CELL:
          BMS_lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_HIGHEST_TEMP_CELL:
          BMS_highest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_PACK_AVG_TEMP:
          BMS_average_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_CELL_MV_MAX:
          BMS_highest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CELL_MV_MIN:
          BMS_lowest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_ORIGINAL_CALIBRATION:
          BMS_capacity_original_calibration = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
          BMC_SOC_original_calibration = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_CURRENT_CALIBRATION:
          BMS_capacity_current_calibration = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
          BMC_SOC_current_calibration = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_MAX_CHARGE_POWER:
          BMS_allowed_charge_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_CHARGE_TIMES:
          BMS_charge_times = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_MAX_DISCHARGE_POWER:
          BMS_allowed_discharge_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_CHARGED_AH:
          BMS_total_charged_ah = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_DISCHARGED_AH:
          BMS_total_discharged_ah = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_CHARGED_KWH:
          BMS_total_charged_kwh = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_DISCHARGED_KWH:
          BMS_total_discharged_kwh = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TIMES_FULL_POWER:
          BMS_times_full_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_MIN_CELL_VOLTAGE_NUMBER:
          BMS_min_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case POLL_MIN_TEMP_MODULE_NUMBER:
          BMS_min_temp_module_number = rx_frame.data.u8[4];
          break;
        case POLL_MAX_CELL_VOLTAGE_NUMBER:
          BMS_max_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case POLL_MAX_TEMP_MODULE_NUMBER:
          BMS_max_temp_module_number = rx_frame.data.u8[4];
          break;
        default:  //Unrecognized reply
          break;
      }
      break;
    default:
      break;
  }
}

void BydAttoBattery::transmit_can(unsigned long currentMillis) {
  //Send 50ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    // Set close contactors to allowed (Useful for crashed packs, started via contactor control thru GPIO)
    if (allows_contactor_closing) {
      if (datalayer_battery->status.bms_status == ACTIVE) {
        *allows_contactor_closing = true;
      } else {  // Fault state, open contactors!
        *allows_contactor_closing = false;
      }
    }

    counter_50ms++;
    if (counter_50ms > 23) {
      ATTO_3_12D.data.u8[2] = 0x00;  // Goes from 02->00
      ATTO_3_12D.data.u8[3] = 0x22;  // Goes from A0->22
      ATTO_3_12D.data.u8[5] = 0x31;  // Goes from 71->31
    }

    // Update the counters in frame 6 & 7 (they are not in sync)
    if (frame6_counter == 0x0) {
      frame6_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame6_counter--;  // Decrement the counter
    }
    if (frame7_counter == 0x0) {
      frame7_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame7_counter--;  // Decrement the counter
    }

    ATTO_3_12D.data.u8[6] = (0x0F | (frame6_counter << 4));
    ATTO_3_12D.data.u8[7] = (0x09 | (frame7_counter << 4));

    transmit_can_frame(&ATTO_3_12D);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (counter_100ms < 100) {
      counter_100ms++;
    }

    if (counter_100ms > 3) {
      if (BMS_voltage_available) {  // Transmit battery voltage back to BMS when confirmed it's available, this closes the contactors
        ATTO_3_441.data.u8[4] = (uint8_t)(battery_voltage - 1);
        ATTO_3_441.data.u8[5] = ((battery_voltage - 1) >> 8);
        ATTO_3_441.data.u8[6] = 0xFF;
        ATTO_3_441.data.u8[7] = compute441Checksum(ATTO_3_441.data.u8);
      } else {
        ATTO_3_441.data.u8[4] = 0x0C;
        ATTO_3_441.data.u8[5] = 0x00;
        ATTO_3_441.data.u8[6] = 0xFF;
        ATTO_3_441.data.u8[7] = 0x87;
      }
    }

    transmit_can_frame(&ATTO_3_441);

    switch (stateMachineClearCrash) {
      case STARTED:
        // DiagnosticSesssionControl enter extendedDiagnosticSession
        ATTO_3_7E7_CLEAR_CRASH.data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH);
        stateMachineClearCrash = RUNNING_STEP_1;
        break;
      case RUNNING_STEP_1:
        ATTO_3_7E7_CLEAR_CRASH.data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH);
        stateMachineClearCrash = RUNNING_STEP_2;
        break;
      case RUNNING_STEP_2:
        ATTO_3_7E7_CLEAR_CRASH.data = {0x03, 0x19, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH);
        stateMachineClearCrash = NOT_RUNNING;
        break;
      case NOT_RUNNING:
        break;
      default:
        break;
    }
    switch (stateMachineCalibrateSOC) {
      case STARTED:
        // DiagnosticSesssionControl enter extendedDiagnosticSession
        ATTO_3_7E7_RESET_SOC.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineCalibrateSOC = RUNNING_STEP_1;
        break;
      case RUNNING_STEP_1:
        // SecurityAccess requestSeed
        ATTO_3_7E7_RESET_SOC.data = {0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineCalibrateSOC = RUNNING_STEP_2;
        break;
      case RUNNING_STEP_2:
        // SecurityAccess sendKey
        if (solvedKey > 0) {  //Process once we have gotten the solved challenge
          ATTO_3_7E7_RESET_SOC.data = {
              0x04, 0x27, 0x02, (uint8_t)((solvedKey & 0xFF00) >> 8), (uint8_t)(solvedKey & 0x00FF), 0x00, 0x00, 0x00};
          transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
          stateMachineCalibrateSOC = RUNNING_STEP_3;
        } else {
          increaseTimeoutSOC++;
          if (increaseTimeoutSOC > 250) {
            increaseTimeoutSOC = 0;
            stateMachineCalibrateSOC = NOT_RUNNING;
          }
        }
        break;
      case RUNNING_STEP_3:
        // WriteDataByIdentifier dataIdentifier=1F FC (calibrate SOC), data = 10 27 98 3A
        ATTO_3_7E7_RESET_SOC.data = {0x07,
                                     0x2E,
                                     0x1F,
                                     0xFC,
                                     (uint8_t)(datalayer_extended.bydAtto3.calibrationTargetSOC * 100),
                                     (uint8_t)((datalayer_extended.bydAtto3.calibrationTargetSOC * 100) >> 8),
                                     (uint8_t)(datalayer_extended.bydAtto3.calibrationTargetAH * 100),
                                     (uint8_t)((datalayer_extended.bydAtto3.calibrationTargetAH * 100) >> 8)};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineCalibrateSOC = NOT_RUNNING;
        break;
      case NOT_RUNNING:
        break;
      default:
        break;
    }
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (poll_state) {
      case POLL_FOR_BATTERY_SOC:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_SOC & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_SOC & 0x00FF);
        poll_state = POLL_FOR_BATTERY_VOLTAGE;
        break;
      case POLL_FOR_BATTERY_VOLTAGE:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_VOLTAGE & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_VOLTAGE & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CURRENT;
        break;
      case POLL_FOR_BATTERY_CURRENT:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CURRENT & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CURRENT & 0x00FF);
        poll_state = POLL_FOR_LOWEST_TEMP_CELL;
        break;
      case POLL_FOR_LOWEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_LOWEST_TEMP_CELL & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_LOWEST_TEMP_CELL & 0x00FF);
        poll_state = POLL_FOR_HIGHEST_TEMP_CELL;
        break;
      case POLL_FOR_HIGHEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_HIGHEST_TEMP_CELL & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_HIGHEST_TEMP_CELL & 0x00FF);
        poll_state = POLL_FOR_BATTERY_PACK_AVG_TEMP;
        break;
      case POLL_FOR_BATTERY_PACK_AVG_TEMP:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_PACK_AVG_TEMP & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_PACK_AVG_TEMP & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CELL_MV_MAX;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MAX:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CELL_MV_MAX & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CELL_MV_MAX & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CELL_MV_MIN;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MIN:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CELL_MV_MIN & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CELL_MV_MIN & 0x00FF);
        poll_state = POLL_FOR_ORIGINAL_CALIBRATION;
        break;
      case POLL_FOR_ORIGINAL_CALIBRATION:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_ORIGINAL_CALIBRATION & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_ORIGINAL_CALIBRATION & 0x00FF);
        poll_state = POLL_FOR_CURRENT_CALIBRATION;
        break;
      case POLL_FOR_CURRENT_CALIBRATION:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_CURRENT_CALIBRATION & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_CURRENT_CALIBRATION & 0x00FF);
        poll_state = POLL_MAX_CHARGE_POWER;
        break;
      case POLL_MAX_CHARGE_POWER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MAX_CHARGE_POWER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MAX_CHARGE_POWER & 0x00FF);
        poll_state = POLL_CHARGE_TIMES;
        break;
      case POLL_CHARGE_TIMES:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_CHARGE_TIMES & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_CHARGE_TIMES & 0x00FF);
        poll_state = POLL_MAX_DISCHARGE_POWER;
        break;
      case POLL_MAX_DISCHARGE_POWER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MAX_DISCHARGE_POWER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MAX_DISCHARGE_POWER & 0x00FF);
        poll_state = POLL_TOTAL_CHARGED_AH;
        break;
      case POLL_TOTAL_CHARGED_AH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_CHARGED_AH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_CHARGED_AH & 0x00FF);
        poll_state = POLL_TOTAL_DISCHARGED_AH;
        break;
      case POLL_TOTAL_DISCHARGED_AH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_DISCHARGED_AH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_DISCHARGED_AH & 0x00FF);
        poll_state = POLL_TOTAL_CHARGED_KWH;
        break;
      case POLL_TOTAL_CHARGED_KWH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_CHARGED_KWH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_CHARGED_KWH & 0x00FF);
        poll_state = POLL_TOTAL_DISCHARGED_KWH;
        break;
      case POLL_TOTAL_DISCHARGED_KWH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_DISCHARGED_KWH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_DISCHARGED_KWH & 0x00FF);
        poll_state = POLL_TIMES_FULL_POWER;
        break;
      case POLL_TIMES_FULL_POWER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TIMES_FULL_POWER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TIMES_FULL_POWER & 0x00FF);
        poll_state = POLL_MIN_CELL_VOLTAGE_NUMBER;
        break;
      case POLL_MIN_CELL_VOLTAGE_NUMBER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MIN_CELL_VOLTAGE_NUMBER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MIN_CELL_VOLTAGE_NUMBER & 0x00FF);
        poll_state = POLL_MIN_TEMP_MODULE_NUMBER;
        break;
      case POLL_MIN_TEMP_MODULE_NUMBER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MIN_TEMP_MODULE_NUMBER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MIN_TEMP_MODULE_NUMBER & 0x00FF);
        poll_state = POLL_MAX_CELL_VOLTAGE_NUMBER;
        break;
      case POLL_MAX_CELL_VOLTAGE_NUMBER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MAX_CELL_VOLTAGE_NUMBER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MAX_CELL_VOLTAGE_NUMBER & 0x00FF);
        poll_state = POLL_MAX_TEMP_MODULE_NUMBER;
        break;
      case POLL_MAX_TEMP_MODULE_NUMBER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MAX_TEMP_MODULE_NUMBER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MAX_TEMP_MODULE_NUMBER & 0x00FF);
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
      default:
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
    }

    if ((stateMachineClearCrash == NOT_RUNNING) &&
        (stateMachineCalibrateSOC == NOT_RUNNING)) {  //Don't poll battery for data if any diag ongoing
      transmit_can_frame(&ATTO_3_7E7_POLL);
    }
  }
}

void BydAttoBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.chemistry = battery_chemistry_enum::LFP;
  datalayer_battery->info.max_design_voltage_dV = 5000;  //Startup in extremes
  datalayer_battery->info.min_design_voltage_dV = 2500;  //We later determine range based on amount of cells
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
