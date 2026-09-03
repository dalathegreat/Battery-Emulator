#ifndef AKASOL_BATTERY_H
#define AKASOL_BATTERY_H

#include <Arduino.h>
#include "../devboard/hal/hal.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"
#include "CanBattery.h"

/* ============================================================================
 * AKASOL AKASYSTEM (15 OEM 50 PRC) battery driver
 * Single-tray (BMM01) configuration.
 *
 * Based on:
 *  - PublicCAN.dbc supplied by the user
 *  - AKASOL User Manual AKASYSTEM 15 OEM 50 PRC, Doc No. 3-024001TEN_0001, v1.2
 *
 * IMPORTANT - THIS IS UNTESTED AGAINST REAL HARDWARE.
 * Verify every value against your own CAN bus traffic before trusting it
 * for anything that touches contactors or high voltage.
 * ========================================================================= */

// ---------------------------------------------------------------------------
// GPIO CONFIGURATION
//
// Besides CAN, the Akasol battery needs three discrete 12V/24V control signals,
// normally driven through a relay/SSR/MOSFET:
//   - KL30_safe  : safety-loop supply. Must be stable BEFORE req_batuse=1,
//                  otherwise the battery stays stuck in Init mode.
//   - KL30       : permanent LV supply to the BMU (24V)
//   - KL15/Wake  : "Battery Wake" - wakes the BMU CPUs and starts CAN
//
// Rather than hardcoding pin numbers for one specific board, the driver reuses
// the three contactor control pins that the HAL already defines for every
// Battery-Emulator board:
//
//   PRECHARGE_PIN()          -> KL30
//   NEGATIVE_CONTACTOR_PIN() -> KL30_safe
//   POSITIVE_CONTACTOR_PIN() -> KL15 / Battery Wake
//
// The mapping is deliberate rather than alphabetical. Losing KL30 drops the
// BMU's supply, and losing KL30_safe is an eStop that latches an Error the
// battery only clears through a full restart sequence - so both want to sit on
// pins that can be held across a firmware-initiated reboot (see the HAL's
// reset_hold_pins()). Dropping Wake for a moment is by far the mildest of the
// three, so it gets whichever pin is left. On the LilyGo T-2CAN this puts KL30
// and KL30_safe on RTC-capable GPIOs and Wake on the one that is not; on other
// boards the split may differ, but the ordering of harm does not.
//
// Note: GPIO contactor control must be left disabled when using this driver -
// the two cannot both allocate the same pins. That is not a limitation in
// practice, since the Akasol closes its own HV contactors over CAN.
//
// A board whose HAL leaves these pins at GPIO_NUM_NC simply will not drive the
// signals; the driver still runs CAN-only, which is useful for bench testing.
// ---------------------------------------------------------------------------

// Also implements BatteryHtmlRenderer directly (instead of a separate helper
// class) so get_status_html() can read the private state below without any
// extra getters/friends - same pattern already used elsewhere in this file
// (CanBattery itself multiply-inherits Battery/Transmitter/CanReceiver).
class AkasolBattery : public CanBattery, public BatteryHtmlRenderer {
 public:
  AkasolBattery() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {}

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can(unsigned long currentMillis);
  virtual const char* interface_name() { return "AKASOL"; }

  // Reveals the "Battery BMS status: STANDBY/OK/FAULT" line on the main page.
  // update_values() already computes real_bms_status correctly; this was
  // simply never turned on, so the line was silently hidden.
  virtual bool supports_real_BMS_status() { return true; }

  // "More Battery Info" page: show the internal state that's already parsed
  // from CAN but wasn't exposed anywhere (precharge/standby/operational/error
  // flags, both contactor bits, and the alive-counter handshake).
  virtual BatteryHtmlRenderer& get_status_renderer() { return *this; }
  String get_status_html();
  bool renders_own_battery_data() { return true; }

  static constexpr const char* Name = "AKASOL";

