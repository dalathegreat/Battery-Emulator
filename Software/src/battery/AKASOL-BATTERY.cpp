#include "AKASOL-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/* ============================================================================
 * AKASOL AKASYSTEM 15 OEM 50 PRC - single tray (BMM01) driver
 *
 * Reference data taken directly from the supplied PublicCAN.dbc and from the
 * AKASOL User Manual (doc 3-024001TEN_0001 v1.2):
 *   - Nominal voltage 655V, range 540-756V, 50Ah, 33kWh, 15 modules x 12 cells
 *     = 180 cells in series (12s1p per module, 15s modules => 180s1p pack)
 *   - Public CAN runs at 250 kbit/s (SAE J1939)
 *   - Startup sequence per chapter 6.2.1 "Start Up"
 * ========================================================================= */

#define AKASOL_NOMINAL_CAPACITY_AH 50
#define AKASOL_NOMINAL_ENERGY_WH 33000
#define AKASOL_NUMBER_OF_CELLS 180  // 15 modules * 12 cells, all in series
// Battery's true physical design voltage limits (AKASOL User Manual,
// doc 3-024001TEN_0001 v1.2: nominal 655V, range 540-756V). Used both for
// SolaX frame 0x1872 (max_design_voltage_dV/min_design_voltage_dV) and by
// Battery-Emulator's own generic safety.cpp over-/under-voltage cutoffs, so
// this must describe the real battery, not any one inverter's expectations.
//
// TESTED AND RULED OUT (attempt 1): temporarily setting these to 7592/5824
// (759.2V/582.4V, matching SolaX type 131 "TR25_P" / module count 13's own
// documented table window exactly, per Dala's suggestion that these should
// be "in tandem" with the SolaX battery type/module setting) made no
// difference to the persistent IE07 BatVoltFault.
//
// TESTED AND RULED OUT (attempt 2, per Dala): narrowed to 700.0V/600.0V,
// centered on the pack's actual operating point (~634-636V), on the theory
// that the full 540-756V true design range might be an unlucky span that
// doesn't sit well against any of SolaX's internal factory-battery voltage
// windows. Also made no difference to IE07.
//
// Reverted to the battery's true limits below - two independent attempts at
// tuning this value have shown no effect on the fault, and narrowing it only
// risks a false over-/under-voltage event from Battery-Emulator's own
// generic safety.cpp layer with no offsetting benefit.
#define AKASOL_MAX_PACK_VOLTAGE_DV 7560  // 756.0V
#define AKASOL_MIN_PACK_VOLTAGE_DV 5400  // 540.0V

// Per-cell design voltage limits, derived directly from the pack limits above
// divided by the real cell count (7560/180 = 42.0, 5400/180 = 30.0 - exact,
// not a guess). FOUND MISSING (2026-08-27): datalayer.battery.info.max/min_
// cell_voltage_mV were never set anywhere in this driver. The stock/pristine
// SOLAX-CAN.cpp uses exactly these two fields as the denominator of its cell
// voltage rescale math for frame 0x1874 (max_cell_voltage_mV - min_cell_
// voltage_mV). Left at 0 (their default), that denominator is 0 - undefined
// behaviour / division by zero in the ORIGINAL driver, before any of our own
// patches. Every other supported battery driver sets these two fields; ours
// never did. This is a very plausible reason AKASOL behaves differently from
// the 50+ batteries the SolaX integration already works for, independent of
// anything in SOLAX-CAN.cpp itself.
#define AKASOL_MAX_CELL_VOLTAGE_MV 4200  // 4.2V, from 7560dV / 180 cells
#define AKASOL_MIN_CELL_VOLTAGE_MV 3000  // 3.0V, from 5400dV / 180 cells

// FOUND MISSING (2026-08-27, second pass): checked all ~49 other battery drivers in this
// codebase for which datalayer.battery.info/status fields they set that we don't. 17/49
// (mostly other NMC/NCA packs - BMW, Ford, Foxess, Geely, BYD, Jaguar etc.) set
// info.max_cell_voltage_deviation_mV, which we never did either. It's used by safety.cpp's
// own EVENT_CELL_DEVIATION_HIGH ("Large cell voltage deviation! Check balancing of cells") -
// WARNING level only, does not affect system_status/FAULT or anything sent to Solax, but left
// at 0 it means ANY nonzero real cell delta permanently trips that warning on the Events page.
// 250mV matches the most common value used by other NMC/NCA drivers (BMW, Ford, Foxess,
// Jaguar, Geely) - comfortably above AKASOL's own observed real-world delta (~47-48mV).
#define AKASOL_MAX_CELL_DEVIATION_MV 250

