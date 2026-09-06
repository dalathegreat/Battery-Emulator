#include "BYD-ATTO-3-BATTERY.h"
#include <Arduino.h>  //For millis()
#include <cstring>    //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

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

// Inverted-sum checksum over bytes 0-6, used as byte 7 of both 0x441 and 0x12D
uint8_t computeBydChecksum(const uint8_t* u8) {
  int sum = 0;
  for (int i = 0; i < 7; ++i) {
    sum += u8[i];
  }
  uint8_t lsb = static_cast<uint8_t>(sum & 0xFF);
  return static_cast<uint8_t>(~lsb & 0xFF);
}

// 0x36D work-mode ramp, from a captured session (link voltage 15V -> 418V). The BMS has to SEE the
// ramp to hand over the DC work mode; a settled value alone does not do it. Live pack voltage takes
// over once the last row is reached.
static const uint8_t ATTO_3_36D_RAMP[][7] = {
    {0x0F, 0x00, 0x32, 0x7B, 0x7D, 0x00, 0x47}, {0x93, 0x00, 0x32, 0x7B, 0x7D, 0x00, 0x47},
    {0x15, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47}, {0x5C, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47},
    {0x7C, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47}, {0x8E, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47},
    {0x96, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47}, {0xA0, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47},
    {0xA1, 0x01, 0x32, 0x7B, 0x7D, 0x00, 0x47}, {0xA2, 0x01, 0x32, 0x8A, 0x85, 0x02, 0x47},
};
static const uint8_t ATTO_3_36D_RAMP_ROWS = sizeof(ATTO_3_36D_RAMP) / sizeof(ATTO_3_36D_RAMP[0]);
static const uint8_t ATTO_3_OFFER_CURRENT = 0x1F;  // 15.5A at 0.5A/bit
static const uint8_t ATTO_3_OFFER_POWER = 0x47;    // 7.1kW at 0.1kW/bit

void BydAttoBattery::set_12D_payload(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
  ATTO_3_12D.data.u8[0] = b0;
  ATTO_3_12D.data.u8[1] = b1;
  ATTO_3_12D.data.u8[2] = b2;
  ATTO_3_12D.data.u8[3] = b3;
  ATTO_3_12D.data.u8[4] = b4;
  ATTO_3_12D.data.u8[5] = b5;
}

void BydAttoBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  if (battery_voltage_dV > 0) {
    datalayer_battery->status.voltage_dV = battery_voltage_dV;  //0x438, 0.1V resolution, prioritized
  } else if (battery_voltage > 0) {
    datalayer_battery->status.voltage_dV = battery_voltage * 10;  //0x444 whole-volt fallback
  }

  // We assume pack is not crashed, and use periodically transmitted SOC
  datalayer_battery->status.real_soc = battery_highprecision_SOC * 10;

  datalayer_battery->status.soh_pptt = BMS_SOH * 100;

  datalayer_battery->status.current_dA = -battery_current_dA;

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = BMS_allowed_discharge_power * 100;

  datalayer_battery->status.max_charge_power_W = BMS_allowed_charge_power * 100;

  datalayer_battery->status.cell_max_voltage_mV = BMS_highest_cell_voltage_mV;

  datalayer_battery->status.cell_min_voltage_mV = BMS_lowest_cell_voltage_mV;

  // AC-like top-of-charge taper. A live charge session moves the taper window up to the BMS's own
  // termination band so the pack, not BE, decides when the charge is over.
  const bool native_termination = datalayer_bydatto && datalayer_bydatto->native_termination_enabled;
  const bool session_charging = chargeSessionState == CHG_SESSION_CHARGING;
  // Below the handover point the BMS charge advisory still governs, so a cold or otherwise derated
  // pack is respected; only near the band is the advisory ignored (it collapses ~200mV early there).
  const bool session_owns_power =
      session_charging && datalayer_battery->status.cell_max_voltage_mV >= SESSION_TAPER_START_MV;
  // The artificial taper exists to serve the UDS auto-calibration. With native termination enabled
  // the BMS owns the top of the charge instead, so outside a session only the user limit and the
  // BMS advisory cap the current - same as regen in the car.
  const bool artificial_taper = !native_termination && !session_charging;

  // Tune thresholds here
  const uint16_t V_TAPER_START_mV = session_charging ? SESSION_TAPER_START_MV : 3420;  // begin tapering here
  const uint16_t V_TAPER_END_mV =
      session_charging ? SESSION_TAPER_END_MV : 3500;  // reach tail current by here (stay below hard clamp region)

  const uint16_t D_TAPER_START_mV = 40;  // begin tapering if delta exceeds this
  const uint16_t D_TAPER_END_mV = 80;    // reach tail current by here

  uint16_t tail_current_dA = session_charging ? SESSION_TAIL_CURRENT_dA : 10;  // 1.0A tail (deci-amps)

  // Slew limits to make taper gradual
  const uint16_t DOWN_RATE_dA_per_s = session_charging ? 5 : 2;  // ramp down at 0.2A/s (0.5A/s in a session)
  const uint16_t UP_RATE_dA_per_s = 1;                           // ramp up at 0.1A/s

  const uint16_t cell_max_mV = datalayer_battery->status.cell_max_voltage_mV;
  const uint16_t cell_min_mV = datalayer_battery->status.cell_min_voltage_mV;
  const uint16_t delta_mV = (cell_max_mV > cell_min_mV) ? (cell_max_mV - cell_min_mV) : 0;

  // Start from the user manual limit (deci-amps).
  uint16_t user_cap_dA = datalayer_battery->settings.max_user_set_charge_dA;
  // In the band, hold to what a real AC charger could deliver: that is the approach rate every
  // captured native termination happened at. Never deliver above the transmitted 0x47E current
  // offer either (0.5A per bit).
  if (session_owns_power) {
    if (datalayer_battery->status.voltage_dV > 0) {
      const uint16_t obc_cap_dA = (uint16_t)(SESSION_OBC_CAP_W * 100 / datalayer_battery->status.voltage_dV);
      if (user_cap_dA > obc_cap_dA)
        user_cap_dA = obc_cap_dA;
    }
    const uint16_t offer_cap_dA = (uint16_t)ATTO_3_OFFER_CURRENT * 5;
    if (user_cap_dA > offer_cap_dA)
      user_cap_dA = offer_cap_dA;
  }
  // The user limit is sovereign: a cap below the tail lowers the tail, never the other way round.
  if (tail_current_dA > user_cap_dA)
    tail_current_dA = user_cap_dA;

  // Compute taper progress 0..1 from voltage and delta; take whichever is "worse".
  auto clamp01 = [](float x) -> float {
    if (x < 0.0f)
      return 0.0f;
    if (x > 1.0f)
      return 1.0f;
    return x;
  };

  float vprog = 0.0f;
  if ((session_charging || artificial_taper) && cell_max_mV > V_TAPER_START_mV) {
    const uint16_t denom = (V_TAPER_END_mV > V_TAPER_START_mV) ? (V_TAPER_END_mV - V_TAPER_START_mV) : 1;
    vprog = float(cell_max_mV - V_TAPER_START_mV) / float(denom);
  }

  // Gate delta-taper on voltage: only allow cell spread to trigger taper when
  // cell_max is already above V_TAPER_START_mV. This prevents low-SOC spread
  // from incorrectly restricting current.
  // Spread is expected to open up at the top of a session (real cars run 250-360mV there), so the
  // delta taper would fight the approach to the band. The BMS owns the top in a session.
  float dprog = 0.0f;
  if (artificial_taper && cell_max_mV > V_TAPER_START_mV && delta_mV > D_TAPER_START_mV) {
    const uint16_t denom = (D_TAPER_END_mV > D_TAPER_START_mV) ? (D_TAPER_END_mV - D_TAPER_START_mV) : 1;
    dprog = float(delta_mV - D_TAPER_START_mV) / float(denom);
  }

  const float prog = clamp01((vprog > dprog) ? vprog : dprog);

  // Desired current cap (deci-amps): linearly reduce from user_cap -> tail as prog goes 0 -> 1
  uint16_t cap_target_dA = user_cap_dA;
  if (prog > 0.0f) {
    const float span = float(user_cap_dA - tail_current_dA);
    cap_target_dA = uint16_t(float(tail_current_dA) + (1.0f - prog) * span);
    if (cap_target_dA < tail_current_dA)
      cap_target_dA = tail_current_dA;
  }

  // Slew-limit the cap so it changes smoothly over time
  const uint32_t now_ms = (uint32_t)millis64();
  if (!taper_initialized) {
    taper_last_ms = now_ms;
    cap_slewed_dA = user_cap_dA;  // seed slewer at full current, not zero
    taper_initialized = true;
  }

  uint32_t dt_ms = now_ms - taper_last_ms;
  taper_last_ms = now_ms;
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
  // Step to the OBC cap at the handover point instead of slewing down to it - the offer on the wire
  // says 7kW from here, so deliver no more than that.
  if (session_owns_power && cap_slewed_dA > user_cap_dA) {
    cap_slewed_dA = user_cap_dA;
  }

  // The BMS recalibrates SOC itself at a native termination, so the UDS write stands down while
  // native termination is enabled. Turning the feature off restores it immediately.
  const bool crit_taper = !native_termination && (prog >= 0.95f && cap_slewed_dA <= tail_current_dA);
  handle_auto_soc_calibration(crit_taper, dt_ms, now_ms);

  // Convert current cap (dA) -> power cap (W): P = I(dA) * V(dV) / 100
  const uint32_t power_cap_W = (uint32_t(cap_slewed_dA) * uint32_t(datalayer_battery->status.voltage_dV)) / 100;

  // Apply taper by capping the allowed charge power reported to the rest of BE/inverter logic
  if (session_owns_power) {
    // Near the band the taper is the only authority: the BMS advisory zeroes about 200mV early and
    // a real charger never reads it. The session ends on the BMS grant instead.
    datalayer_battery->status.max_charge_power_W = power_cap_W;
  } else if (datalayer_battery->status.max_charge_power_W > power_cap_W) {
    datalayer_battery->status.max_charge_power_W = power_cap_W;
  }
  // End taper

  // Lift the cell rails only once the BMS has actually granted the charge (and through the
  // terminated rest), so it can reach its own 3742-3753mV termination band. Everywhere else,
  // including a session still waiting for a grant, the stock limits are in force.
  // Rails follow the pack, not the session: an open leaves the cells where the BMS left them.
  const uint16_t rails_cell_max_mV = datalayer_battery->status.cell_max_voltage_mV;
  const uint16_t rails_cell_min_mV = datalayer_battery->status.cell_min_voltage_mV;
  if (chargeTerminatedRails && rails_cell_min_mV > 0 && rails_cell_max_mV <= MAX_CELL_VOLTAGE_MV &&
      (rails_cell_max_mV - rails_cell_min_mV) <= MAX_CELL_DEVIATION_MV) {
    chargeTerminatedRails = false;
  }
  const bool rails_owned = chargeSessionState == CHG_SESSION_CHARGING || chargeSessionState == CHG_SESSION_FINISHING ||
                           (chargeSessionState == CHG_SESSION_HOLD && chargeSessionTerminated) || chargeTerminatedRails;
  // Turning native termination off restores the stock limits immediately.
  const bool session_owns_top = rails_owned && datalayer_bydatto && datalayer_bydatto->native_termination_enabled;
  datalayer_battery->info.max_cell_voltage_mV = session_owns_top ? SESSION_CELL_CLAMP_MV : MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV =
      session_owns_top ? SESSION_DELTA_LIMIT_MV : MAX_CELL_DEVIATION_MV;

  // Stop charging the moment the battery withdraws its grant, and keep it stopped afterwards: the
  // cells relax back below the band within minutes, which would otherwise restart the charge. The
  // hold is released by a real discharge (handle_charge_session).
  if (chargeDonePending || (chargeSessionTerminated && !chargeRearmAllowed)) {
    datalayer_battery->status.max_charge_power_W = 0;
  }

  // Hold power at zero until the pack confirms closed (0x344 bit7), and while opening/idle
  if (!(contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) ||
      (contactorState != CONTACTORS_CLOSING && contactorState != CONTACTORS_ACTIVE)) {
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  }

  // Pack-internal contactors: DC bus is live once the pack confirms the main contactor
  // closed (same 0x344 bit7 feedback used to gate power above). Guarded so the GPIO
  // contactor state machine stays authoritative when enabled.
  if (!contactor_control_enabled) {
    datalayer.system.status.dc_bus_live = (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) != 0;
  }

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
          (datalayer_battery->info.number_of_cells * datalayer_battery->info.max_cell_voltage_mV) / 100;
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

  if (battery_insulation_valid) {
    // BMS reports insulation in Ohm per Volt of pack voltage. Convert to kOhm.
    datalayer_battery->status.insulation_resistance_kOhm =
        (uint16_t)(((uint32_t)battery_insulation_ohm_per_volt * datalayer_battery->status.voltage_dV) / 10000u);
    datalayer_battery->status.insulation_resistance_available = true;
  }

  // Update webserver datalayer
  if (datalayer_bydatto) {
    datalayer_bydatto->SOC_highprec = battery_highprecision_SOC;
    datalayer_bydatto->SOC_polled = BMS_SOC;
    //Resolved voltage, not raw 0x438, so the page matches the main view when 0x438 is rejected
    datalayer_bydatto->pack_voltage_dV = datalayer_battery->status.voltage_dV;
    datalayer_bydatto->insulation_ohm_per_volt = battery_insulation_ohm_per_volt;
    datalayer_bydatto->insulation_valid = battery_insulation_valid;
    datalayer_bydatto->iso_status_valid = (last_35E_ms != 0) && ((millis() - last_35E_ms) < 3000);
    datalayer_bydatto->iso_measurement_active = battery_iso_measurement_active;
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
    // Pre-fill from BMS on first read so the web page shows real pack AH, not the 150AH default
    if (!calibrationAH_seeded && BMS_capacity_current_calibration > 0) {
      datalayer_bydatto->calibrationTargetAH = BMS_capacity_current_calibration / 100;
      calibrationAH_seeded = true;
    }
    datalayer_bydatto->charge_session_state = chargeSessionState;
    datalayer_bydatto->charge_grant = chargeGrant;
    datalayer_bydatto->charge_session_seconds =
        (chargeSessionState == CHG_SESSION_IDLE) ? 0 : (uint32_t)((millis() - chargeSessionEntryMillis) / 1000);
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
    datalayer_bydatto->contactor_control_state = contactorState;
    datalayer_bydatto->contactor_feedback = contactor_feedback;
    datalayer_bydatto->contactor_main_closed = (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) != 0;
    datalayer_bydatto->contactor_precharging = (contactor_feedback & BMS_FEEDBACK_PRECHARGING) != 0;
    datalayer_bydatto->contactor_hv_active = (contactor_feedback & BMS_FEEDBACK_HV_ACTIVE) != 0;
    datalayer_bydatto->contactor_drive_flag = (contactor_feedback & BMS_FEEDBACK_DRIVE_FLAG) != 0;
    datalayer_bydatto->contactor_charge_flag = (contactor_feedback & BMS_FEEDBACK_CHARGE_FLAG) != 0;

    // Update requests from webserver datalayer. All 0x7E7 diagnostics share the request/reply IDs,
    // so only one may run at a time; DTC reception counts as busy too.
    bool diag_busy = (stateMachineClearCrash != NOT_RUNNING) || (stateMachineCalibrateSOC != NOT_RUNNING) ||
                     (stateMachineReadDTC != NOT_RUNNING) || (stateMachineEraseDTC != NOT_RUNNING) ||
                     (stateMachineIsoRoutine != NOT_RUNNING) || dtc_rx_active ||
                     datalayer_bydatto->dtc_read_in_progress || cell_balance_time_requested.load();

    if (datalayer_bydatto->UserRequestCrashReset && !diag_busy) {
      stateMachineClearCrash = STARTED;
      datalayer_bydatto->UserRequestCrashReset = false;
      diag_busy = true;
    }

    if (datalayer_bydatto->UserRequestCalibrateSOC && !diag_busy) {
      stateMachineCalibrateSOC = STARTED;
      datalayer_bydatto->UserRequestCalibrateSOC = false;
      diag_busy = true;
    }

    if (datalayer_bydatto->UserRequestDTCreadout && !diag_busy) {
      stateMachineReadDTC = STARTED;
      datalayer_bydatto->dtc_read_in_progress = true;
      datalayer_battery->dtc.dtc_read_failed = false;
      dtc_request_millis = millis();
      datalayer_bydatto->UserRequestDTCreadout = false;
      diag_busy = true;
    }

    if (datalayer_bydatto->UserRequestDTCreset && !diag_busy) {
      stateMachineEraseDTC = STARTED;
      datalayer_bydatto->UserRequestDTCreset = false;
      diag_busy = true;
    }

    if ((datalayer_bydatto->UserRequestIsoRoutineDisable || datalayer_bydatto->UserRequestIsoRoutineEnable) &&
        !diag_busy) {
      isoRoutineAction = datalayer_bydatto->UserRequestIsoRoutineDisable ? 1 : 2;
      datalayer_bydatto->iso_command_status = 1;  // running
      increaseTimeoutIso = 0;
      stateMachineIsoRoutine = STARTED;
      datalayer_bydatto->UserRequestIsoRoutineDisable = false;
      datalayer_bydatto->UserRequestIsoRoutineEnable = false;
      diag_busy = true;
    }

    // keep_iso_disabled: the monitor re-enables on every BMS power-up, so re-send disable after each
    // BMS start (arm on the CAN alive edge, retry until accepted).
    bool bms_alive = (lastContactorFeedbackMillis != 0) && ((millis() - lastContactorFeedbackMillis) < 3000);
    if (bms_alive && !bms_was_alive) {
      bms_alive_since_ms = millis();
      if (datalayer_bydatto->keep_iso_disabled) {
        iso_reassert_needed = true;
        iso_reassert_attempt_ms = 0;
      }
    }
    bms_was_alive = bms_alive;
    // Also re-arms ~30s after a contactor open, which is no BMS start: catch it on the 0x35E edge.
    if (battery_iso_measurement_active && !iso_measurement_was_active && datalayer_bydatto->keep_iso_disabled) {
      iso_reassert_needed = true;
      iso_reassert_attempt_ms = 0;
    }
    iso_measurement_was_active = battery_iso_measurement_active;
    if (!datalayer_bydatto->keep_iso_disabled) {
      iso_reassert_needed = false;
    }
    // Disable early and retry fast to beat the BMS's boot-time insulation trip.
    if (iso_reassert_needed && bms_alive && (millis() - bms_alive_since_ms > 1000) &&
        (iso_reassert_attempt_ms == 0 || (millis() - iso_reassert_attempt_ms) > 2000) && !diag_busy) {
      isoRoutineAction = 1;  // disable
      datalayer_bydatto->iso_command_status = 1;
      increaseTimeoutIso = 0;
      stateMachineIsoRoutine = STARTED;
      iso_reassert_attempt_ms = millis();
      diag_busy = true;
    }
    if (iso_reassert_needed && iso_reassert_attempt_ms != 0 && datalayer_bydatto->iso_command_status == 2) {
      iso_reassert_needed = false;  // accepted, monitor disabled until the next BMS restart
    }
    // Fail the read if the BMS never answers
    if (datalayer_bydatto->dtc_read_in_progress && (millis() - dtc_request_millis > 2000)) {
      datalayer_bydatto->dtc_read_in_progress = false;
      datalayer_battery->dtc.dtc_read_failed = true;
      dtc_rx_active = false;
    }
  }
}