 private:
  // --- CAN node addresses (J1939-style: the low byte of every extended ID) -
  // The AKASOL BMU's own address on Public CAN is set by hardware straps on
  // its BMU-ID pins (Address0..4 In) - see User Manual Figure 24 / Table 10.
  // With the factory-style "Tray 01" bridge (AD0-AD1 + AD2-AD3) fitted, the
  // BMU claims address 0xF3.
  //
  // >>> REVERTED BACK TO 0xF3 - VERIFIED AGAINST THE ACTUAL DBC <<<
  // The bridge was removed earlier for testing (CAN log evidence showed the
  // BMU then claiming address 0x00 instead), and the driver was pointed at
  // 0x00 to match. That was the wrong direction: checked directly against
  // the supplied PublicCAN.dbc, every single one of the 15 predefined
  // per-tray message sets (BMM01..BMM15) has its own hardcoded address baked
  // into the CAN-ID - BMM01=0xF3, BMM02=0xF4 ... BMM15=0xF2 (e.g.
  // VCU1_to_BMM01 default ID decodes to 0x0CEFF3E3, BMM01_State to
  // 0x0CFF10F3, BMM01_ErrorFlag to 0x18FF15F3 - all address 0xF3). There is
  // no "BMM00"/0x00 entry anywhere in the DBC - address 0x00 is not a real,
  // supported tray identity for this battery, it's just whatever the
  // BMU-ID decoding logic happens to output when all 5 AD-pins are left
  // floating (no bridge at all), which is not the same thing as "the
  // documented single-tray default". 0xF3 (BMM01/Tray01, bridge fitted) is
  // the only address that actually exists in this unit's own CAN matrix, and
  // matches how the battery originally shipped.
  // This also gives a plausible mechanism for the persistent, otherwise
  // unexplained bit63/SCUError + detectDevice=163 that never mapped to any
  // of the 62 named ErrorFlag bits: an address the SCU doesn't recognize as
  // a valid configured tray identity is exactly the kind of thing that could
  // trip a generic internal "configuration" self-check (see ErrorFlag bit 31
  // SCUConfig - "Faulty configuration of battery system") without ever
  // setting a specific named bit.
  // ACTION REQUIRED: refit the Tray01 bridge (AD2-AD3 + AD0-AD1, see Table
  // 10) before this build will talk to the BMU again.
  static const uint8_t AKASOL_BMU_ADDR = 0xF3;  // Tray01, per DBC - bridge must be fitted
  static const uint8_t AKASOL_VCU_ADDR = 0xE3;  // our own (VCU) source address - unchanged

  // --- CAN IDs (decoded from PublicCAN.dbc, 29-bit extended) ---------------
  // TX: emulator (VCU) -> battery (BMM01). PDU1 format: ...PF PS(=dest) SA.
  static const uint32_t AKASOL_VCU1_TO_BMM01 =
      0x0CEF0000UL | (static_cast<uint32_t>(AKASOL_BMU_ADDR) << 8) | AKASOL_VCU_ADDR;

  // RX: battery (BMM01) -> emulator (VCU). Base id | BMU's own address.
  static const uint32_t AKASOL_BMM01_STATE = 0x0CFF1000UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_CELLVOLTAGES = 0x18FF1100UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_CELLTEMPERATURES = 0x18FF1200UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_ELECTRICS = 0x18FF1400UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_LIMITS1 = 0x0CFF1A00UL | AKASOL_BMU_ADDR;   // charge/discharge current limits
  static const uint32_t AKASOL_BMM01_LIMITS2 = 0x0CFF1B00UL | AKASOL_BMU_ADDR;   // voltage/power limits
  static const uint32_t AKASOL_BMM01_CAPACITY = 0x18FCEA00UL | AKASOL_BMU_ADDR;  // SOC (Ah based), Ah capacity
  static const uint32_t AKASOL_BMM01_SOH = 0x18FF1C00UL | AKASOL_BMU_ADDR;

  // Detailed 64-bit named fault messages, found in the full PublicCAN.txt
  // (not present in the shorter excerpt originally used) - these break the
  // single generic stat_Error bit down into 64 individually named causes.
  static const uint32_t AKASOL_BMM01_ERRORFLAG = 0x18FF1500UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_ALARMFLAG = 0x18FF1600UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_WARNINGFLAG = 0x18FF1700UL | AKASOL_BMU_ADDR;
  static const uint32_t AKASOL_BMM01_ERROR_INFO = 0x18FF1800UL | AKASOL_BMU_ADDR;  // numeric error/source codes

  // --- Wake / init state machine (see chapter 6.2.1 "Start Up" of manual) --
  // Sequenced to match the exact order manually verified to work on real
  // hardware: KL30_safe alone first (must be bounce-free before anything
  // else - the driver previously asserted KL30_safe and KL30 at the same
  // instant, which is the likely cause of a persistent stat_Error/HVIL-not-
  // seen condition even though Wake/CAN comms came up fine), THEN KL30,
  // THEN (after ~1s) Wake.
  enum class AkasolState {
    INIT_HW,       // driving GPIOs low, nothing started yet
    KL30_SAFE_ON,  // KL30_safe asserted alone, settling before KL30
    KL30_ON,       // KL30_safe + KL30 asserted, settling before Wake
    REQUEST_USE,   // Wake also asserted; waiting for Standby, then sending req_batuse=1
    RUNNING        // battery reported Operational at least once
  };
  AkasolState akasol_state = AkasolState::INIT_HW;
  unsigned long state_entry_time = 0;