// Time (ms) to hold KL30_safe alone before asserting KL30.
// Manual does not give an exact value; this is a conservative placeholder.
#define AKASOL_T_KL30_SETTLE_MS 500

// Time (ms) to hold KL30_safe+KL30 before asserting Battery Wake - matches
// the "1s pause" step of the sequence manually verified to work on real
// hardware (KL30_safe -> KL30 -> 1s -> Wake).
#define AKASOL_T_KL30_TO_WAKE_MS 1000

// How often to transmit the VCU1_to_BMM01 message. Manual does not specify an
// exact cyclic rate; 100ms is a common BMS heartbeat rate and safely inside
// any reasonable alive-counter timeout.
#define AKASOL_TX_INTERVAL_MS 100

// The three setters are no-ops on a board whose HAL leaves the pin at
// GPIO_NUM_NC, so the driver still runs CAN-only on such hardware.
void AkasolBattery::set_kl30(bool state) {
  if (pin_kl30 != GPIO_NUM_NC) {
    digitalWrite(pin_kl30, state ? HIGH : LOW);
  }
}

void AkasolBattery::set_kl15_wake(bool state) {
  if (pin_kl15_wake != GPIO_NUM_NC) {
    digitalWrite(pin_kl15_wake, state ? HIGH : LOW);
  }
}

void AkasolBattery::set_kl30_safe(bool state) {
  if (pin_kl30_safe != GPIO_NUM_NC) {
    digitalWrite(pin_kl30_safe, state ? HIGH : LOW);
  }
}

void AkasolBattery::setup() {
  // Reuse the HAL's contactor pins for the battery's three discrete control
  // signals (see the mapping in AKASOL-BATTERY.h). Claim them up front so a
  // clash with GPIO contactor control is reported rather than silently
  // fighting over the same outputs.
  pin_kl30 = esp32hal->PRECHARGE_PIN();
  pin_kl30_safe = esp32hal->NEGATIVE_CONTACTOR_PIN();
  pin_kl15_wake = esp32hal->POSITIVE_CONTACTOR_PIN();

  // ignore_unused, not plain alloc_pins: a board whose HAL leaves any of these
  // at GPIO_NUM_NC should still come up CAN-only rather than refusing to start.
  if (!esp32hal->alloc_pins_ignore_unused(Name, pin_kl30_safe, pin_kl30, pin_kl15_wake)) {
    return;
  }

  // Drive everything low before anything else, so the start-up sequence always
  // begins from a known state.
  if (pin_kl30_safe != GPIO_NUM_NC) {
    pinMode(pin_kl30_safe, OUTPUT);
    digitalWrite(pin_kl30_safe, LOW);
  }
  if (pin_kl30 != GPIO_NUM_NC) {
    pinMode(pin_kl30, OUTPUT);
    digitalWrite(pin_kl30, LOW);
  }
  if (pin_kl15_wake != GPIO_NUM_NC) {
    pinMode(pin_kl15_wake, OUTPUT);
    digitalWrite(pin_kl15_wake, LOW);
  }

  strncpy(datalayer.system.info.battery_protocol, Name, 63);

  datalayer.battery.info.chemistry = battery_chemistry_enum::NMC;
  datalayer.battery.info.number_of_cells = AKASOL_NUMBER_OF_CELLS;
  datalayer.battery.info.total_capacity_Wh = AKASOL_NOMINAL_ENERGY_WH;
  datalayer.battery.info.max_design_voltage_dV = AKASOL_MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = AKASOL_MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = AKASOL_MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = AKASOL_MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = AKASOL_MAX_CELL_DEVIATION_MV;

  // Do not allow contactor closing / current flow until the state machine
  // below has taken the battery through Init -> Standby -> Operational.
  datalayer.battery.status.real_bms_status = BMS_STANDBY;

  akasol_state = AkasolState::INIT_HW;
  state_entry_time = millis();

  set_kl30(false);
  set_kl15_wake(false);
  set_kl30_safe(false);
}

void AkasolBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case AKASOL_BMM01_STATE: {
      uint8_t b0 = rx_frame.data.u8[0];
      stat_init = (b0 & 0x01) != 0;         // bit 0
      stat_standby = (b0 & 0x02) != 0;      // bit 1
      stat_precharge = (b0 & 0x04) != 0;    // bit 2
      stat_operational = (b0 & 0x08) != 0;  // bit 3
      stat_disabling = (b0 & 0x10) != 0;    // bit 4
      stat_error = (b0 & 0x40) != 0;        // bit 6

      uint8_t b1 = rx_frame.data.u8[1];
      flag_drive = (b1 & 0x01) != 0;   // bit 8
      flag_charge = (b1 & 0x02) != 0;  // bit 9

      uint8_t b2 = rx_frame.data.u8[2];
      flag_wake = (b2 & 0x01) != 0;              // bit 16
      flag_isodisable = (b2 & 0x02) != 0;        // bit 17
      contactor_pos = (b2 & 0x08) != 0;          // bit 19 -> byte2 bit3
      contactor_neg = (b2 & 0x10) != 0;          // bit 20 -> byte2 bit4
      flag_contactor_precha = (b2 & 0x20) != 0;  // bit 21
      flag_chargecomplete = (b2 & 0x40) != 0;    // bit 22
      flag_extshutdownreq = (b2 & 0x80) != 0;    // bit 23

      uint8_t b3 = rx_frame.data.u8[3];
      // NOTE: per PublicCAN.dbc, both HVILState and eStopLoopClosed are documented as
      // "0 = closed, 1 = open" - i.e. the RAW bit is 1 when the loop is OPEN, not closed.
      // The variable names here mean "is closed", so we invert the raw bit to match.
      flag_internalkl30safe = (b3 & 0x01) != 0;  // bit 24 - DBC comment doesn't state polarity, left as raw bit
      flag_hvilstate = (b3 & 0x40) == 0;         // bit 30 - HV interlock loop (0 = closed, 1 = open per DBC)
      flag_estoploopclosed = (b3 & 0x80) == 0;   // bit 31 - e-stop/safety loop (0 = closed, 1 = open per DBC)

      uint8_t b4 = rx_frame.data.u8[4];
      flag_warning = (b4 & 0x01) != 0;   // bit 32
      flag_alarm = (b4 & 0x02) != 0;     // bit 33
      flag_auxcont = (b4 & 0x04) != 0;   // bit 34
      flag_firerisk = (b4 & 0x80) != 0;  // bit 39

      bmm_alive_counter_mirror = rx_frame.data.u8[7];  // byte 56-63

      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      CAN_battery_still_alive_frames_received = true;
      break;
    }
    case AKASOL_BMM01_ELECTRICS: {
      battery_current_dA = (int16_t)(rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8));
      battery_voltage_dV = (int16_t)(rx_frame.data.u8[2] | (rx_frame.data.u8[3] << 8));
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    }
    case AKASOL_BMM01_CELLVOLTAGES: {
      cell_voltage_max_mV = rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8);
      cell_voltage_min_mV = rx_frame.data.u8[2] | (rx_frame.data.u8[3] << 8);
      break;
    }
    case AKASOL_BMM01_CELLTEMPERATURES: {
      cell_temperature_max_dC = (int16_t)(rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8));
      cell_temperature_min_dC = (int16_t)(rx_frame.data.u8[2] | (rx_frame.data.u8[3] << 8));
      break;
    }
    case AKASOL_BMM01_LIMITS1: {
      // bytes 4-5: charge current limit (0.1A), bytes 6-7: discharge current limit (0.1A)
      lim_charge_curr_A_x10 = rx_frame.data.u8[4] | (rx_frame.data.u8[5] << 8);
      lim_discharge_curr_A_x10 = rx_frame.data.u8[6] | (rx_frame.data.u8[7] << 8);
      break;
    }
    case AKASOL_BMM01_LIMITS2: {
      lim_max_volt_V_x10 = rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8);
      lim_min_volt_V_x10 = rx_frame.data.u8[2] | (rx_frame.data.u8[3] << 8);
      break;
    }
    case AKASOL_BMM01_CAPACITY: {
      // SOC scale factor is 0.0025% per bit -> multiply by 25 to get 0.01% (pptt)
      uint16_t raw_soc = rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8);
      soc_pptt = (uint16_t)((uint32_t)raw_soc * 25 / 100);
      max_avail_capacity_Ah_x10 = rx_frame.data.u8[2] | (rx_frame.data.u8[3] << 8);
      break;
    }
    case AKASOL_BMM01_SOH: {
      // 0.1% resolution -> multiply by 10 to get pptt (0.01%)... value is already *10 vs %,
      // dbc scale 0.1 means raw*0.1 = percent, so pptt = raw*10
      uint16_t raw_soh = rx_frame.data.u8[0] | (rx_frame.data.u8[1] << 8);
      soh_pptt = raw_soh * 10;
      break;
    }
    case AKASOL_BMM01_ERRORFLAG: {
      error_flags_raw = 0;
      for (int i = 0; i < 8; i++) {
        error_flags_raw |= ((uint64_t)rx_frame.data.u8[i]) << (8 * i);
      }
      break;
    }
    case AKASOL_BMM01_ALARMFLAG: {
      alarm_flags_raw = 0;
      for (int i = 0; i < 8; i++) {
        alarm_flags_raw |= ((uint64_t)rx_frame.data.u8[i]) << (8 * i);
      }
      break;
    }
    case AKASOL_BMM01_WARNINGFLAG: {
      warning_flags_raw = 0;
      for (int i = 0; i < 8; i++) {
        warning_flags_raw |= ((uint64_t)rx_frame.data.u8[i]) << (8 * i);
      }
      break;
    }
    case AKASOL_BMM01_ERROR_INFO: {
      const uint8_t* d = rx_frame.data.u8;
      errinfo_value = (int16_t)(d[0] | (d[1] << 8));                 // bits 0-15, signed
      uint32_t bits16_31 = d[2] | (d[3] << 8);                       // bits 16-31 packed
      errinfo_srccompnr = bits16_31 & 0x7FF;                         // bits 16-26 (11 bits)
      errinfo_srcsubcompclass = (uint8_t)((bits16_31 >> 11) & 0x3);  // bits 27-28 (2 bits)
      errinfo_srccompclass = (uint8_t)((bits16_31 >> 13) & 0x7);     // bits 29-31 (3 bits)
      errinfo_errornumber = d[4] | (d[5] << 8);                      // bits 32-47
      errinfo_detectdevice = d[6];                                   // bits 48-55
      break;
    }
    default:
      break;
  }
}