void BydAttoBattery::handle_auto_soc_calibration(bool crit_taper, uint32_t dt_ms, uint32_t now_ms) {
  if (!datalayer_bydatto)
    return;
  const uint32_t TAIL_DWELL_REQUIRED_MS = 10UL * 60UL * 1000UL;
  const uint32_t CURRENT_SPIKE_GRACE_MS = 60UL * 1000UL;

  const int16_t current_dA = datalayer_battery->status.current_dA;
  const bool crit_low_current = (current_dA >= -5 &&  // discharge up to 0.5A
                                 current_dA <= 30);   // charge up to 3A

  if (!crit_taper) {
    autocal_dwell_ms = 0;
    autocal_grace_start_ms = 0;
  } else if (crit_low_current) {
    autocal_grace_start_ms = 0;
    autocal_dwell_ms += dt_ms;
    if (autocal_dwell_ms > TAIL_DWELL_REQUIRED_MS)
      autocal_dwell_ms = TAIL_DWELL_REQUIRED_MS;
  } else {
    if (autocal_grace_start_ms == 0) {
      autocal_grace_start_ms = now_ms;
    }
    if ((now_ms - autocal_grace_start_ms) >= CURRENT_SPIKE_GRACE_MS) {
      autocal_dwell_ms = 0;
      autocal_grace_start_ms = 0;
    }
  }

  const uint64_t now64 = millis64();

  const bool crit_dwell = (autocal_dwell_ms >= TAIL_DWELL_REQUIRED_MS);
  const bool crit_drift =
      (battery_highprecision_SOC < 1000 &&
       (1000 - battery_highprecision_SOC) > (uint16_t)(datalayer_bydatto->auto_calibrate_soc_drift_percent * 10));
  const bool crit_cooldown = ((now64 - last_auto_calibrate_ms) > 3600000ULL);
  // Only calibrate when the pack itself reports closed, not just when BE permits closing
  const bool crit_contactors = (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) != 0;
  uint32_t current_spike_ms = 0;
  if (crit_taper && !crit_low_current && autocal_grace_start_ms != 0) {
    current_spike_ms = now_ms - autocal_grace_start_ms;
  }

  datalayer_bydatto->autocal_crit_taper = crit_taper;
  datalayer_bydatto->autocal_crit_low_current = crit_low_current;
  datalayer_bydatto->autocal_dwell_accumulated_ms = autocal_dwell_ms;
  datalayer_bydatto->autocal_grace_timer_ms = current_spike_ms;
  datalayer_bydatto->autocal_drift_percent =
      (battery_highprecision_SOC < 1000) ? (float)(1000 - battery_highprecision_SOC) / 10.0f : 0.0f;
  datalayer_bydatto->autocal_current_dA = current_dA;
  datalayer_bydatto->autocal_crit_dwell = crit_dwell;
  datalayer_bydatto->autocal_crit_drift = crit_drift;
  datalayer_bydatto->autocal_crit_cooldown_ready = crit_cooldown;
  datalayer_bydatto->autocal_crit_contactors = crit_contactors;

  if (datalayer_bydatto->auto_calibrate_soc_enabled &&
      !datalayer_bydatto->UserRequestCalibrateSOC &&  // don't fight manual request
      stateMachineCalibrateSOC == NOT_RUNNING && crit_contactors && crit_taper && crit_low_current && crit_dwell &&
      crit_drift && crit_cooldown) {

    set_event(EVENT_BYD_AUTO_SOC_CALIBRATION, (uint8_t)((1000 - battery_highprecision_SOC) / 10));

    datalayer_bydatto->calibrationTargetSOC = 100;
    if (BMS_capacity_current_calibration > 0) {  // guard against startup zero
      datalayer_bydatto->calibrationTargetAH = BMS_capacity_current_calibration / 100;
    }
    datalayer_bydatto->UserRequestCalibrateSOC = true;

    last_auto_calibrate_ms = now64;
    autocal_dwell_ms = 0;
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
      contactor_feedback = rx_frame.data.u8[0];
      lastContactorFeedbackMillis = millis();
      if ((rx_frame.data.u8[1] & 0x40) && !(contactor_feedback_state & 0x40)) {
        prechargeRampStartMillis = millis();  // BMS started precharging
        prechargeEdgeSeen = true;
      }
      contactor_feedback_state = rx_frame.data.u8[1];
      discharge_status = (rx_frame.data.u8[1] & 0x0F);
      break;
    case 0x345:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      chargeGrantMirror = rx_frame.data.u8[4];  // mirrors 0x347 byte 1, typically a frame ahead of it
      chargeGrantMirrorMillis = millis();
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        BMS_allowed_discharge_power = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];  // 0.1kW, same as DID 0x000E
        BMS_allowed_charge_power = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];     // 0.1kW, same as DID 0x000A
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x347:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      chargeGrant = rx_frame.data.u8[1];
      handle_charge_grant(rx_frame.data.u8[1]);
      break;
    case 0x34A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x35E:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // b0 bit 0x80 = isolation measurement active (set = running, clear = disabled/idle)
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        battery_iso_measurement_active = (rx_frame.data.u8[0] & 0x80) != 0;
        last_35E_ms = millis();
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x360:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x36C:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x438:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        //b5..b6 is not pack voltage on every pack family (142S decodes to thousands of volts).
        //Only a resolution upgrade on 0x444, so adopt it once seen and the two agree.
        const uint16_t candidate_dV = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[5];
        if (BMS_voltage_available) {
          const uint16_t reference_dV = battery_voltage * 10;
          const uint16_t deviation_dV =
              (candidate_dV > reference_dV) ? (candidate_dV - reference_dV) : (reference_dV - candidate_dV);
          battery_voltage_dV = (deviation_dV <= VOLTAGE_CROSSCHECK_TOLERANCE_DV) ? candidate_dV : 0;
        }
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x43A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        battery_insulation_ohm_per_volt = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
        battery_insulation_valid = true;
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
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
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        battery_voltage = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[0];
        battery_current_dA = (int16_t)(((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 5000);
        lastCurrentSampleMillis = millis();
        BMS_SOH = rx_frame.data.u8[4];
        BMS_SOC = rx_frame.data.u8[5];  // Whole-percent SOC, same basis as DID 0x0005 (not the 0x447 basis)
        BMS_voltage_available = true;
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x445:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x446:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // Broadcast cell extrema summary (~1Hz), fresher than the UDS round-robin. b0/b4 = 1-based
      // min/max cell number; min/max mV are 12-bit little-endian in b1:b2 / b5:b6.
      BMS_min_cell_voltage_number = rx_frame.data.u8[0];
      BMS_lowest_cell_voltage_mV = rx_frame.data.u8[1] | ((rx_frame.data.u8[2] & 0x0F) << 8);
      BMS_max_cell_voltage_number = rx_frame.data.u8[4];
      BMS_highest_cell_voltage_mV = rx_frame.data.u8[5] | ((rx_frame.data.u8[6] & 0x0F) << 8);
      break;
    case 0x447:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[7] == computeBydChecksum(rx_frame.data.u8)) {
        BMS_min_temp_module_number = rx_frame.data.u8[0];  // 1-based coldest sensor
        BMS_lowest_cell_temperature = (rx_frame.data.u8[1] - 40);
        BMS_max_temp_module_number = rx_frame.data.u8[2];  // 1-based hottest sensor
        BMS_highest_cell_temperature = (rx_frame.data.u8[3] - 40);
        battery_highprecision_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4];  // 03 E0 = 992 = 99.2%
        BMS_average_cell_temperature = (rx_frame.data.u8[6] - 40);
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x47B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x524:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EF:  //OBD2 PID reply from battery
      // DTC reply (0x19 0x02 -> 0x59 0x02) is multi-frame. Reassemble it, ACK the first frame with
      // our own flow control, and parse once complete. Done before the single-frame PID parsing so
      // the consecutive frames aren't mistaken for PIDs.
      if (rx_frame.data.u8[0] == 0x10 && rx_frame.data.u8[2] == 0x59 && rx_frame.data.u8[3] == 0x02) {
        dtc_rx_expected = ((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
        dtc_rx_len = 0;
        for (uint8_t i = 2; i < 8 && dtc_rx_len < sizeof(dtc_buffer); i++) {
          dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
        }
        dtc_rx_active = true;
        transmit_can_frame(&ATTO_3_7E7_DTC_FC);
        break;
      }
      if (dtc_rx_active && (rx_frame.data.u8[0] & 0xF0) == 0x20) {
        for (uint8_t i = 1; i < 8 && dtc_rx_len < sizeof(dtc_buffer); i++) {
          dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
        }
        if (dtc_rx_len >= dtc_rx_expected) {
          dtc_rx_len = dtc_rx_expected;  // Drop ISO-TP padding so it isn't parsed as a DTC
          parseDTCResponse();
          dtc_rx_active = false;
        }
        break;
      }
      if ((rx_frame.data.u8[0] == 0x04) && (rx_frame.data.u8[1] == 0x67) && (rx_frame.data.u8[2] == 0x01)) {
        seed = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        solvedKey = byd_generate_key(seed, 0x63);  //For now key can be either 0xbd or 0x63, 50/50 of guessing right
      }
      if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F) &&
          !(awaiting_cell_balance_reply() && rx_frame.data.u8[2] == 0x22)) {
        servicemode = REJECTED;
      }
      if ((rx_frame.data.u8[0] == 0x02) && (rx_frame.data.u8[1] == 0x67) && (rx_frame.data.u8[2] == 0x02) &&
          (rx_frame.data.u8[3] == 0xAA)) {
        servicemode = APPROVED;
      }

      // ISO routine reply (only while it is running): 71 = accepted; any 7F = rejected (session,
      // security, or routine NRC). Ends the machine on the reply.
      if (stateMachineIsoRoutine != NOT_RUNNING) {
        if (rx_frame.data.u8[1] == 0x71 && rx_frame.data.u8[3] == 0x20 && rx_frame.data.u8[4] == 0x08) {
          datalayer_bydatto->iso_command_status = 2;  // routine accepted
          stateMachineIsoRoutine = NOT_RUNNING;
        } else if (rx_frame.data.u8[1] == 0x7F) {
          datalayer_bydatto->iso_command_status = 3;  // NRC
          stateMachineIsoRoutine = NOT_RUNNING;
        }
      }

      if (rx_frame.data.u8[0] == 0x10) {
        transmit_can_frame(&ATTO_3_7E7_ACK);  //Send next line request
      }

      if (handle_cell_balance_time_reply(rx_frame)) {
        break;
      }

      pid_reply = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      switch (pid_reply) {
        case POLL_FOR_ORIGINAL_CALIBRATION:
          BMS_capacity_original_calibration = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
          BMC_SOC_original_calibration = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_CURRENT_CALIBRATION:
          BMS_capacity_current_calibration = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
          BMC_SOC_current_calibration = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_CHARGE_TIMES:
          BMS_charge_times = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
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
          BMS_times_full_power_valid = true;
          break;
        default:  //Unrecognized reply
          break;
      }
      break;
    default:
      break;
  }
}