  // --- Parsed raw values from CAN --------------------------------------
  int16_t battery_current_dA = 0;  // 0.1A resolution, already matches Battery-Emulator unit
  int16_t battery_voltage_dV = 0;  // 0.1V resolution
  uint16_t cell_voltage_max_mV = 0;
  uint16_t cell_voltage_min_mV = 0;
  int16_t cell_temperature_max_dC = 0;  // 0.1 degC
  int16_t cell_temperature_min_dC = 0;
  uint16_t soc_pptt = 0;  // 0.01% resolution (basis points)
  uint16_t soh_pptt = 0;
  uint16_t max_avail_capacity_Ah_x10 = 0;
  uint16_t lim_charge_curr_A_x10 = 0;
  uint16_t lim_discharge_curr_A_x10 = 0;
  uint16_t lim_max_volt_V_x10 = 0;
  uint16_t lim_min_volt_V_x10 = 0;

  bool stat_operational = false;
  bool stat_standby = false;
  bool stat_error = false;
  bool stat_precharge = false;
  bool stat_init = false;
  bool stat_disabling = false;
  bool contactor_pos = false;
  bool contactor_neg = false;

  // --- Additional BMM01_State bits, decoded from PublicCAN.dbc -------------
  // (previously received but discarded - now parsed so the real reason
  // behind stat_Error can be narrowed down: interlock loop vs. BMS-raised
  // Warning/Alarm/FireRisk.)
  bool flag_drive = false;             // bit8
  bool flag_charge = false;            // bit9
  bool flag_wake = false;              // bit16
  bool flag_isodisable = false;        // bit17
  bool flag_contactor_precha = false;  // bit21 - precharge contactor
  bool flag_chargecomplete = false;    // bit22
  bool flag_extshutdownreq = false;    // bit23
  bool flag_internalkl30safe = false;  // bit24
  bool flag_hvilstate = false;         // bit30 - HV interlock loop closed
  bool flag_estoploopclosed = false;   // bit31 - e-stop / safety loop closed
  bool flag_warning = false;           // bit32
  bool flag_alarm = false;             // bit33
  bool flag_auxcont = false;           // bit34
  bool flag_firerisk = false;          // bit39

  // --- Detailed 64-bit fault registers (BMM01_ErrorFlag/AlarmFlag/WarningFlag) ---
  // Each bit has a specific named meaning (see AKASOL_FAULT_BITS in the .cpp).
  // These are the ONLY place the real root cause behind stat_Error/
  // flag_warning/flag_alarm can be identified - stat_error above just says
  // "something in here is set".
  uint64_t error_flags_raw = 0;
  uint64_t alarm_flags_raw = 0;
  uint64_t warning_flags_raw = 0;

  // BMM01_Error_info: internal AKASOL numeric codes, not publicly documented,
  // but useful to quote verbatim when contacting AKASOL support.
  int16_t errinfo_value = 0;
  uint16_t errinfo_srccompnr = 0;
  uint8_t errinfo_srcsubcompclass = 0;
  uint8_t errinfo_srccompclass = 0;
  uint16_t errinfo_errornumber = 0;
  uint8_t errinfo_detectdevice = 0;

  uint8_t bmm_alive_counter_mirror = 0;  // BMM01_cnt_MirroredAliveCounter (echo of our alive counter)

  bool CAN_battery_still_alive_frames_received = false;

  // --- Our TX alive counter ---
  uint8_t vcu_alive_counter = 0;

  // --- Discrete control signals, resolved from the HAL in setup() ---
  gpio_num_t pin_kl30 = GPIO_NUM_NC;
  gpio_num_t pin_kl15_wake = GPIO_NUM_NC;
  gpio_num_t pin_kl30_safe = GPIO_NUM_NC;

  // --- Helper: drive the three discrete GPIO signals ---
  void set_kl30(bool state);
  void set_kl15_wake(bool state);
  void set_kl30_safe(bool state);
};

#endif  // AKASOL_BATTERY_H