void AkasolBattery::update_values() {
  // Guard against publishing zero/uninitialized readings to the datalayer
  // before any real CAN frame has ever been received. battery_voltage_dV,
  // cell_voltage_min_mV/max_mV etc. all default to 0 (see header) and this
  // function is called on its own timer, independent of whether the BMU has
  // said anything yet - so without this guard, there is a real window at
  // boot where update_values() runs first and briefly publishes voltage=0V
  // / cell_min_voltage=0mV to the datalayer. The framework's own generic
  // safety layer polls the datalayer on its own schedule and reacted to
  // that transient zero by permanently latching
  // CELL_CRITICAL_UNDER_VOLTAGE/CELL_UNDER_VOLTAGE/BATTERY_UNDERVOLTAGE and
  // system_status=FAULT (seen on real hardware once contactors closed
  // cleanly) even though real, healthy cell voltages (~3.5V) arrived a
  // fraction of a second later and the AKASOL driver itself reported zero
  // faults. Skip publishing entirely until we've heard from the BMU at
  // least once.
  if (!CAN_battery_still_alive_frames_received) {
    return;
  }

  datalayer.battery.status.voltage_dV = battery_voltage_dV;
  datalayer.battery.status.current_dA = battery_current_dA;
  datalayer.battery.status.real_soc = soc_pptt;
  datalayer.battery.status.soh_pptt = soh_pptt > 0 ? soh_pptt : 10000;

  datalayer.battery.status.cell_max_voltage_mV = cell_voltage_max_mV;
  datalayer.battery.status.cell_min_voltage_mV = cell_voltage_min_mV;

  datalayer.battery.status.temperature_max_dC = cell_temperature_max_dC;
  datalayer.battery.status.temperature_min_dC = cell_temperature_min_dC;

  // Battery reports its own dynamic charge/discharge current limits (BMM01_limits1).
  // Use them directly rather than recomputing from SOC/temperature tables.
  // lim_charge_curr_A_x10 / lim_discharge_curr_A_x10 are already in "amps x10"
  // i.e. exactly the same 0.1A resolution as max_charge_current_dA /
  // max_discharge_current_dA expect (deci-amps) - no rescaling needed.
  uint32_t charge_current_dA = lim_charge_curr_A_x10;
  uint32_t discharge_current_dA = lim_discharge_curr_A_x10;

// SECOND BUG, found from Solax's own compatibility page
// (kb.solaxpower.com/solution/detail/2c9fa4148ceddee5018e94039f8026a0):
// the X1/X3 Hybrid G4 battery PORT has a hardware ceiling of "Max. charge /
// Discharge current [A]: 30" - identical for both models, regardless of how
// many kWh of battery is behind it. AKASOL's own BMM01_limits1 message
// reports what the 50Ah AKASOL pack itself can do, which is on a totally
// different scale (seen on the bus: ~137A charge / ~180A discharge - normal
// for a pack this size, has nothing to do with what Solax's port hardware
// can swallow). Forwarding that raw AKASOL number unclamped (which is
// exactly what the *first* current_dA fix did) tells the inverter "charge/
// discharge me at 137-180A", 4-6x past its own declared port limit - a
// second, opposite-extreme way to feed it an invalid current value
// (first bug: always 0A/too low; this one: uncapped/way too high). Clamp to
// the inverter's real hardware limit before it ever reaches the datalayer.
#define AKASOL_SOLAX_PORT_MAX_CURRENT_DA 300  // 30.0A, per SolaX's own spec
  if (charge_current_dA > AKASOL_SOLAX_PORT_MAX_CURRENT_DA) {
    charge_current_dA = AKASOL_SOLAX_PORT_MAX_CURRENT_DA;
  }
  if (discharge_current_dA > AKASOL_SOLAX_PORT_MAX_CURRENT_DA) {
    discharge_current_dA = AKASOL_SOLAX_PORT_MAX_CURRENT_DA;
  }

  // BUG FIX: these two datalayer fields were never being set anywhere in this
  // driver (only the *_power_W siblings were). They default to 0 and stay 0
  // forever. SOLAX-CAN.cpp's BMS_Limits frame (0x1872, bytes 4-7) sends
  // max_charge_current_dA/max_discharge_current_dA verbatim to the inverter,
  // and it does so starting in the very first BATTERY_ANNOUNCE handshake -
  // before contactors ever close. That means the SolaX was being told, from
  // the first frame onward, "this battery allows 0A charge and 0A discharge"
  // even while voltage/SOC/temperature all read out correctly - a battery
  // that reports a real pack voltage but zero allowed current is exactly the
  // kind of internally-inconsistent limits data that a battery-voltage/limits
  // sanity check performed during the inverter's init/checking phase (IE07
  // BatVoltFault) would plausibly reject, and it would explain why E07
  // persisted even after the earlier datalayer-guard fix (that fix only
  // stopped a transient 0V publish at boot - it never touched these two
  // fields, which were unconditionally 0 the entire time, guard or no guard).
  datalayer.battery.status.max_charge_current_dA = (uint16_t)charge_current_dA;
  datalayer.battery.status.max_discharge_current_dA = (uint16_t)discharge_current_dA;

  datalayer.battery.status.max_charge_power_W = (uint32_t)((uint64_t)charge_current_dA * battery_voltage_dV / 100);
  datalayer.battery.status.max_discharge_power_W =
      (uint32_t)((uint64_t)discharge_current_dA * battery_voltage_dV / 100);

  datalayer.battery.status.remaining_capacity_Wh =
      (uint32_t)((uint64_t)datalayer.battery.info.total_capacity_Wh * soc_pptt / 10000);

  // Do not allow HV use to be requested (or reported "ready") until we have
  // actually completed the wake-up handshake and the battery reports itself
  // Operational. Until then, force limits to zero as a safety default.
  if (akasol_state != AkasolState::RUNNING || stat_error || !stat_operational) {
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_current_dA = 0;
    datalayer.battery.status.max_discharge_current_dA = 0;
  }

  if (stat_error) {
    datalayer.battery.status.real_bms_status = BMS_FAULT;
    set_event(EVENT_BATTERY_CAUTION, 0);
  } else if (stat_operational) {
    datalayer.battery.status.real_bms_status = BMS_ACTIVE;
  } else {
    datalayer.battery.status.real_bms_status = BMS_STANDBY;
  }
}