// Parse a reassembled UDS ReadDTCInformation (0x59 0x02) reply: 3 header bytes (59 02 mask) then
// 4 bytes per DTC (3-byte code + status). Stores raw codes; HTML decodes them to strings.
void BydAttoBattery::parseDTCResponse() {
  if (dtc_buffer[0] != 0x59 || dtc_buffer[1] != 0x02) {
    datalayer_battery->dtc.dtc_read_failed = true;
    datalayer_bydatto->dtc_read_in_progress = false;
    return;
  }
  uint8_t count = 0;
  for (uint16_t off = 3; (off + 4 <= dtc_rx_len) && (count < MAX_DTC_COUNT); off += 4) {
    uint32_t code =
        ((uint32_t)dtc_buffer[off] << 16) | ((uint32_t)dtc_buffer[off + 1] << 8) | (uint32_t)dtc_buffer[off + 2];
    uint8_t status = dtc_buffer[off + 3];
    if (code == 0 || status == 0) {  // Empty slot or cleared DTC
      continue;
    }
    datalayer_battery->dtc.dtc_codes[count] = code;
    datalayer_battery->dtc.dtc_status[count] = status;
    count++;
  }
  // Display order: active (status bit0) first, then ascending code.
  uint32_t* codes = datalayer_battery->dtc.dtc_codes;
  uint8_t* sts = datalayer_battery->dtc.dtc_status;
  for (uint8_t a = 1; a < count; a++) {
    uint32_t c = codes[a];
    uint8_t s = sts[a];
    int b = a - 1;
    while (b >= 0) {
      bool keyActive = (s & 0x01);
      bool bActive = (sts[b] & 0x01);
      bool keyFirst = (keyActive != bActive) ? keyActive : (c < codes[b]);
      if (!keyFirst) {
        break;
      }
      codes[b + 1] = codes[b];
      sts[b + 1] = sts[b];
      b--;
    }
    codes[b + 1] = c;
    sts[b + 1] = s;
  }
  datalayer_battery->dtc.dtc_count = count;
  datalayer_battery->dtc.dtc_last_read_millis = millis();
  datalayer_battery->dtc.dtc_read_failed = false;
  datalayer_bydatto->dtc_read_in_progress = false;
}

/* Native BMS termination.

The pack only ends a charge, and only recalibrates its SOC to 100%, inside a real charge session: the
charge grant on 0x347 exists nowhere else. So BE runs the charger side of that session on an already
closed pack - 0x24A/0x47E ask for charge, 0x36A/0x36D grant the DC work mode - and lets the BMS decide
when the charge is over. Nothing here moves a contactor: the pack goes 0x86 -> 0x85 -> 0x84 in place,
and discharge stays available throughout.

On by default. The BMS only enters the charge context when it is not reporting an insulation fault;
the isolation-monitor-disable setting (also on by default) normally keeps that clear, so a pack out of
a car does not need its case isolated from earth for this. */

void BydAttoBattery::start_charge_session(unsigned long currentMillis) {
  // Re-entry from a rest hold keeps the work mode it is already broadcasting; only a first session
  // has to ramp 0x36D up from idle.
  chargeRampDone = (chargeSessionState == CHG_SESSION_HOLD);
  chargeSessionState = CHG_SESSION_REQUEST;
  chargeSessionEntryMillis = currentMillis;
  chargeRampStartMillis = currentMillis;
  chargeSessionTerminated = false;
  chargeDonePending = false;
  chargeGrantZeroCandidate = false;
  chargeGrantPrevious = 0;
  chargeArmCurrentSinceMillis = 0;
  ATTO_3_24A.data.u8[0] = 0x80;  // charge requested
  ATTO_3_47E.data.u8[2] = 0x01;
  DEBUG_PRINTF("[BYD] Charge session requested\n");
}

void BydAttoBattery::enter_charge_delivery(unsigned long currentMillis) {
  ATTO_3_24A.data.u8[0] = 0x88;
  ATTO_3_47E.data.u8[2] = 0x0C;
  chargeSessionState = CHG_SESSION_CHARGING;
  chargeSessionEntryMillis = currentMillis;
  // Forget any grant value left over from an earlier charge, or its drop to zero would read as this
  // charge ending the moment it starts.
  chargeGrantPrevious = 0;
  chargeGrantZeroCandidate = false;
  chargeDonePending = false;
  DEBUG_PRINTF("[BYD] Charge session granted, battery is charging\n");
}

// Fall back to the rest posture the car holds after a charge: session still declared, no current
// offered, pack sitting closed. The frames keep running because stopping 0x36D would open the pack.
void BydAttoBattery::hold_charge_session(unsigned long currentMillis, const char* reason, bool backoff) {
  ATTO_3_24A.data.u8[0] = 0x8C;
  ATTO_3_24A.data.u8[1] = 0x00;
  ATTO_3_24A.data.u8[3] = 0x00;
  ATTO_3_24A.data.u8[6] = 0x00;
  ATTO_3_47E.data.u8[2] = 0x0F;
  chargeSessionState = CHG_SESSION_HOLD;
  chargeSessionEntryMillis = currentMillis;
  chargeDonePending = false;
  chargeGrantZeroCandidate = false;
  chargeRestFlagSeenClear = false;
  chargeArmCurrentSinceMillis = 0;
  if (backoff) {
    chargeBackoffActive = true;
    chargeBackoffStartMillis = currentMillis;
  }
  DEBUG_PRINTF("[BYD] Charge session resting: %s\n", reason);
}

// Full stand-down. Only safe while the pack is open or opening, since the BMS drops the contactors
// about half a second after the 0x36D keep-alive stops.
void BydAttoBattery::end_charge_session(const char* reason) {
  if (chargeSessionState == CHG_SESSION_IDLE) {
    return;
  }
  chargeSessionState = CHG_SESSION_IDLE;
  chargeSessionTerminated = false;
  chargeDonePending = false;
  chargeGrantZeroCandidate = false;
  chargeRearmAllowed = true;
  chargeRampDone = false;
  chargeArmCurrentSinceMillis = 0;
  chargeRearmDischargeSinceMillis = 0;
  ATTO_3_24A.data.u8[0] = 0x00;
  ATTO_3_24A.data.u8[1] = 0x00;
  ATTO_3_24A.data.u8[3] = 0x00;
  ATTO_3_24A.data.u8[6] = 0x00;
  ATTO_3_47E.data.u8[2] = 0x13;
  DEBUG_PRINTF("[BYD] Charge session ended: %s\n", reason);
}

// The BMS called the pack full: record it and stop offering current. Callers check the termination
// floor first.
void BydAttoBattery::confirm_charge_termination() {
  const uint16_t cell_max_mV = datalayer_battery->status.cell_max_voltage_mV;
  const uint16_t cell_min_mV = datalayer_battery->status.cell_min_voltage_mV;
  const uint16_t spread_mV = (cell_max_mV > cell_min_mV) ? (cell_max_mV - cell_min_mV) : 0;
  chargeGrantZeroCandidate = false;
  chargeDonePending = true;
  chargeSessionEntryMillis = millis();
  // Snapshot the cells at the moment the BMS called it full: comparing these across charges is
  // how top-of-charge balancing shows up.
  if (datalayer_bydatto) {
    datalayer_bydatto->termination_cell_max_mV = cell_max_mV;
    datalayer_bydatto->termination_cell_min_mV = cell_min_mV;
    datalayer_bydatto->termination_cell_delta_mV = spread_mV;
    datalayer_bydatto->termination_cell_max_number = BMS_max_cell_voltage_number;
    datalayer_bydatto->termination_cell_min_number = BMS_min_cell_voltage_number;
    chargeTerminatedRails = true;
    if (datalayer_bydatto->balancing_enabled && datalayer_battery == &datalayer.battery && !balancingCycleDone) {
      balancingState = BALANCING_ARMED;
      balancingStateMillis = millis();
    }
  }
  set_event(EVENT_BYD_CHARGE_TERMINATED, (uint8_t)(spread_mV / 10));
  DEBUG_PRINTF("[BYD] Battery ended the charge at %umV, cell spread %umV\n", cell_max_mV, spread_mV);
}

// The BMS withdraws its grant by dropping 0x347 byte 1 to zero about a second before it clears the
// charge flag. The value sawtooths through a session, so only the zero edge counts, and 0x345 byte 4
// has to agree before acting on it - it mirrors the same value a frame earlier.
void BydAttoBattery::handle_charge_grant(uint8_t grant) {
  if (chargeSessionState == CHG_SESSION_CHARGING && (contactor_feedback & BMS_FEEDBACK_CHARGE_FLAG) &&
      !chargeDonePending) {
    if (!chargeGrantZeroCandidate && chargeGrantPrevious != 0x00 && grant == 0x00) {
      chargeGrantZeroCandidate = true;
      chargeGrantCandidateMillis = millis();
    } else if (chargeGrantZeroCandidate && grant != 0x00) {
      chargeGrantZeroCandidate = false;  // came back up: sawtooth, not the end of the charge
    }
    const bool mirror_confirms =
        chargeGrantMirror == 0x00 && (millis() - chargeGrantMirrorMillis) < SESSION_MIRROR_FRESH_MS;
    // The timeout only stands in for a mirror that has gone quiet. A mirror that is still arriving
    // and disagreeing keeps the veto; a genuine stop then resolves through the charge flag instead.
    const bool mirror_quiet = (millis() - chargeGrantMirrorMillis) > SESSION_GRANT_MIRROR_MS;
    if (chargeGrantZeroCandidate && grant == 0x00 &&
        (mirror_confirms || (mirror_quiet && (millis() - chargeGrantCandidateMillis) > SESSION_GRANT_MIRROR_MS))) {
      chargeGrantZeroCandidate = false;
      if (datalayer_battery->status.cell_max_voltage_mV < SESSION_TERMINATION_FLOOR_MV) {
        // Every real termination sits at 3742-3753mV. A grant-zero this far below the band is the
        // BMS aborting the charge, not the pack being full - stand down and try again later.
        hold_charge_session(millis(), "battery stopped granting below the termination band", true);
        return;
      }
      confirm_charge_termination();
    }
  }
  chargeGrantPrevious = grant;
}

