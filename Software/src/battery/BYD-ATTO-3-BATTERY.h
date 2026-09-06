#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"

#include "BYD-ATTO-3-HTML.h"
#include "CanBattery.h"

#include <atomic>

enum class BydCellBalanceTimeState : uint8_t {
  NOT_READ = 0,
  QUEUED,
  READING,
  COMPLETE,
  PARTIAL,
  FAILED,
};

// Lifetime balancer on-time in whole hours.
struct BydCellBalanceTimeData {
  // Matches the datalayer so larger BYD packs are read in full rather than silently truncated.
  static_assert(MAX_AMOUNT_CELLS <= 255, "cell cursors are uint8_t");
  static constexpr uint8_t MAX_CELLS = MAX_AMOUNT_CELLS;
  static constexpr uint8_t VALID_BYTES = (MAX_CELLS + 7) / 8;

  BydCellBalanceTimeState state = BydCellBalanceTimeState::NOT_READ;
  uint8_t expected_cells = 0;
  uint8_t received_cells = 0;
  uint16_t charge_cycles = 0;
  bool charge_cycles_valid = false;
  uint32_t scan_id = 0;
  uint16_t hours[MAX_CELLS] = {0};
  uint8_t valid[VALID_BYTES] = {0};

  bool cell_valid(uint8_t cell) const {
    return cell < MAX_CELLS && (valid[cell / 8] & static_cast<uint8_t>(1U << (cell % 8)));
  }
};

class BydAttoBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  BydAttoBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_BYDATTO3* extended, CAN_Interface targetCan)
      : CanBattery(targetCan), renderer(extended, "2") {
    datalayer_battery = datalayer_ptr;
    datalayer_bydatto = extended;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  BydAttoBattery() : renderer(&datalayer_extended.bydAtto3) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_bydatto = &datalayer_extended.bydAtto3;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "BYD Atto 3/Seal/Dolphin";

  bool supports_charged_energy() { return true; }
  bool supports_reset_crash() { return true; }
  void reset_crash() { datalayer_bydatto->UserRequestCrashReset = true; }
  bool supports_calibrate_SOC() { return true; }
  void reset_SOC() { datalayer_bydatto->UserRequestCalibrateSOC = true; }
  bool supports_contactor_close() { return true; }
  void request_open_contactors() {
    contactorOpenOptional = false;
    requestContactorOpen = true;
  }
  // An open that is worth abandoning rather than breaking contact under load for.
  void request_open_contactors_optional() {
    contactorOpenOptional = true;
    requestContactorOpen = true;
  }
  void request_close_contactors() { requestContactorClose = true; }
  bool supports_read_DTC() { return true; }
  void read_DTC() { datalayer_bydatto->UserRequestDTCreadout = true; }
  bool supports_reset_DTC() { return true; }
  bool supports_insulation_resistance() { return true; }
  void reset_DTC() { datalayer_bydatto->UserRequestDTCreset = true; }
  bool request_cell_balance_times();
  const BydCellBalanceTimeData& cell_balance_times() const { return cell_balance_time_data; }
  String cell_balance_times_json() const;

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  BydAtto3HtmlRenderer renderer;
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_BYDATTO3* datalayer_bydatto;
  bool* allows_contactor_closing;

  // Ramp down settings that are used when SOC is estimated from voltage
  static const int RAMPDOWN_SOC = 100;  // SOC to start ramping down from. Value set here is scaled by 10 (100 = 10.0%)
  static const int RAMPDOWN_POWER_ALLOWED =
      10000;  // Power to start ramp down from, set a lower value to limit the power even further as SOC decreases

  unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send
  uint64_t last_auto_calibrate_ms = 0;  // Cooldown timer for auto-calibration
  uint32_t autocal_dwell_ms = 0;        // Valid low-current/full time
  uint32_t autocal_grace_start_ms = 0;  // When current left the valid window
  uint16_t cap_slewed_dA = 0;           // Taper slew state, per instance so two packs taper independently
  uint32_t taper_last_ms = 0;
  bool taper_initialized = false;

  static const int POLL_TIMES_FULL_POWER = 0x0004;  // Using Carscanner name for now.
  // 0x0005/0x0008/0x0009 (SOC, voltage, current) come from 0x444 and 0x438, not polled.
  // 0x000A/0x000E (allowed charge/discharge power) come from 0x345 at ~100ms, not polled.
  static const int POLL_CHARGE_TIMES = 0x000B;  // Using Carscanner name for now.
  static const int POLL_TOTAL_CHARGED_AH = 0x000F;
  static const int POLL_TOTAL_DISCHARGED_AH = 0x0010;
  static const int POLL_TOTAL_CHARGED_KWH = 0x0011;
  static const int POLL_TOTAL_DISCHARGED_KWH = 0x0012;
  // 0x002A-0x002D (cell min/max number + voltage) are sourced from the 0x446 broadcast, not polled.
  // 0x002E-0x0032 (temperatures and sensor numbers) are sourced from the 0x447 broadcast, not polled.
  static const int POLL_MODULE_1_LOWEST_MV_NUMBER = 0x016C;
  static const int POLL_MODULE_1_LOWEST_CELL_MV = 0x016D;
  static const int POLL_MODULE_1_HIGHEST_MV_NUMBER = 0x016E;
  static const int POLL_MODULE_1_HIGH_CELL_MV = 0x016F;
  static const int POLL_MODULE_1_HIGH_TEMP = 0x0171;
  static const int POLL_MODULE_1_LOW_TEMP = 0x0173;
  static const int POLL_MODULE_2_LOWEST_MV_NUMBER = 0x0174;
  static const int POLL_MODULE_2_LOWEST_CELL_MV = 0x0175;
  static const int POLL_MODULE_2_HIGHEST_MV_NUMBER = 0x0176;
  static const int POLL_MODULE_2_HIGH_CELL_MV = 0x0177;
  static const int POLL_MODULE_2_HIGH_TEMP = 0x0179;
  static const int POLL_MODULE_2_LOW_TEMP = 0x017B;
  static const int POLL_MODULE_3_LOWEST_MV_NUMBER = 0x017C;
  static const int POLL_MODULE_3_LOWEST_CELL_MV = 0x017D;
  static const int POLL_MODULE_3_HIGHEST_MV_NUMBER = 0x017E;
  static const int POLL_MODULE_3_HIGH_CELL_MV = 0x017F;
  static const int POLL_MODULE_3_HIGH_TEMP = 0x0181;
  static const int POLL_MODULE_3_LOW_TEMP = 0x0183;
  static const int POLL_MODULE_4_LOWEST_MV_NUMBER = 0x0184;
  static const int POLL_MODULE_4_LOWEST_CELL_MV = 0x0185;
  static const int POLL_MODULE_4_HIGHEST_MV_NUMBER = 0x0186;
  static const int POLL_MODULE_4_HIGH_CELL_MV = 0x0187;
  static const int POLL_MODULE_4_HIGH_TEMP = 0x0189;
  static const int POLL_MODULE_4_LOW_TEMP = 0x018B;
  static const int POLL_MODULE_5_LOWEST_MV_NUMBER = 0x018C;
  static const int POLL_MODULE_5_LOWEST_CELL_MV = 0x018D;
  static const int POLL_MODULE_5_HIGHEST_MV_NUMBER = 0x018E;
  static const int POLL_MODULE_5_HIGH_CELL_MV = 0x018F;
  static const int POLL_MODULE_5_HIGH_TEMP = 0x0191;
  static const int POLL_MODULE_5_LOW_TEMP = 0x0193;
  static const int POLL_MODULE_6_LOWEST_MV_NUMBER = 0x0194;
  static const int POLL_MODULE_6_LOWEST_CELL_MV = 0x0195;
  static const int POLL_MODULE_6_HIGHEST_MV_NUMBER = 0x0196;
  static const int POLL_MODULE_6_HIGH_CELL_MV = 0x0197;
  static const int POLL_MODULE_6_HIGH_TEMP = 0x0199;
  static const int POLL_MODULE_6_LOW_TEMP = 0x019B;
  static const int POLL_MODULE_7_LOWEST_MV_NUMBER = 0x019C;
  static const int POLL_MODULE_7_LOWEST_CELL_MV = 0x019D;
  static const int POLL_MODULE_7_HIGHEST_MV_NUMBER = 0x019E;
  static const int POLL_MODULE_7_HIGH_CELL_MV = 0x019F;
  static const int POLL_MODULE_7_HIGH_TEMP = 0x01A1;
  static const int POLL_MODULE_7_LOW_TEMP = 0x01A3;
  static const int POLL_MODULE_8_LOWEST_MV_NUMBER = 0x01A4;
  static const int POLL_MODULE_8_LOWEST_CELL_MV = 0x01A5;
  static const int POLL_MODULE_8_HIGHEST_MV_NUMBER = 0x01A6;
  static const int POLL_MODULE_8_HIGH_CELL_MV = 0x01A7;
  static const int POLL_MODULE_8_HIGH_TEMP = 0x01A9;
  static const int POLL_MODULE_8_LOW_TEMP = 0x01AB;
  static const int POLL_MODULE_9_LOWEST_MV_NUMBER = 0x01AC;
  static const int POLL_MODULE_9_LOWEST_CELL_MV = 0x01AD;
  static const int POLL_MODULE_9_HIGHEST_MV_NUMBER = 0x01AE;
  static const int POLL_MODULE_9_HIGH_CELL_MV = 0x01AF;
  static const int POLL_MODULE_9_HIGH_TEMP = 0x01B1;
  static const int POLL_MODULE_9_LOW_TEMP = 0x01B3;
  static const int POLL_MODULE_10_LOWEST_MV_NUMBER = 0x01B4;
  static const int POLL_MODULE_10_LOWEST_CELL_MV = 0x01B5;
  static const int POLL_MODULE_10_HIGHEST_MV_NUMBER = 0x01B6;
  static const int POLL_MODULE_10_HIGH_CELL_MV = 0x01B7;
  static const int POLL_MODULE_10_HIGH_TEMP = 0x01B9;
  static const int POLL_MODULE_10_LOW_TEMP = 0x01BB;
  static const int POLL_FOR_ORIGINAL_CALIBRATION = 0x1FFE;
  static const int POLL_FOR_CURRENT_CALIBRATION = 0x1FFC;

  static const uint16_t MAX_CELL_DEVIATION_MV = 230;
  static const uint16_t MAX_CELL_VOLTAGE_MV = 3650;  //Charging stops if one cell exceeds this value
  static const uint16_t MIN_CELL_VOLTAGE_MV = 2800;  //Discharging stops if one cell goes below this value

  //Max 0x438-vs-0x444 disagreement before 0x438 is treated as not carrying pack voltage
  static const uint16_t VOLTAGE_CROSSCHECK_TOLERANCE_DV = 50;

  /* Native BMS termination (on by default, see handle_charge_session). Runs a real AC charge session on
  the pack bus so the BMS ends the charge itself and recalibrates SOC to 100%, instead of BE stopping the
  charge at its own cell clamp. An insulation fault keeps the BMS out of the charge context; the
  isolation-monitor-disable setting (on by default) normally keeps that clear. Inert while off. */
  static const uint16_t SESSION_TAPER_START_MV = 3500;  // taper start, and where the BMS advisory hands over
  static const uint16_t SESSION_TAPER_END_MV = 3752;    // top of the observed 3742-3753 termination band
  static const uint8_t SESSION_TAIL_CURRENT_dA = 45;    // tail has to clear the site's net-delivery floor
  static const uint16_t SESSION_CELL_CLAMP_MV =
      3780;  // backstop above full+overshoot; applies only while a session owns the top
  static const uint16_t SESSION_DELTA_LIMIT_MV = 400;     // real cars run 250-360mV of spread at the top
  static const uint32_t SESSION_OBC_CAP_W = 7000;         // donor OBC maximum offer (0x47E b4 = 0x47)
  static const int16_t SESSION_ARM_CURRENT_dA = 20;       // arm on 2.0A of charge current...
  static const uint32_t SESSION_ARM_DWELL_MS = 30000;     // ...sustained this long
  static const int16_t SESSION_REARM_DISCHARGE_dA = -20;  // a real discharge releases the post-charge hold
  static const uint32_t SESSION_REARM_DWELL_MS = 30000;
  static const uint32_t SESSION_REQUEST_DWELL_MS = 1000;      // hold 0x24A=80 before advancing to 84
  static const uint32_t SESSION_GRANT_TIMEOUT_MS = 15000;     // real chargers walk away if no grant arrives
  static const uint32_t SESSION_BACKOFF_MS = 900000;          // wait 15min before asking again
  static const uint32_t SESSION_RAMP_IDLE_HOLD_MS = 4000;     // 0x36D idle prearm before the work-mode ramp
  static const uint32_t SESSION_GRANT_MIRROR_MS = 1500;       // act without 0x345 if the mirror goes quiet
  static const uint32_t SESSION_MIRROR_FRESH_MS = 500;        // 0x345 must be this recent to fast-confirm
  static const uint16_t SESSION_TERMINATION_FLOOR_MV = 3700;  // a grant-zero below this is an abort, not full
  static const uint32_t SESSION_FINISH_ACK_MS = 4000;
  static const uint32_t SESSION_DONE_TIMEOUT_MS = 30000;  // finish anyway if the charge flag lags the grant
  static const uint32_t SESSION_HOLD_SETTLE_MS = 2500;    // 0x24A 8C -> 80 once the pack is resting

  /* Session states. The session only ever runs in place on an already closed pack: 0x86 -> 0x85 ->
  (BMS terminates) -> 0x84, no contactor movement. HOLD keeps the frames alive at rest values, because
  ceasing 0x36D self-opens the pack ~0.5s later - they stop only when the pack is opening anyway. */
  static const uint8_t CHG_SESSION_IDLE = 0;
  static const uint8_t CHG_SESSION_REQUEST = 1;    // 0x24A=80, 0x47E b2 01->03
  static const uint8_t CHG_SESSION_READY = 2;      // 0x24A=84, 0x47E b2=07, waiting for the charge flag
  static const uint8_t CHG_SESSION_CHARGING = 3;   // 0x24A=88, 0x47E b2=0C, BMS granted
  static const uint8_t CHG_SESSION_FINISHING = 4;  // grant withdrawn, acknowledging the end
  static const uint8_t CHG_SESSION_HOLD = 5;       // resting at 0x84, keepalive only

  // Balancing enabled: cycle the contactors after a termination. Opt-in, off by default.
  static const uint8_t BALANCING_IDLE = 0;
  static const uint8_t BALANCING_ARMED = 1;         // terminated, letting the session settle first
  static const uint8_t BALANCING_OPENING = 2;       // open asked for, waiting for the pack to go
  static const uint8_t BALANCING_WAITING = 3;       // open, running the configured hold
  static const uint8_t BALANCING_CLOSING = 4;       // close asked for, waiting for the pack to come back
  static const uint8_t BALANCING_CLOSE_FAILED = 5;  // pack would not close, left open for a person
  static const uint8_t BALANCING_CLOSE_RETRIES = 2;
  static const uint32_t BALANCING_CLOSE_BACKOFF_MS = 45000;
  static const uint32_t BALANCING_SETTLE_MS = 10000;         // session reaches rest ~5s after termination
  static const uint32_t BALANCING_MOVE_TIMEOUT_MS = 120000;  // give up if the pack will not move

  uint16_t rampdown_power = 0;
  uint16_t poll_state = POLL_FOR_ORIGINAL_CALIBRATION;
  uint16_t pid_reply = 0;
  uint16_t battery_voltage = 0;                  // Whole volts from 0x444, used for the 0x441 link voltage
  uint16_t battery_voltage_dV = 0;               // Deci-volts from 0x438, primary pack voltage
  uint16_t battery_insulation_ohm_per_volt = 0;  // 0x43A, multiply by pack voltage for Ohms
  uint16_t battery_highprecision_SOC = 0;
  uint16_t battery_estimated_SOC = 0;
  uint16_t BMS_SOC = 0;
  uint16_t BMS_lowest_cell_voltage_mV = 3300;
  uint16_t BMS_highest_cell_voltage_mV = 3300;
  uint16_t BMS_allowed_charge_power = 0;
  uint16_t BMS_charge_times = 0;
  uint16_t BMS_allowed_discharge_power = 0;
  uint16_t BMS_total_charged_ah = 0;
  uint16_t BMS_total_discharged_ah = 0;
  uint16_t BMS_total_charged_kwh = 0;
  uint16_t BMS_total_discharged_kwh = 0;
  uint16_t BMS_times_full_power = 0;
  uint16_t BMS_capacity_original_calibration = 0;
  uint16_t BMC_SOC_original_calibration = 0;
  uint16_t BMS_capacity_current_calibration = 0;
  uint16_t BMC_SOC_current_calibration = 0;
  uint16_t seed = 0;
  uint16_t solvedKey = 0;

  static const uint16_t CELL_BALANCE_TIME_DID_BASE = 0x003F;
  static const uint16_t CELL_BALANCE_TIME_TIMEOUT_MS = 600;
  // 126 cells take about 27s, so the cap needs headroom for the largest packs on a slow bus.
  static const uint32_t CELL_BALANCE_TIME_SCAN_TIMEOUT_MS = 90000;
  static const uint8_t CELL_BALANCE_TIME_RETRIES = 1;
  BydCellBalanceTimeData cell_balance_time_data;
  unsigned long cell_balance_time_scan_millis = 0;
  unsigned long cell_balance_time_request_millis = 0;
  uint8_t cell_balance_time_cell = 0;
  uint8_t cell_balance_time_retries = 0;
  bool cell_balance_time_queued = false;
  bool cell_balance_time_active = false;
  bool cell_balance_time_waiting = false;
  std::atomic<bool> cell_balance_time_requested{false};
  bool BMS_times_full_power_valid = false;

  bool awaiting_cell_balance_reply() const { return cell_balance_time_active && cell_balance_time_waiting; }
  bool diagnostics_idle() const;
  void begin_cell_balance_time_scan(unsigned long currentMillis);
  bool handle_cell_balance_time_reply(const CAN_frame& frame);
  void advance_cell_balance_time_cell();
  void handle_cell_balance_time_poll(unsigned long currentMillis);
  void finish_cell_balance_time_scan();

  int16_t battery_temperature_ambient = 0;
  int16_t battery_calc_min_temperature = 0;
  int16_t battery_calc_max_temperature = 0;
  int16_t battery_current_dA = 0;  // 0x444, deci-amps, negative while charging
  int16_t BMS_lowest_cell_temperature = 0;
  int16_t BMS_highest_cell_temperature = 0;
  int16_t BMS_average_cell_temperature = 0;

  static const uint8_t NOT_DETERMINED_YET = 0;
  static const uint8_t STANDARD_RANGE = 1;
  static const uint8_t EXTENDED_RANGE = 2;
  static const uint8_t NOT_RUNNING = 0xFF;
  static const uint8_t STARTED = 0;
  static const uint8_t RUNNING_STEP_1 = 1;
  static const uint8_t RUNNING_STEP_2 = 2;
  static const uint8_t RUNNING_STEP_3 = 3;
  static const uint8_t RUNNING_STEP_4 = 4;
  uint8_t battery_type = NOT_DETERMINED_YET;
  uint8_t stateMachineClearCrash = NOT_RUNNING;
  uint8_t stateMachineCalibrateSOC = NOT_RUNNING;

  // Isolation monitor routine (RoutineControl 0x2008); shares the 0x7E7 session with SOC cal.
  uint8_t stateMachineIsoRoutine = NOT_RUNNING;
  uint8_t isoRoutineAction = 0;  // 1 disable (31 01), 2 enable (31 02)
  uint8_t increaseTimeoutIso = 0;
  // keep_iso_disabled enforcement: re-send disable after each BMS start (monitor re-enables on power-up)
  bool bms_was_alive = false;
  bool iso_reassert_needed = false;
  bool iso_measurement_was_active = false;
  unsigned long bms_alive_since_ms = 0;
  unsigned long iso_reassert_attempt_ms = 0;

  // DTC readout: request 0x19 02 09, reassemble the 0x59 02 ISO-TP reply, parse 4 bytes per DTC.
  static const int MAX_DTC_COUNT = 30;
  uint8_t stateMachineReadDTC = NOT_RUNNING;
  uint8_t stateMachineEraseDTC = NOT_RUNNING;
  unsigned long dtc_request_millis = 0;
  uint8_t dtc_buffer[140] = {0};
  uint16_t dtc_rx_expected = 0;
  uint16_t dtc_rx_len = 0;
  bool dtc_rx_active = false;
  void parseDTCResponse();

  /* Software contactor control: step the transmitted 0x12D frame through the same payload
  states the real VCU uses (taken from CAN logs of two cars). Byte 6 is a rolling counter,
  byte 7 the 0x441-style checksum over bytes 0-6.
  0x12D states (bytes 0-5):
  - standby:      50 14 02 10 04 31  ignition off, pack open
  - active ack:   50 18 02 20 04 31  ignition off, pack closed (also seen throughout AC charging)
  - close/active: A0 28 02 A0 0C 71  BMS starts precharge ~60ms later
  - drive ready:  A0 28 00 22 0C 31  car sends this ~0.4s after the main contactor closes
  - shutdown:     A0 28 02 60 04 31  BMS drops HV-active ~0.3s later
  Car power-off: shutdown (~1.4s) -> active ack -> pack opens (instant when driving, waits for
  charging to finish) -> ~2.5s -> standby. The pack never opens on the shutdown step itself.
  Logs show another close pattern (50 18 12 20 44 31) for parked aux/DC-DC closes - not used here.
  0x344 byte 0 reports state, by bit:
  - bit7 0x80 = main contactor closed
  - bit6 0x40 = precharge in progress
  - bit2 0x04 = HV active
  - bit1 0x02 = drive flag (set when idle/driving, clear while charging)
  - bit0 0x01 = charge flag
  Drive close: 02 -> 42 -> 82 -> 86. Charge close: 02 -> 42 -> 82 -> 81. Charge open: 81 -> 80 -> 00.
  In charge mode the BMS ignores the 0x12D shutdown and opens on charge completion instead.
  0x84 (closed, HV, no mode flag) also opens and re-closes fine once byte 7 is correct. */
  static const uint8_t CONTACTORS_CLOSING = 0;             // Close pattern sent, drive-ready transition pending
  static const uint8_t CONTACTORS_ACTIVE = 1;              // Drive-ready pattern, closed and running
  static const uint8_t CONTACTORS_AWAIT_ZERO_CURRENT = 2;  // Open asked for, waiting for current to drop
  static const uint8_t CONTACTORS_OPENING = 3;             // Holding the shutdown pattern (~1.4s like the car)
  static const uint8_t CONTACTORS_STANDBY = 4;             // Standby pattern, contactors open
  static const uint8_t CONTACTORS_OPEN_REQUESTED = 5;      // Active-ack held, waiting for the BMS to open
  static const uint8_t CONTACTORS_OPEN_SETTLE = 6;         // Open confirmed, settling before standby
  static const uint8_t CONTACTORS_BOOT_ESTOP = 7;          // Booted held open (fault/stop/inverter), holding
                                                           // active-ack until 0x344 tells us the pack state

  // 0x344 byte 0 feedback bits
  static const uint8_t BMS_FEEDBACK_MAIN_CLOSED = 0x80;
  static const uint8_t BMS_FEEDBACK_PRECHARGING = 0x40;
  static const uint8_t BMS_FEEDBACK_HV_ACTIVE = 0x04;
  static const uint8_t BMS_FEEDBACK_DRIVE_FLAG = 0x02;
  static const uint8_t BMS_FEEDBACK_CHARGE_FLAG = 0x01;

  // Car ramps the link to pack over ~900ms: ~62% at 100ms, ~95% at 400ms
  static const uint32_t PRECHARGE_RAMP_MS = 900;
  // BMS never moved - report the link anyway rather than hold the pack open waiting on ourselves
  static const uint32_t PRECHARGE_WAIT_MAX_MS = 3000;
  static const uint8_t PRECHARGE_WAIT = 0;                 // closed, BMS hasn't started precharging
  static const uint8_t PRECHARGE_RAMP = 1;                 // BMS moved, walking the link up to pack
  static const uint8_t PRECHARGE_DONE = 2;                 // link at pack voltage, latched until the pack opens
  static const int16_t OPEN_MAX_CURRENT_dA = 25;           // Open only below 2.5A
  static const uint32_t ZERO_CURRENT_MIN_WAIT_MS = 5000;   // Let the inverter settle before trusting current
  static const uint32_t ZERO_CURRENT_TIMEOUT_MS = 10000;   // Force the open if current never drops
  static const uint32_t OPEN_SHUTDOWN_HOLD_MS = 1500;      // Hold the shutdown pattern, like the car
  static const uint32_t OPEN_CONFIRM_TIMEOUT_MS = 6000;    // Warn if the BMS hasn't opened by now
  static const uint32_t OPEN_TO_STANDBY_DELAY_MS = 2500;   // Car's wait between open and standby
  static const uint32_t CLOSE_CONFIRM_TIMEOUT_MS = 15000;  // Warn if the BMS hasn't closed by now

  uint8_t contactorState = CONTACTORS_CLOSING;  // Boot default: close right away, as before
  uint8_t contactor_feedback = 0;               // Raw 0x344 byte 0
  uint8_t contactor_feedback_state = 0;         // 0x344 byte 1; 0x00 while open, bit 0x40 once the BMS moves
  uint8_t prechargeState = PRECHARGE_WAIT;
  bool prechargeEdgeSeen = false;
  bool prechargeInitialised = false;
  unsigned long prechargeRampStartMillis = 0;
  unsigned long prechargeWaitStartMillis = 0;
  unsigned long contactorStateEntryMillis = 0;
  unsigned long closeConfirmStartMillis = 0;
  unsigned long lastCurrentSampleMillis = 0;
  unsigned long lastContactorFeedbackMillis = 0;  // 0 = no 0x344 received yet
  bool closeConfirmPending = false;               // Only for user closes, not the boot default
  bool openTimeoutEventSent = false;              // Open-delay warning fired once per attempt
  bool requestContactorOpen = false;
  bool requestContactorClose = false;
  bool previousContactorsAllowedClosed = false;  // Combined fault + equipment-stop + inverter-permission state
  bool contactorControlInitialized = false;

  void set_12D_payload(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
  void handle_contactor_control(unsigned long currentMillis);

  uint8_t chargeSessionState = CHG_SESSION_IDLE;
  uint8_t balancingState = BALANCING_IDLE;
  unsigned long balancingStateMillis = 0;
  uint8_t balancingCloseAttempts = 0;
  bool contactorOpenOptional = false;
  bool balancingCycleDone = false;  // one cycle per real discharge, like the session re-arm
  unsigned long balancingDischargeSinceMillis = 0;
  bool chargeSessionTerminated = false;  // last session ended on a grant edge, not an interruption
  bool chargeTerminatedRails = false;    // rails stay lifted after the session ends, until cells relax
  bool chargeSessionHeartbeat = false;
  bool chargeRampDone = false;
  bool chargeDonePending = false;  // grant edge confirmed, stop offering current
  bool chargeGrantZeroCandidate = false;
  bool chargeRearmAllowed = true;        // cleared after a termination until a real discharge
  bool chargeRestFlagSeenClear = false;  // pack seen out of 0x85 since the rest began
  bool chargeBackoffActive = false;
  uint8_t chargeGrant = 0;        // 0x347 byte 1, the BMS -> charger grant
  uint8_t chargeGrantMirror = 0;  // 0x345 byte 4, mirrors the grant a frame ahead
  uint8_t chargeGrantPrevious = 0;
  unsigned long chargeGrantMirrorMillis = 0;
  unsigned long chargeSessionEntryMillis = 0;
  unsigned long chargeRampStartMillis = 0;
  unsigned long chargeGrantCandidateMillis = 0;
  unsigned long chargeArmCurrentSinceMillis = 0;
  unsigned long chargeRearmDischargeSinceMillis = 0;
  unsigned long chargeBackoffStartMillis = 0;

  void handle_charge_session(unsigned long currentMillis);
  void handle_balancing(unsigned long currentMillis);
  void handle_charge_grant(uint8_t grant);
  void confirm_charge_termination();
  void start_charge_session(unsigned long currentMillis);
  void enter_charge_delivery(unsigned long currentMillis);
  void hold_charge_session(unsigned long currentMillis, const char* reason, bool backoff);
  void end_charge_session(const char* reason);
  void transmit_charge_session(unsigned long currentMillis);

  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t frame6_counter = 0xB;
  uint8_t BMS_SOH = 99;
  uint8_t BMS_min_cell_voltage_number = 0;
  uint8_t BMS_min_temp_module_number = 0;
  uint8_t BMS_max_cell_voltage_number = 0;
  uint8_t BMS_max_temp_module_number = 0;
  uint8_t battery_frame_index = 0;
  uint8_t discharge_status = 14;
  uint8_t increaseTimeoutSOC = 0;
  static const uint8_t REJECTED = 1;
  static const uint8_t APPROVED = 2;
  uint8_t servicemode = NOT_DETERMINED_YET;
  uint8_t secondsSinceStartup = 0;

  bool BMS_voltage_available = false;
  bool battery_insulation_valid = false;        // Zero is a valid 0x43A fault reading, so track receipt separately
  bool battery_iso_measurement_active = false;  // 0x35E b0 bit0x80
  unsigned long last_35E_ms = 0;                // 0 = 0x35E not yet received (staleness)
  bool calibrationAH_seeded = false;

  int16_t battery_daughterboard_temperatures[13] = {-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40};
  uint16_t battery_cellvoltages[MAX_AMOUNT_CELLS] = {0};

  /* Extra CAN info 
  - 0x40D supposedly has vehicle model in frame1
  - 0x2B6 contains date in frame0-6
  - 
  
    */

  /*12D 
  - Byte0(bits7:6) = IG1 Relay state
  - Byte1(bits3:2) = IG3 Relay state
  - Byte1(bits5:4) = IG4 Relay state*/
  CAN_frame ATTO_3_12D = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x12D,
                          .data = {0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71, 0xCF, 0x49}};
  CAN_frame ATTO_3_441 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x441,
                          .data = {0x98, 0x3A, 0x88, 0x13, 0x07, 0x00, 0xFF, 0x8C}};
  /* Charge-session frames, sent at 100ms while a session runs. 0x36A = precharge/session permission
  (static, also the marker the BMS counts charge sessions on). 0x36D = DC work mode and the keep-alive
  that holds the pack closed: b0-1 link voltage (LE, whole volts), b3 heartbeat, b4 output current,
  b6 charger temperature. 0x24A/0x47E are the charge request itself: 0x24A b0 80->84->88->8C, 0x47E b2
  13->01->03->07->0C->0E->0F, with 0x47E b3/b4 the current and power offer. Byte 7 is the checksum. */
  CAN_frame ATTO_3_36A = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x36A,
                          .data = {0xF9, 0x01, 0x00, 0x00, 0x38, 0x00, 0x00, 0xCD}};
  CAN_frame ATTO_3_36D = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x36D,
                          .data = {0xA2, 0x01, 0x32, 0x8A, 0x83, 0x02, 0x47, 0xD4}};
  CAN_frame ATTO_3_24A = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x24A,
                          .data = {0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0xBB}};
  CAN_frame ATTO_3_47E = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x47E,
                          .data = {0x00, 0x47, 0x13, 0x00, 0x00, 0x18, 0x00, 0x8D}};
  CAN_frame ATTO_3_7E7_POLL = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E7,  //Poll PID 03 22 1F FE (POLL_FOR_ORIGINAL_CALIBRATION)
                               .data = {0x03, 0x22, 0x1F, 0xFE, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_ACK = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E7,  //ACK frame for long PIDs
                              .data = {0x30, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_CLEAR_CRASH = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x7E7,
                                      .data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_RESET_SOC = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x7E7,  //This sets SOC to 100.00% (0x27 10) , and AH to 150.00 (0x3A 98)
                                    .data = {0x07, 0x2E, 0x1F, 0xFC, 0x10, 0x27, 0x98, 0x3A}};
  CAN_frame ATTO_3_7E7_READ_DTC = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 8,
                                   .ID = 0x7E7,  //ReadDTCInformation, reportDTCByStatusMask, mask 0x09
                                   .data = {0x03, 0x19, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_DTC_FC = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x7E7,  //Flow control for the DTC reply, BS 0 (send all), STmin 5ms
                                 .data = {0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};

  void handle_auto_soc_calibration(bool crit_taper, uint32_t dt_ms, uint32_t now_ms);
};

#endif