void AkasolBattery::transmit_can(unsigned long currentMillis) {
  static unsigned long last_tx = 0;

  // --- GPIO / wake-up state machine (manual chapter 6.2.1 Start Up) --------
  // Sequenced KL30_safe -> KL30 -> (settle) -> Wake, matching the exact order
  // manually verified to work on real hardware. Previously KL30 and
  // KL30_safe were asserted in the same instant, which likely prevented the
  // BMU from ever seeing KL30_safe as bounce-free/stable before other
  // signals came up (manual: "must be bounce-free BEFORE req_batuse=1,
  // otherwise the battery gets stuck") - a plausible cause of a persistent
  // stat_Error / HVIL-not-seen condition even though Wake and CAN comms
  // came up fine.
  switch (akasol_state) {
    case AkasolState::INIT_HW:
      set_kl30(false);
      set_kl15_wake(false);
      set_kl30_safe(true);  // KL30_safe asserted alone first.
      state_entry_time = currentMillis;
      akasol_state = AkasolState::KL30_SAFE_ON;
      break;

    case AkasolState::KL30_SAFE_ON:
      if (currentMillis - state_entry_time >= AKASOL_T_KL30_SETTLE_MS) {
        set_kl30(true);  // KL30_safe stays asserted; KL30 comes up next.
        state_entry_time = currentMillis;
        akasol_state = AkasolState::KL30_ON;
      }
      break;

    case AkasolState::KL30_ON:
      if (currentMillis - state_entry_time >= AKASOL_T_KL30_TO_WAKE_MS) {
        set_kl15_wake(true);  // "Battery Wake" -> starts BMU CPUs and CAN
        state_entry_time = currentMillis;
        akasol_state = AkasolState::REQUEST_USE;
      }
      break;

    case AkasolState::REQUEST_USE:
      // Wait until we see the battery in Standby (CAN alive) before asking for
      // HV use. If we never hear from it, we simply keep sending req_batuse=0.
      if (CAN_battery_still_alive_frames_received && stat_standby) {
        akasol_state = AkasolState::RUNNING;  // proceed to request HV use below
      }
      break;

    case AkasolState::RUNNING:
      // Nothing to do here; message content below handles req_batuse=1.
      break;
  }

  // --- Cyclic VCU1_to_BMM01 message ----------------------------------------
  if (currentMillis - last_tx < AKASOL_TX_INTERVAL_MS) {
    return;
  }
  last_tx = currentMillis;

  bool request_use = (akasol_state == AkasolState::RUNNING) && !stat_error;

  CAN_frame AKASOL_VCU_frame = {
      .FD = false, .ext_ID = true, .DLC = 8, .ID = AKASOL_VCU1_TO_BMM01, .data = {0, 0, 0, 0, 0, 0, 0, 0}};

  uint8_t byte0 = 0;
  if (request_use) {
    byte0 |= 0x01;  // bit0 use_req
  }
  // bit4 isolated_bmu - REVERTED back to 0 after testing.
  // Tested isolated_bmu=1 on real hardware (canlog_0d00h01m30s.txt): bit63/
  // SCUError and Error_info (detectDevice=163) were IDENTICAL to the
  // isolated_bmu=0 case - no change at all. So this bit is NOT the cause of
  // the persistent SCUError/detectDevice=163 signature; that theory is ruled
  // out empirically.
  // More importantly, manual chapter 6.2.5 revealed isolated_bmu=1 is not
  // just an informational flag - it is a HARDWARE variant declaration, and
  // setting it commits you to a mandatory procedure: an insulated-BMU system
  // must, during Init mode, temporarily bond LV ground to chassis ground via
  // a relay AND send req_iso_meas=1 within 2 seconds, then release both
  // before Standby. We are NOT doing that dance. The manual's own words:
  // "Noncompliance or work arounds pose the risk of severe injuries or
  // death in case of low insulation between HV and LV." Since it bought us
  // nothing and the described physical setup (KL31_GND only wired to the
  // control supply's own negative, nothing to the battery's chassis output
  // reported) looks like a standard, non-insulated BMU, 0 is the safer
  // default until/unless a real isolated-BMU hardware variant is confirmed
  // and the full init-mode grounding procedure is actually implemented.
#define AKASOL_ISOLATED_BMU 0  // reverted - see comment above
  byte0 |= (AKASOL_ISOLATED_BMU << 4);
  // bit5 req_iso_meas, bit7 iso_disable: leave at 0 unless you implement the
  // insulation-monitoring handshake described in manual chapter 6.2.5.

  AKASOL_VCU_frame.data.u8[0] = byte0;
  AKASOL_VCU_frame.data.u8[2] = vcu_alive_counter;  // bits 16-23
  AKASOL_VCU_frame.data.u8[6] = 0xAA;               // val_code low byte  -> 0x55AA
  AKASOL_VCU_frame.data.u8[7] = 0x55;               // val_code high byte

  transmit_can_frame(&AKASOL_VCU_frame);

  vcu_alive_counter++;
}