void BydAttoBattery::handle_charge_session(unsigned long currentMillis) {
  // Primary battery only: inverter charge limits are computed from battery 1 alone, so a session on
  // a secondary battery could never stop the bank when its BMS terminates.
  const bool enabled =
      datalayer_bydatto && datalayer_bydatto->native_termination_enabled && datalayer_battery == &datalayer.battery;
  const bool pack_closed = (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) != 0;
  const bool charge_flag = (contactor_feedback & BMS_FEEDBACK_CHARGE_FLAG) != 0;
  const int16_t current_dA = datalayer_battery->status.current_dA;

  // The keep-alive may only stop once the pack is open or the contactor machine has actually
  // committed to opening - it force-opens about half a second after 0x36D stops, which must not
  // happen while the inverter is still winding current down.
  const bool pack_opening = (contactorState == CONTACTORS_OPENING || contactorState == CONTACTORS_OPEN_REQUESTED ||
                             contactorState == CONTACTORS_OPEN_SETTLE || contactorState == CONTACTORS_STANDBY ||
                             contactorState == CONTACTORS_BOOT_ESTOP);
  if (chargeSessionState != CHG_SESSION_IDLE && (!pack_closed || pack_opening)) {
    end_charge_session("battery is opening");
    return;
  }
  // Open requested but current not down yet: stop asking for charge, keep the pack held closed.
  const bool session_running = chargeSessionState != CHG_SESSION_IDLE && chargeSessionState != CHG_SESSION_HOLD;
  if (contactorState == CONTACTORS_AWAIT_ZERO_CURRENT && session_running) {
    hold_charge_session(currentMillis, "open requested", false);
  } else if (!enabled && session_running) {
    // Turned off mid-charge. The keep-alive still has to run until the pack opens, so rest instead
    // of standing down completely.
    hold_charge_session(currentMillis, "feature turned off", false);
  }
  if (chargeBackoffActive && currentMillis - chargeBackoffStartMillis >= SESSION_BACKOFF_MS) {
    chargeBackoffActive = false;
  }

  switch (chargeSessionState) {
    case CHG_SESSION_IDLE:
    case CHG_SESSION_HOLD:
      if (chargeSessionState == CHG_SESSION_HOLD) {
        if (ATTO_3_24A.data.u8[0] == 0x8C && currentMillis - chargeSessionEntryMillis >= SESSION_HOLD_SETTLE_MS) {
          ATTO_3_24A.data.u8[0] = 0x80;  // settled, session declared but idle
        }
        if (!charge_flag) {
          chargeRestFlagSeenClear = true;
        }
        // Only a fresh 0x85 after the pack has been seen at rest counts as the battery resuming on
        // its own. At stand-down the old charge flag lingers for a second and must not re-enter.
        if (charge_flag && chargeRestFlagSeenClear && enabled && chargeRearmAllowed &&
            contactorState != CONTACTORS_AWAIT_ZERO_CURRENT) {
          enter_charge_delivery(currentMillis);
          break;
        }
      }
      // A terminated charge stays blocked until the pack has actually been discharged, otherwise the
      // cells relax below the band and the charge simply restarts.
      if (chargeSessionTerminated && !chargeRearmAllowed) {
        if (current_dA <= SESSION_REARM_DISCHARGE_dA) {
          if (chargeRearmDischargeSinceMillis == 0) {
            chargeRearmDischargeSinceMillis = currentMillis;
          } else if (currentMillis - chargeRearmDischargeSinceMillis >= SESSION_REARM_DWELL_MS) {
            chargeRearmAllowed = true;
            chargeSessionTerminated = false;  // stock cell limits back until the next session arms
            chargeRearmDischargeSinceMillis = 0;
            DEBUG_PRINTF("[BYD] Charge released after discharge, ready for the next session\n");
          }
        } else {
          chargeRearmDischargeSinceMillis = 0;
        }
      }
      // A user charge limit below the session tail could never reach the termination band, so don't
      // start a session that cannot finish.
      if (enabled && chargeRearmAllowed && !chargeBackoffActive && pack_closed && contactorState == CONTACTORS_ACTIVE &&
          datalayer_battery->settings.max_user_set_charge_dA >= SESSION_TAIL_CURRENT_dA) {
        if (current_dA >= SESSION_ARM_CURRENT_dA) {
          if (chargeArmCurrentSinceMillis == 0) {
            chargeArmCurrentSinceMillis = currentMillis;
          } else if (currentMillis - chargeArmCurrentSinceMillis >= SESSION_ARM_DWELL_MS) {
            start_charge_session(currentMillis);
          }
        } else {
          chargeArmCurrentSinceMillis = 0;
        }
      }
      break;
    case CHG_SESSION_REQUEST:
      if (ATTO_3_47E.data.u8[2] == 0x01 && currentMillis - chargeSessionEntryMillis >= 200) {
        ATTO_3_47E.data.u8[2] = 0x03;
      }
      if (charge_flag) {
        enter_charge_delivery(currentMillis);
      } else if (currentMillis - chargeSessionEntryMillis >= SESSION_REQUEST_DWELL_MS) {
        ATTO_3_24A.data.u8[0] = 0x84;  // ready
        ATTO_3_47E.data.u8[2] = 0x07;
        chargeSessionState = CHG_SESSION_READY;
        chargeSessionEntryMillis = currentMillis;
      }
      break;
    case CHG_SESSION_READY:
      if (charge_flag) {
        enter_charge_delivery(currentMillis);
      } else if (currentMillis - chargeSessionEntryMillis >= SESSION_GRANT_TIMEOUT_MS) {
        // Same as a real charger plugged into a battery that is already full: it waits, gets no
        // grant, and closes the session down.
        hold_charge_session(currentMillis, "battery did not grant charge", true);
      }
      break;
    case CHG_SESSION_CHARGING:
      if (!charge_flag) {
        // A flag drop while a grant-zero candidate is still waiting on its quiet mirror IS the
        // termination: grant withdrawn, then session ended. Confirm now so this pass enters
        // FINISHING instead of reading the drop as an interruption.
        if (!chargeDonePending && chargeGrantZeroCandidate &&
            datalayer_battery->status.cell_max_voltage_mV >= SESSION_TERMINATION_FLOOR_MV) {
          confirm_charge_termination();
        }
        if (chargeDonePending) {
          ATTO_3_24A.data.u8[3] = 0x00;
          ATTO_3_24A.data.u8[6] = 0x00;
          ATTO_3_47E.data.u8[2] = 0x0E;
          chargeSessionState = CHG_SESSION_FINISHING;
          chargeSessionEntryMillis = currentMillis;
        } else {
          // Interrupted rather than finished (cloud, load, inverter). Nothing to block afterwards.
          hold_charge_session(currentMillis, "charge stopped before the battery ended it", true);
        }
      } else if (chargeDonePending && currentMillis - chargeSessionEntryMillis >= SESSION_DONE_TIMEOUT_MS) {
        // Grant withdrawn but the flag never followed. The grant is the authority, so finish anyway.
        ATTO_3_24A.data.u8[3] = 0x00;
        ATTO_3_24A.data.u8[6] = 0x00;
        ATTO_3_47E.data.u8[2] = 0x0E;
        chargeSessionState = CHG_SESSION_FINISHING;
        chargeSessionEntryMillis = currentMillis;
      }
      break;
    case CHG_SESSION_FINISHING:
      if (currentMillis - chargeSessionEntryMillis >= 1000 && ATTO_3_47E.data.u8[2] == 0x0E) {
        ATTO_3_47E.data.u8[2] = 0x0F;
      }
      if (currentMillis - chargeSessionEntryMillis >= SESSION_FINISH_ACK_MS) {
        hold_charge_session(currentMillis, "charge complete, holding the battery closed", false);
        chargeSessionTerminated = true;
        chargeRearmAllowed = false;
        chargeRearmDischargeSinceMillis = 0;
      }
      break;
    default:
      break;
  }
}

// Cycle the contactors after a termination, holding the pack open for the configured time.
void BydAttoBattery::handle_balancing(unsigned long currentMillis) {
  if (!datalayer_bydatto) {
    return;
  }
  const bool enabled = datalayer_bydatto->balancing_enabled && datalayer_battery == &datalayer.battery;
  const bool pack_closed = (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) != 0;
  // Still closed only because the open has not finished yet, which is not the same as closed.
  const bool open_in_flight = contactorState == CONTACTORS_AWAIT_ZERO_CURRENT || contactorState == CONTACTORS_OPENING ||
                              contactorState == CONTACTORS_OPEN_REQUESTED || contactorState == CONTACTORS_OPEN_SETTLE;
  const uint32_t hold_ms = (uint32_t)datalayer_bydatto->balancing_hold_minutes * 60000UL;

  // One cycle per real discharge, so a day of solar top-ups cannot cycle the contactors repeatedly.
  if (datalayer_battery->status.current_dA <= SESSION_REARM_DISCHARGE_dA) {
    if (balancingDischargeSinceMillis == 0) {
      balancingDischargeSinceMillis = currentMillis;
    } else if (currentMillis - balancingDischargeSinceMillis >= SESSION_REARM_DWELL_MS) {
      balancingCycleDone = false;
    }
  } else {
    balancingDischargeSinceMillis = 0;
  }

  if (datalayer.system.info.equipment_stop_active) {
    balancingState = BALANCING_IDLE;  // the stop owns the contactors
  } else if (!enabled && balancingState != BALANCING_IDLE && balancingState != BALANCING_CLOSING) {
    // Turned off mid-cycle: close the pack back up rather than leaving it open indefinitely. Once
    // this has handed over to CLOSING, the bounded retry below owns it - re-running would reset it.
    const bool give_up = balancingState == BALANCING_ARMED || balancingState == BALANCING_CLOSE_FAILED;
    if (balancingState == BALANCING_OPENING) {
      request_close_contactors();  // cancels the wind-down outright if it has not shut down yet
    }
    balancingState = give_up ? BALANCING_IDLE : BALANCING_CLOSING;
    balancingStateMillis = currentMillis;
    balancingCloseAttempts = 0;
  }

  switch (balancingState) {
    case BALANCING_ARMED:
      // The charge-flag fallback can take 30s, so wait for the session to be genuinely at rest.
      if (currentMillis - balancingStateMillis >= BALANCING_MOVE_TIMEOUT_MS) {
        balancingState = BALANCING_IDLE;  // never got there, leave the pack alone
      } else if (chargeSessionState == CHG_SESSION_HOLD && chargeSessionTerminated &&
                 currentMillis - balancingStateMillis >= BALANCING_SETTLE_MS && !cell_balance_time_active) {
        request_open_contactors_optional();
        balancingCycleDone = true;
        balancingState = BALANCING_OPENING;
        balancingStateMillis = currentMillis;
      }
      break;
    case BALANCING_OPENING:
      if (!pack_closed) {
        balancingState = BALANCING_WAITING;
        balancingStateMillis = currentMillis;
      } else if (contactorState == CONTACTORS_ACTIVE && currentMillis - balancingStateMillis >= 1000) {
        balancingState = BALANCING_IDLE;  // abandoned under load, the pack stayed closed
      } else if (currentMillis - balancingStateMillis >= BALANCING_MOVE_TIMEOUT_MS) {
        // Reverse it rather than walking away with the contactor machine still mid-open.
        request_close_contactors();
        balancingCloseAttempts = 1;
        balancingState = BALANCING_CLOSING;
        balancingStateMillis = currentMillis;
      }
      break;
    case BALANCING_WAITING:
      if (pack_closed) {
        balancingState = BALANCING_IDLE;  // closed by something else, do not fight it
      } else if (currentMillis - balancingStateMillis >= hold_ms) {
        balancingState = BALANCING_CLOSING;
        balancingStateMillis = currentMillis;
        balancingCloseAttempts = 0;
      }
      break;
    case BALANCING_CLOSING:
      // A close is ignored during the ~1.5s shutdown hold, so retry from standby, but only a few
      // times and with a backoff (measured from the request, so ~30s idle after a failed confirm) -
      // a pack that will not come back needs a person, not more attempts.
      if (pack_closed && !open_in_flight) {
        balancingState = BALANCING_IDLE;
      } else if (contactorState == CONTACTORS_STANDBY &&
                 (balancingCloseAttempts == 0 || currentMillis - balancingStateMillis >= BALANCING_CLOSE_BACKOFF_MS)) {
        if (balancingCloseAttempts > BALANCING_CLOSE_RETRIES) {
          set_event(EVENT_BYD_CONTACTOR_MISMATCH, 4);
          balancingState = BALANCING_CLOSE_FAILED;
        } else {
          request_close_contactors();
          balancingCloseAttempts++;
          balancingStateMillis = currentMillis;
        }
      }
      break;
    case BALANCING_CLOSE_FAILED:
      if (pack_closed) {
        balancingState = BALANCING_IDLE;  // closed by hand, carry on
      }
      break;
    default:
      break;
  }

  datalayer_bydatto->balancing_state = balancingState;
  uint16_t remaining = 0;
  if (balancingState == BALANCING_WAITING) {
    const uint32_t elapsed = currentMillis - balancingStateMillis;
    if (elapsed < hold_ms) {
      remaining = (uint16_t)((hold_ms - elapsed + 59999UL) / 60000UL);
    }
  }
  datalayer_bydatto->balancing_remaining_min = remaining;
}

void BydAttoBattery::transmit_charge_session(unsigned long currentMillis) {
  if (chargeSessionState == CHG_SESSION_IDLE) {
    return;
  }
  chargeSessionHeartbeat = !chargeSessionHeartbeat;
  const bool delivering = (chargeSessionState == CHG_SESSION_CHARGING) && !chargeDonePending;
  const bool after_charge =
      chargeDonePending || chargeSessionState == CHG_SESSION_FINISHING || chargeSessionState == CHG_SESSION_HOLD;
  const uint8_t charger_temperature = after_charge ? 0x47 : 0x46;  // 0x47E byte 1 mirrors 0x36D byte 6
  uint16_t link_voltage_V = (uint16_t)((datalayer_battery->status.voltage_dV + 5) / 10);
  if (link_voltage_V > 0) {
    link_voltage_V--;  // charger output sits a volt under the pack, as the captured sessions do
  }

  // 0x24A charge request. Byte 2 is a charger temperature, bytes 3 and 6 session flags that go to
  // zero as the charge ends - the zeroing is what the BMS reads as the charger standing down.
  if (ATTO_3_24A.data.u8[0] == 0x88) {
    ATTO_3_24A.data.u8[2] = 0x51;
    ATTO_3_24A.data.u8[3] = chargeDonePending ? 0x00 : 0x07;
    ATTO_3_24A.data.u8[6] = chargeDonePending ? 0x00 : 0x0E;
  }
  ATTO_3_24A.data.u8[7] = computeBydChecksum(ATTO_3_24A.data.u8);
  transmit_can_frame(&ATTO_3_24A);

  // 0x47E charger state and offer. A real charger delivers full current right up to the grant edge,
  // so the offer only drops once the BMS has withdrawn it.
  ATTO_3_47E.data.u8[1] = charger_temperature;
  ATTO_3_47E.data.u8[3] = delivering ? ATTO_3_OFFER_CURRENT : 0x00;
  ATTO_3_47E.data.u8[4] = delivering ? ATTO_3_OFFER_POWER : 0x01;
  ATTO_3_47E.data.u8[5] = 0xB4;
  ATTO_3_47E.data.u8[6] = 0x01;
  ATTO_3_47E.data.u8[7] = computeBydChecksum(ATTO_3_47E.data.u8);
  transmit_can_frame(&ATTO_3_47E);

  ATTO_3_36A.data.u8[7] = computeBydChecksum(ATTO_3_36A.data.u8);
  transmit_can_frame(&ATTO_3_36A);

  // 0x36D: ramp the DC work mode up on a first session, then hold the keep-alive with live link
  // voltage and delivered current.
  if (!chargeRampDone) {
    const uint32_t elapsed = currentMillis - chargeRampStartMillis;
    uint32_t step = (elapsed < SESSION_RAMP_IDLE_HOLD_MS) ? 0 : (1 + (elapsed - SESSION_RAMP_IDLE_HOLD_MS) / 100);
    if (step >= (uint32_t)(ATTO_3_36D_RAMP_ROWS - 1)) {
      step = ATTO_3_36D_RAMP_ROWS - 1;
      chargeRampDone = true;
    }
    memcpy(ATTO_3_36D.data.u8, ATTO_3_36D_RAMP[step], 7);
  }
  if (chargeRampDone) {
    ATTO_3_36D.data.u8[0] = (uint8_t)(link_voltage_V & 0xFF);
    ATTO_3_36D.data.u8[1] = (uint8_t)(link_voltage_V >> 8);
    ATTO_3_36D.data.u8[2] = 0x32;
    ATTO_3_36D.data.u8[3] = chargeSessionHeartbeat ? 0x89 : 0x8A;
    if (delivering) {
      // Byte 4 is charger output current, 0.5A per bit above a 0x7D zero point
      const int16_t charge_current_dA = datalayer_battery->status.current_dA;
      uint16_t encoded_current = (charge_current_dA > 0) ? (uint16_t)((charge_current_dA + 2) / 5) : 0;
      if (encoded_current > 0x82) {
        encoded_current = 0x82;
      }
      ATTO_3_36D.data.u8[4] = (uint8_t)(0x7D + encoded_current);
    } else {
      ATTO_3_36D.data.u8[4] = (chargeSessionState == CHG_SESSION_HOLD) ? 0x84 : 0x85;
    }
    ATTO_3_36D.data.u8[5] = 0x02;
    ATTO_3_36D.data.u8[6] = charger_temperature;
  }
  ATTO_3_36D.data.u8[7] = computeBydChecksum(ATTO_3_36D.data.u8);
  transmit_can_frame(&ATTO_3_36D);
}