// Signal names for all 64 bits shared by BMM01_ErrorFlag, BMM01_AlarmFlag and
// BMM01_WarningFlag - the same bit layout in all three messages, only the
// ef_/af_/wf_ prefix differs in the DBC. Names are AKASOL's own, taken from the
// PublicCAN.sym file they supply.
//
// AKASOL's verbatim plain-English descriptions used to sit alongside each name and
// were rendered in a "Meaning" column on the status page. They cost about 3 kB of
// flash for something only ever read by a human looking at one web page, so they
// are gone; the signal names are descriptive enough to search the AKASOL docs for.
// Flash only - none of this was ever in RAM.
static const char* const AKASOL_FAULT_BITS[64] = {
    /*0*/ "CellVoltageMax",
    /*1*/ "CellVoltageMin",
    /*2*/ "SysVoltageMax",
    /*3*/ "SysVoltageMin",
    /*4*/ "SysVoltageSum",
    /*5*/ "ModVoltageSum",
    /*6*/ "ModVoltageRefMax",
    /*7*/ "ModVoltageRefMin",
    /*8*/ "IsoFaultBattNeg",
    /*9*/ "IsoFaultBattPos",
    /*10*/ "ContactorNegStuck",
    /*11*/ "ContactorPosStuck",
    /*12*/ "LVSupplyMax",
    /*13*/ "LVSupplyMin",
    /*14*/ "TerminalTempMax",
    /*15*/ "TerminalTempMin",
    /*16*/ "CellTempChargMax",
    /*17*/ "CellTempChargMin",
    /*18*/ "CellTempDischMax",
    /*19*/ "CellTempDischMin",
    /*20*/ "SysCurCha",
    /*21*/ "SysCurDis",
    /*22*/ "HVCurrLoopMax",
    /*23*/ "HVCurrLoopMin",
    /*24*/ "eStopCurrLoopMax",
    /*25*/ "eStopCurrLoopMin",
    /*26*/ "SysCurLimitCha",
    /*27*/ "SysCurLimitDis",
    /*28*/ "SysPrechargeFailed",
    /*29*/ "BMMBattCom",
    /*30*/ "SysInitTimeout",
    /*31*/ "SCUConfig",
    /*32*/ "reserved32",
    /*33*/ "WDReset",
    /*34*/ "ExtCommunicationTout",
    /*35*/ "ContactorCoilCurrMax",
    /*36*/ "CellVoltUnbalance",
    /*37*/ "CellTempUnbalance",
    /*38*/ "TerminalTempUnbalance",
    /*39*/ "FanError",
    /*40*/ "KL15Max",
    /*41*/ "KL30SafeCurrentMax",
    /*42*/ "StuckAtTemp",
    /*43*/ "StuckAtVolt",
    /*44*/ "TaskGuardianError",
    /*45*/ "MemoryFault",
    /*46*/ "reserved46",
    /*47*/ "reserved47",
    /*48*/ "InvalidData",
    /*49*/ "ContactorWrongState",
    /*50*/ "CurrSens",
    /*51*/ "ContDam",
    /*52*/ "RefVoltErrorMax",
    /*53*/ "RefVoltErrorMin",
    /*54*/ "Kl30SafeMax",
    /*55*/ "Kl30SafeMin",
    /*56*/ "SCUSupply",
    /*57*/ "SCUPowerProtection",
    /*58*/ "Kl30SafeOff",
    /*59*/ "Dew_Sensor",
    /*60*/ "ContactorDropOut",
    /*61*/ "ECUBoardTemp",
    /*62*/ "Valve",
    /*63*/ "SCUError",
};