// Software contactor state machine. Steps the transmitted 0x12D frame between the vehicle's
// payload states and confirms each move against 0x344 feedback. See the header for the states.
void BydAttoBattery::handle_contactor_control(unsigned long currentMillis) {
  // Hold open on a fault, the equipment stop, or when the inverter withdraws permission (Solax/SMA
  // gate closing until ready). Edge-trigger on the combined signal so the buttons and inverter
  // can't fight; a held-open reason at boot keeps active-ack until 0x344 reports state.
  bool contactorsAllowedClosed = !datalayer.system.info.equipment_stop_active &&
                                 datalayer.system.status.inverter_allows_contactor_closing &&
                                 datalayer.system.status.system_status != FAULT;
  if (!contactorControlInitialized) {
    previousContactorsAllowedClosed = contactorsAllowedClosed;
    contactorControlInitialized = true;
    if (!contactorsAllowedClosed) {
      set_12D_payload(0x50, 0x18, 0x02, 0x20, 0x04, 0x31);  // Active-ack pattern
      contactorState = CONTACTORS_BOOT_ESTOP;
    }
  }
  if (contactorsAllowedClosed != previousContactorsAllowedClosed) {
    previousContactorsAllowedClosed = contactorsAllowedClosed;
    if (!contactorsAllowedClosed) {
      contactorOpenOptional = false;  // a fault, stop or withdrawn permission always opens
      requestContactorOpen = true;
    } else {
      requestContactorClose = true;
    }
  }

  if (requestContactorOpen) {
    requestContactorOpen = false;
    closeConfirmPending = false;
    if (contactorState == CONTACTORS_CLOSING || contactorState == CONTACTORS_ACTIVE) {
      set_event(EVENT_BYD_CONTACTOR_OPEN_REQ, 0);
      if (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) {
        // Pack is closed - power is already zeroed (update_values) so the inverter winds down while we wait
        contactorState = CONTACTORS_AWAIT_ZERO_CURRENT;
        contactorStateEntryMillis = currentMillis;
      } else {
        // Asked to open mid-precharge - drop to standby now so the BMS stops closing
        set_12D_payload(0x50, 0x14, 0x02, 0x10, 0x04, 0x31);  // Standby pattern
        contactorState = CONTACTORS_STANDBY;
      }
    }
  }

  if (requestContactorClose) {
    requestContactorClose = false;
    if (!contactorsAllowedClosed) {
      // A fault, the equipment stop or the inverter withdrawing permission all hold the pack open
    } else if (contactorState == CONTACTORS_AWAIT_ZERO_CURRENT) {
      // Cancel the pending open (shutdown not sent yet). If the pack already closed, resume the
      // drive-ready hold; if it was still precharging, resume the close so it finishes properly
      set_event(EVENT_BYD_CONTACTOR_CLOSE_REQ, 1);
      if (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) {
        set_12D_payload(0xA0, 0x28, 0x00, 0x22, 0x0C, 0x31);  // Drive-ready pattern
        contactorState = CONTACTORS_ACTIVE;
      } else {
        set_12D_payload(0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71);  // Close/active pattern
        counter_50ms = 0;                                     // Re-run the drive-ready transition
        contactorState = CONTACTORS_CLOSING;
        closeConfirmPending = true;
        closeConfirmStartMillis = currentMillis;
      }
    } else if (contactorState == CONTACTORS_STANDBY || contactorState == CONTACTORS_OPEN_REQUESTED ||
               contactorState == CONTACTORS_OPEN_SETTLE || contactorState == CONTACTORS_BOOT_ESTOP) {
      // Car re-closes straight from the active-ack frame, so allow close from any open state
      set_event(EVENT_BYD_CONTACTOR_CLOSE_REQ, 0);
      set_12D_payload(0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71);  // Close/active pattern
      counter_50ms = 0;                                     // Re-run the drive-ready transition
      contactorState = CONTACTORS_CLOSING;
      closeConfirmPending = true;
      closeConfirmStartMillis = currentMillis;
    }
  }

  switch (contactorState) {
    case CONTACTORS_CLOSING:
      counter_50ms++;
      // Hold off drive-ready until the pack reports closed (0x344 bit7), keeping the car's ~1.15s min
      if (counter_50ms > 23 && (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED)) {
        ATTO_3_12D.data.u8[2] = 0x00;  // Goes from 02->00
        ATTO_3_12D.data.u8[3] = 0x22;  // Goes from A0->22
        ATTO_3_12D.data.u8[5] = 0x31;  // Goes from 71->31
        contactorState = CONTACTORS_ACTIVE;
      }
      break;
    case CONTACTORS_ACTIVE:
      break;
    case CONTACTORS_AWAIT_ZERO_CURRENT:
      // Only act on a current reading taken after the settle wait, so a stale low sample
      // from before the request can't authorise opening under load
      if ((int32_t)(lastCurrentSampleMillis - contactorStateEntryMillis) >= (int32_t)ZERO_CURRENT_MIN_WAIT_MS &&
          battery_current_dA > -OPEN_MAX_CURRENT_dA && battery_current_dA < OPEN_MAX_CURRENT_dA) {
        set_12D_payload(0xA0, 0x28, 0x02, 0x60, 0x04, 0x31);  // Shutdown pattern
        contactorOpenOptional = false;
        contactorState = CONTACTORS_OPENING;
        contactorStateEntryMillis = currentMillis;
      } else if (currentMillis - contactorStateEntryMillis >= ZERO_CURRENT_TIMEOUT_MS && contactorOpenOptional) {
        // Optional open: current never settled, so stay closed rather than breaking contact under load
        set_12D_payload(0xA0, 0x28, 0x00, 0x22, 0x0C, 0x31);  // Drive-ready pattern
        contactorState = CONTACTORS_ACTIVE;
        contactorOpenOptional = false;
      } else if (currentMillis - contactorStateEntryMillis >= ZERO_CURRENT_TIMEOUT_MS) {
        // Timed out - open anyway. Flag whether a fresh reading stayed high (0) or none arrived (1)
        bool had_fresh_sample =
            (int32_t)(lastCurrentSampleMillis - contactorStateEntryMillis) >= (int32_t)ZERO_CURRENT_MIN_WAIT_MS;
        set_event(EVENT_BYD_CONTACTOR_FORCE_OPEN, had_fresh_sample ? 0 : 1);
        set_12D_payload(0xA0, 0x28, 0x02, 0x60, 0x04, 0x31);  // Shutdown pattern
        contactorState = CONTACTORS_OPENING;
        contactorStateEntryMillis = currentMillis;
      }
      break;
    case CONTACTORS_OPENING:
      // Hold shutdown like the car (~1.4s), then the active-ack frame. Pack doesn't open yet
      if (currentMillis - contactorStateEntryMillis >= OPEN_SHUTDOWN_HOLD_MS) {
        set_12D_payload(0x50, 0x18, 0x02, 0x20, 0x04, 0x31);  // Active-ack pattern, ignition off
        contactorState = CONTACTORS_OPEN_REQUESTED;
        contactorStateEntryMillis = currentMillis;
        openTimeoutEventSent = false;
      }
      break;
    case CONTACTORS_OPEN_REQUESTED:
      // Hold until the BMS reports open - check bit7 only since the mode bits vary. Require a
      // frame received since we started holding, so a stale reading can't confirm the open
      if ((int32_t)(lastContactorFeedbackMillis - contactorStateEntryMillis) >= 0 &&
          (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) == 0) {
        clear_event(EVENT_BYD_CONTACTOR_MISMATCH);
        contactorState = CONTACTORS_OPEN_SETTLE;
        contactorStateEntryMillis = currentMillis;
      } else if (!openTimeoutEventSent && currentMillis - contactorStateEntryMillis >= OPEN_CONFIRM_TIMEOUT_MS) {
        set_event(EVENT_BYD_CONTACTOR_MISMATCH, 2);  // Flag the delay but keep holding
        openTimeoutEventSent = true;
      }
      break;
    case CONTACTORS_OPEN_SETTLE:
      // Open confirmed, drop to standby after the car's ~2.5s wait
      if (currentMillis - contactorStateEntryMillis >= OPEN_TO_STANDBY_DELAY_MS) {
        set_12D_payload(0x50, 0x14, 0x02, 0x10, 0x04, 0x31);  // Standby pattern
        contactorState = CONTACTORS_STANDBY;
      }
      break;
    case CONTACTORS_STANDBY:
      break;
    case CONTACTORS_BOOT_ESTOP:
      // Booted with a held-open reason (fault/stop/inverter). Once the pack reports in: already
      // open -> standby, still closed -> run the full open sequence
      if (lastContactorFeedbackMillis != 0) {
        if (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) {
          set_event(EVENT_BYD_CONTACTOR_OPEN_REQ, 1);
          contactorState = CONTACTORS_AWAIT_ZERO_CURRENT;
          contactorStateEntryMillis = currentMillis;
        } else {
          set_12D_payload(0x50, 0x14, 0x02, 0x10, 0x04, 0x31);  // Standby pattern
          contactorState = CONTACTORS_STANDBY;
        }
      }
      break;
    default:
      break;
  }

  if (closeConfirmPending) {
    // Require a frame received since the close was commanded, not a stale closed reading
    if ((int32_t)(lastContactorFeedbackMillis - closeConfirmStartMillis) >= 0 &&
        (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED)) {
      clear_event(EVENT_BYD_CONTACTOR_MISMATCH);
      closeConfirmPending = false;
    } else if (currentMillis - closeConfirmStartMillis >= CLOSE_CONFIRM_TIMEOUT_MS) {
      // Never confirmed closed - fall back to standby (open, no power) instead of sitting active
      set_event(EVENT_BYD_CONTACTOR_MISMATCH, 3);
      set_12D_payload(0x50, 0x14, 0x02, 0x10, 0x04, 0x31);  // Standby pattern
      contactorState = CONTACTORS_STANDBY;
      closeConfirmPending = false;
    }
  }
}

bool BydAttoBattery::request_cell_balance_times() {
  if (datalayer_battery->info.number_of_cells == 0) {
    return false;
  }

  bool idle = false;
  return cell_balance_time_requested.compare_exchange_strong(idle, true);
}

void BydAttoBattery::begin_cell_balance_time_scan(unsigned long currentMillis) {
  memset(cell_balance_time_data.hours, 0, sizeof(cell_balance_time_data.hours));
  memset(cell_balance_time_data.valid, 0, sizeof(cell_balance_time_data.valid));
  cell_balance_time_data.expected_cells =
      min(datalayer_battery->info.number_of_cells, BydCellBalanceTimeData::MAX_CELLS);
  cell_balance_time_data.received_cells = 0;
  cell_balance_time_data.charge_cycles_valid = false;
  cell_balance_time_data.state = BydCellBalanceTimeState::QUEUED;
  cell_balance_time_cell = 0;
  cell_balance_time_retries = 0;
  cell_balance_time_waiting = false;
  cell_balance_time_queued = true;
  cell_balance_time_scan_millis = currentMillis;
}

String BydAttoBattery::cell_balance_times_json() const {
  const BydCellBalanceTimeData& snapshot = cell_balance_time_data;
  const BydCellBalanceTimeState state =
      cell_balance_time_requested.load() && !cell_balance_time_queued && !cell_balance_time_active
          ? BydCellBalanceTimeState::QUEUED
          : snapshot.state;
  String json;
  json.reserve(64 + snapshot.expected_cells * 7);
  json += "{\"s\":" + String(static_cast<uint8_t>(state));
  json += ",\"e\":" + String(snapshot.expected_cells);
  json += ",\"r\":" + String(snapshot.received_cells);
  json += ",\"cy\":";
  json += snapshot.charge_cycles_valid ? String(snapshot.charge_cycles) : "null";
  json += ",\"v\":[";
  for (uint8_t cell = 0; cell < snapshot.expected_cells; cell++) {
    if (cell) {
      json += ",";
    }
    if (snapshot.cell_valid(cell)) {
      json += snapshot.hours[cell];
    } else {
      json += "null";
    }
  }
  json += "]}";
  return json;
}

bool BydAttoBattery::diagnostics_idle() const {
  return stateMachineClearCrash == NOT_RUNNING && stateMachineCalibrateSOC == NOT_RUNNING &&
         stateMachineReadDTC == NOT_RUNNING && stateMachineEraseDTC == NOT_RUNNING &&
         stateMachineIsoRoutine == NOT_RUNNING && !dtc_rx_active && !datalayer_bydatto->dtc_read_in_progress;
}

void BydAttoBattery::finish_cell_balance_time_scan() {
  cell_balance_time_active = false;
  cell_balance_time_waiting = false;
  // 0x0004 counts completed charges; 0x000B counts sessions entered, which is roughly 5x higher.
  cell_balance_time_data.charge_cycles = BMS_times_full_power;
  cell_balance_time_data.charge_cycles_valid = BMS_times_full_power_valid;
  cell_balance_time_data.scan_id++;
  if (cell_balance_time_data.received_cells == cell_balance_time_data.expected_cells) {
    cell_balance_time_data.state = BydCellBalanceTimeState::COMPLETE;
  } else if (cell_balance_time_data.received_cells) {
    cell_balance_time_data.state = BydCellBalanceTimeState::PARTIAL;
  } else {
    cell_balance_time_data.state = BydCellBalanceTimeState::FAILED;
  }
  cell_balance_time_requested.store(false);
}

// Cell timers are little-endian uint16 values from DIDs 0x0040-0x00BD. Returns true when the frame
// belonged to the scan and needs no further parsing.
bool BydAttoBattery::handle_cell_balance_time_reply(const CAN_frame& frame) {
  if (!awaiting_cell_balance_reply()) {
    return false;
  }

  if (frame.data.u8[0] >= 0x05 && frame.data.u8[1] == 0x62) {
    const uint16_t reply_did = (static_cast<uint16_t>(frame.data.u8[2]) << 8) | frame.data.u8[3];
    if (reply_did != CELL_BALANCE_TIME_DID_BASE + cell_balance_time_cell + 1) {
      return false;
    }
    cell_balance_time_data.hours[cell_balance_time_cell] =
        frame.data.u8[4] | (static_cast<uint16_t>(frame.data.u8[5]) << 8);
    cell_balance_time_data.valid[cell_balance_time_cell / 8] |=
        static_cast<uint8_t>(1U << (cell_balance_time_cell % 8));
    cell_balance_time_data.received_cells++;
    advance_cell_balance_time_cell();
    return true;
  }

  // A pending response keeps the current DID active; other negative replies mark it unreadable.
  if (frame.data.u8[0] >= 0x03 && frame.data.u8[1] == 0x7F && frame.data.u8[2] == 0x22) {
    if (frame.data.u8[3] == 0x78) {
      cell_balance_time_request_millis = millis();
      return true;
    }
    advance_cell_balance_time_cell();
    return true;
  }

  return false;
}

// Every exit from a DID - answered, refused or timed out - goes through here, so the cursor, the
// retry count and the end-of-scan check cannot drift apart.
void BydAttoBattery::advance_cell_balance_time_cell() {
  cell_balance_time_cell++;
  cell_balance_time_retries = 0;
  cell_balance_time_waiting = false;
  if (cell_balance_time_cell >= cell_balance_time_data.expected_cells) {
    finish_cell_balance_time_scan();
  }
}

void BydAttoBattery::handle_cell_balance_time_poll(unsigned long currentMillis) {
  if (cell_balance_time_queued) {
    cell_balance_time_queued = false;
    cell_balance_time_active = true;
    cell_balance_time_data.state = BydCellBalanceTimeState::READING;
  }
  if (!cell_balance_time_active) {
    return;
  }
  if (currentMillis - cell_balance_time_scan_millis >= CELL_BALANCE_TIME_SCAN_TIMEOUT_MS) {
    finish_cell_balance_time_scan();
    return;
  }

  if (cell_balance_time_waiting) {
    if (currentMillis - cell_balance_time_request_millis < CELL_BALANCE_TIME_TIMEOUT_MS) {
      return;
    }
    if (cell_balance_time_retries < CELL_BALANCE_TIME_RETRIES) {
      cell_balance_time_retries++;
    } else {
      advance_cell_balance_time_cell();
      if (!cell_balance_time_active) {
        return;
      }
    }
  }

  const uint16_t did = CELL_BALANCE_TIME_DID_BASE + cell_balance_time_cell + 1;
  ATTO_3_7E7_POLL.data.u8[2] = static_cast<uint8_t>(did >> 8);
  ATTO_3_7E7_POLL.data.u8[3] = static_cast<uint8_t>(did);
  transmit_can_frame(&ATTO_3_7E7_POLL);
  cell_balance_time_request_millis = currentMillis;
  cell_balance_time_waiting = true;
}

void BydAttoBattery::transmit_can(unsigned long currentMillis) {
  //Send 50ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    // Set close contactors to allowed (Useful for crashed packs, started via contactor control thru GPIO)
    if (allows_contactor_closing) {
      if (datalayer.system.status.system_status == ACTIVE) {
        *allows_contactor_closing = true;
      } else {  // Fault state, open contactors!
        *allows_contactor_closing = false;
      }
    }

    handle_contactor_control(currentMillis);
    handle_charge_session(currentMillis);
    handle_balancing(currentMillis);

    // Byte 6 = rolling counter (high nibble counts up, low nibble 0xF), byte 7 = checksum
    frame6_counter = (frame6_counter + 1) & 0x0F;
    ATTO_3_12D.data.u8[6] = (0x0F | (frame6_counter << 4));
    ATTO_3_12D.data.u8[7] = computeBydChecksum(ATTO_3_12D.data.u8);

    transmit_can_frame(&ATTO_3_12D);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (counter_100ms < 100) {
      counter_100ms++;
    }

    if (counter_100ms > 3) {
      // Bytes 4-5 = link voltage; matched to the pack it's the precharge-done signal that closes
      // the contactors. Report the low floating link while open, else we hold close while opening.
      const bool pack_open = contactorState == CONTACTORS_OPEN_SETTLE || contactorState == CONTACTORS_STANDBY ||
                             contactorState == CONTACTORS_BOOT_ESTOP;
      // byte 1 is 0x00 only while open: 0x40 on the close, 0x44 precharging, 0x41 running
      if (!prechargeInitialised) {
        prechargeInitialised = true;
        prechargeWaitStartMillis = currentMillis;  // boot-default close: time out from here, not from uptime 0
      }
      if (pack_open) {
        prechargeState = PRECHARGE_WAIT;  // next close ramps from the floating link again
        prechargeEdgeSeen = false;
        prechargeWaitStartMillis = currentMillis;
      } else if (prechargeState == PRECHARGE_WAIT) {
        if (contactor_feedback & BMS_FEEDBACK_MAIN_CLOSED) {
          prechargeState = PRECHARGE_DONE;  // booted into a closed pack, never fake a ramp
        } else if (prechargeEdgeSeen) {
          prechargeState = PRECHARGE_RAMP;
        } else if (currentMillis - prechargeWaitStartMillis >= PRECHARGE_WAIT_MAX_MS) {
          prechargeState = PRECHARGE_DONE;  // BMS never moved, report the link rather than stall
        }
      } else if (prechargeState == PRECHARGE_RAMP && currentMillis - prechargeRampStartMillis >= PRECHARGE_RAMP_MS) {
        prechargeState = PRECHARGE_DONE;
      }
      // Pack voltage before the BMS has precharged reads as a stuck contactor - P1A3400
      bool report_link_voltage_low = !BMS_voltage_available || pack_open || prechargeState == PRECHARGE_WAIT;
      uint16_t link_voltage = battery_voltage ? (uint16_t)(battery_voltage - 1) : 0;
      if (prechargeState == PRECHARGE_RAMP && !report_link_voltage_low && battery_voltage > 12) {
        const uint32_t since = currentMillis - prechargeRampStartMillis;
        uint32_t pct = (since < 100)   ? (62 * since) / 100
                       : (since < 400) ? 62 + (33 * (since - 100)) / 300
                                       : 95 + (5 * (since - 400)) / 500;
        link_voltage = (uint16_t)(12 + ((uint32_t)(link_voltage - 12) * pct) / 100);
      }
      if (report_link_voltage_low) {
        ATTO_3_441.data.u8[4] = 0x0C;
        ATTO_3_441.data.u8[5] = 0x00;
        ATTO_3_441.data.u8[6] = 0xFF;
        ATTO_3_441.data.u8[7] = 0x87;
      } else {
        ATTO_3_441.data.u8[4] = (uint8_t)link_voltage;
        ATTO_3_441.data.u8[5] = (link_voltage >> 8);
        ATTO_3_441.data.u8[6] = 0xFF;
        ATTO_3_441.data.u8[7] = computeBydChecksum(ATTO_3_441.data.u8);
      }
    }

    transmit_can_frame(&ATTO_3_441);

    transmit_charge_session(currentMillis);

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
    switch (stateMachineReadDTC) {
      case STARTED:
        transmit_can_frame(&ATTO_3_7E7_READ_DTC);
        stateMachineReadDTC = NOT_RUNNING;
        break;
      default:
        break;
    }
    switch (stateMachineEraseDTC) {
      case STARTED:
        // DiagnosticSessionControl, default session
        ATTO_3_7E7_CLEAR_CRASH.data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH);
        stateMachineEraseDTC = RUNNING_STEP_1;
        break;
      case RUNNING_STEP_1:
        // ClearDiagnosticInformation, all groups
        ATTO_3_7E7_CLEAR_CRASH.data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH);
        stateMachineEraseDTC = NOT_RUNNING;
        datalayer_bydatto->UserRequestDTCreadout = true;  // Re-read so the table reflects the cleared state
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
                                     (uint8_t)(datalayer_bydatto->calibrationTargetSOC * 100),
                                     (uint8_t)((datalayer_bydatto->calibrationTargetSOC * 100) >> 8),
                                     (uint8_t)(datalayer_bydatto->calibrationTargetAH * 100),
                                     (uint8_t)((datalayer_bydatto->calibrationTargetAH * 100) >> 8)};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineCalibrateSOC = NOT_RUNNING;
        break;
      case NOT_RUNNING:
        break;
      default:
        break;
    }
    switch (stateMachineIsoRoutine) {
      case STARTED:
        // DiagnosticSessionControl enter extendedDiagnosticSession (10 03)
        solvedKey = 0;  // force a fresh seed before the key is sent
        increaseTimeoutIso = 0;
        ATTO_3_7E7_RESET_SOC.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineIsoRoutine = RUNNING_STEP_1;
        break;
      case RUNNING_STEP_1:
        // SecurityAccess requestSeed (27 01)
        ATTO_3_7E7_RESET_SOC.data = {0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        stateMachineIsoRoutine = RUNNING_STEP_2;
        break;
      case RUNNING_STEP_2:
        // Wait for the seed (RX sets solvedKey), then SecurityAccess sendKey (27 02)
        if (solvedKey > 0) {
          ATTO_3_7E7_RESET_SOC.data = {
              0x04, 0x27, 0x02, (uint8_t)((solvedKey & 0xFF00) >> 8), (uint8_t)(solvedKey & 0x00FF), 0x00, 0x00, 0x00};
          transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
          stateMachineIsoRoutine = RUNNING_STEP_3;
        } else if (++increaseTimeoutIso > 30) {
          datalayer_bydatto->iso_command_status = 4;  // no reply
          stateMachineIsoRoutine = NOT_RUNNING;
        }
        break;
      case RUNNING_STEP_3: {
        // RoutineControl (1 disable -> 31 01, 2 enable -> 31 02)
        uint8_t subfn = (isoRoutineAction == 1) ? 0x01 : 0x02;
        ATTO_3_7E7_RESET_SOC.data = {0x04, 0x31, subfn, 0x20, 0x08, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_RESET_SOC);
        increaseTimeoutIso = 0;
        stateMachineIsoRoutine = RUNNING_STEP_4;  // RX ends the machine on 71/7F
        break;
      }
      case RUNNING_STEP_4:
        // Wait for the routine reply (RX ends the machine); time out so the UI can't stick.
        if (++increaseTimeoutIso > 20) {  // ~2 s at 100 ms/tick
          if (datalayer_bydatto->iso_command_status == 1) {
            datalayer_bydatto->iso_command_status = 4;  // no reply
          }
          stateMachineIsoRoutine = NOT_RUNNING;
        }
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

    if (!diagnostics_idle()) {
      return;
    }
    if (cell_balance_time_requested.load() && !cell_balance_time_queued && !cell_balance_time_active) {
      begin_cell_balance_time_scan(currentMillis);
    }
    if (cell_balance_time_queued || cell_balance_time_active) {
      handle_cell_balance_time_poll(currentMillis);
      return;
    }

    switch (poll_state) {
      case POLL_FOR_ORIGINAL_CALIBRATION:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_ORIGINAL_CALIBRATION & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_ORIGINAL_CALIBRATION & 0x00FF);
        poll_state = POLL_FOR_CURRENT_CALIBRATION;
        break;
      case POLL_FOR_CURRENT_CALIBRATION:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_CURRENT_CALIBRATION & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_CURRENT_CALIBRATION & 0x00FF);
        poll_state = POLL_CHARGE_TIMES;
        break;
      case POLL_CHARGE_TIMES:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_CHARGE_TIMES & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_CHARGE_TIMES & 0x00FF);
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
        poll_state = POLL_FOR_ORIGINAL_CALIBRATION;
        break;
      default:
        poll_state = POLL_FOR_ORIGINAL_CALIBRATION;
        break;
    }

    transmit_can_frame(&ATTO_3_7E7_POLL);
  }
}

void BydAttoBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.chemistry = battery_chemistry_enum::LFP;
  datalayer_battery->info.max_design_voltage_dV = 6500;  //Startup in extremes
  datalayer_battery->info.min_design_voltage_dV = 2000;  //We later determine range based on amount of cells
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