String AkasolBattery::get_status_html() {
  // Surfaces internal state that's already parsed from BMM01_State but was
  // previously invisible in the web UI - useful while confirming the wake
  // sequence / contactor closing behaves as expected on real hardware.
  String content;
  content.reserve(9000);

  content +=
      "<h4 style='margin-top:20px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>"
      "AKASOL internal state</h4>";

  auto flag_row = [&content](const char* label, bool value) {
    content += "<p style='margin:4px 0;'>";
    content += label;
    content += ": ";
    content += value ? "<span style='color:#69f0ae;'>yes</span>" : "<span style='color:#bbb;'>no</span>";
    content += "</p>";
  };

  flag_row("Init", stat_init);
  flag_row("Standby", stat_standby);
  flag_row("Precharge", stat_precharge);
  flag_row("Operational (contactors closed)", stat_operational);
  flag_row("Disabling", stat_disabling);
  flag_row("Error flag", stat_error);

  content +=
      "<h4 style='margin-top:16px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>"
      "Contactors &amp; safety loop</h4>";
  flag_row("Contactor + closed", contactor_pos);
  flag_row("Contactor - closed", contactor_neg);
  flag_row("Precharge contactor closed", flag_contactor_precha);
  flag_row("HV interlock loop (HVIL) closed", flag_hvilstate);
  flag_row("E-stop / safety loop closed", flag_estoploopclosed);
  // Raw BMM01 bit 24. AKASOL's DBC gives the signal a name but never states its polarity,
  // so a "no" here does not mean the BMU is missing KL30_safe - it means the bit reads 0,
  // and we do not know which way round that is. The authoritative indicator is fault bit 58
  // "Kl30SafeOff" in the table below: if that is clear, KL30_safe is fine.
  flag_row("BMM01 bit24 (internal KL30_safe, polarity undocumented)", flag_internalkl30safe);
  flag_row("External shutdown requested", flag_extshutdownreq);

  content +=
      "<h4 style='margin-top:16px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>"
      "Warning / Alarm (separate from generic Error above)</h4>";
  flag_row("Warning", flag_warning);
  flag_row("Alarm", flag_alarm);
  flag_row("Fire risk", flag_firerisk);

  content +=
      "<h4 style='margin-top:16px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>"
      "Other flags</h4>";
  flag_row("Drive enabled", flag_drive);
  flag_row("Charge enabled", flag_charge);
  flag_row("Wake", flag_wake);
  flag_row("Isolation monitoring disabled", flag_isodisable);
  flag_row("Charge complete", flag_chargecomplete);
  flag_row("Aux contactor", flag_auxcont);

  content +=
      "<p style='margin:12px 0 4px;color:#ccc;'>BMM01 mirrored alive counter: " + String(bmm_alive_counter_mirror) +
      "</p>";
  content += "<p style='margin:4px 0;color:#ccc;'>Our VCU alive counter (last sent): " +
             String((int)(vcu_alive_counter - 1) & 0xFF) + "</p>";

  // --- Full 64-bit named fault breakdown (BMM01_ErrorFlag/AlarmFlag/WarningFlag) ---
  // This is the ONLY place that identifies which specific condition is
  // behind the generic "Error flag"/"Warning"/"Alarm" shown above. Every one
  // of the 64 defined bits is listed, with active ones highlighted, so
  // nothing gets missed even without AKASOL support available to ask.
  content +=
      "<h4 style='margin-top:20px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>"
      "Detailed fault bits</h4>";
  content +=
      "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>"
      "<tr style='color:#9be7c4;text-align:left;'>"
      "<th style='padding:3px 6px;'>#</th>"
      "<th style='padding:3px 6px;'>Signal</th>"
      "<th style='padding:3px 6px;text-align:center;'>Err</th>"
      "<th style='padding:3px 6px;text-align:center;'>Alm</th>"
      "<th style='padding:3px 6px;text-align:center;'>Wrn</th>"
      "</tr>";
  for (int i = 0; i < 64; i++) {
    bool e = ((error_flags_raw >> i) & 1ULL) != 0;
    bool a = ((alarm_flags_raw >> i) & 1ULL) != 0;
    bool w = ((warning_flags_raw >> i) & 1ULL) != 0;
    bool active = e || a || w;
    content += "<tr style='";
    if (active) {
      content += "background:#3a2323;";
    }
    content += "border-bottom:1px solid #333;'>";
    content += "<td style='padding:3px 6px;color:#888;'>" + String(i) + "</td>";
    content += "<td style='padding:3px 6px;'><b>" + String(AKASOL_FAULT_BITS[i]) + "</b></td>";
    content += "<td style='padding:3px 6px;text-align:center;'>" +
               String(e ? "<span style='color:#ff6b6b;font-weight:bold;'>ERROR</span>" : "-") + "</td>";
    content += "<td style='padding:3px 6px;text-align:center;'>" +
               String(a ? "<span style='color:#ffd166;font-weight:bold;'>ALARM</span>" : "-") + "</td>";
    content += "<td style='padding:3px 6px;text-align:center;'>" +
               String(w ? "<span style='color:#9be7c4;font-weight:bold;'>WARN</span>" : "-") + "</td>";
    content += "</tr>";
  }
  content += "</table>";

  // Raw BMM01_Error_info fields. All zero means no error is being reported.
  content += "<p style='margin:10px 0 2px;color:#ccc;font-size:0.9em;'>Error info: value=" + String(errinfo_value) +
             ", errornumber=" + String(errinfo_errornumber) + ", srcCompClass=" + String(errinfo_srccompclass) +
             ", srcSubcompClass=" + String(errinfo_srcsubcompclass) + ", srcCompNr=" + String(errinfo_srccompnr) +
             ", detectDevice=" + String(errinfo_detectdevice) + "</p>";

  return content;
}
