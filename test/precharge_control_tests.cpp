#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/communication/precharge_control/precharge_control.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/datalayer/datalayer_extended.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/utils/events.h"

#include "Arduino.h"

namespace {

// The state machine runs from the 10 ms core loop, so a stuck sequence shows
// up as repeated actuation rather than a single event. Ticking many times is
// what makes a restart loop visible.
constexpr int kTicks = 50;
constexpr unsigned long kTickMs = 10;
// CONTACTOR_OFF in precharge_control.cpp with the default (normally-closed)
// wiring: the state that puts the precharge resistor in circuit.
constexpr uint8_t kContactorOff = 1;

void runTicks(int ticks = kTicks) {
  for (int i = 0; i < ticks; ++i) {
    handle_precharge_control(i * kTickMs);
  }
}

// Everything the sequence needs in order to run. Individual tests then break
// exactly one of these.
void setUpRunnableConditions() {
  init_hal();
  precharge_control_enabled = true;
  datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
  datalayer.system.info.start_precharging = true;
  datalayer.system.status.inverter_allows_contactor_closing = true;
  datalayer.system.status.battery_allows_contactor_closing = false;
  datalayer.system.info.equipment_stop_active = false;
  datalayer.system.status.system_status = ACTIVE;
  datalayer.battery.status.real_bms_status = BMS_STANDBY;
  precharge_max_precharge_time_before_fault = 15000;
  Precharge_max_PWM_Freq = 34000;
  datalayer_extended.meb.BMS_voltage_intermediate_dV = 0;
  clear_tone_writes();
  reset_all_events();
  clear_pin_writes();
}

bool event_is_active(EVENTS_ENUM_TYPE event) {
  const EVENTS_STRUCT_TYPE* entry = get_event_pointer(event);
  return entry->state == EVENT_STATE_ACTIVE || entry->state == EVENT_STATE_ACTIVE_LATCHED;
}

bool pin_driven(gpio_num_t pin) {
  for (const PinWrite& write : get_pin_writes()) {
    if (write.pin == static_cast<uint8_t>(pin)) {
      return true;
    }
  }
  return false;
}

bool pin_driven_to(gpio_num_t pin, uint8_t value) {
  for (const PinWrite& write : get_pin_writes()) {
    if (write.pin == static_cast<uint8_t>(pin) && write.value == value) {
      return true;
    }
  }
  return false;
}

bool inverterDisconnectContactorWasDriven() {
  const uint8_t pin = static_cast<uint8_t>(esp32hal->INVERTER_DISCONNECT_CONTACTOR_PIN());
  for (const PinWrite& write : get_pin_writes()) {
    if (write.pin == pin) {
      return true;
    }
  }
  return false;
}

}  // namespace

// The start gate used to ignore system_status, so a battery asking to precharge
// while the system was in FAULT started a sequence that the next tick aborted -
// toggling the inverter-disconnect contactor on every pass of the core loop.
TEST(PrechargeControlTests, DoesNotStartWhileSystemStatusIsFault) {
  setUpRunnableConditions();
  datalayer.system.status.system_status = FAULT;

  runTicks();

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);
  EXPECT_FALSE(inverterDisconnectContactorWasDriven())
      << "the inverter-disconnect contactor must not be switched while precharge cannot run";
}

// Same asymmetry on the equipment-stop path: the contactor must stay put during
// an equipment stop rather than chattering.
TEST(PrechargeControlTests, DoesNotStartWhileEquipmentStopIsActive) {
  setUpRunnableConditions();
  datalayer.system.info.equipment_stop_active = true;

  runTicks();

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);
  EXPECT_FALSE(inverterDisconnectContactorWasDriven())
      << "the inverter-disconnect contactor must not be switched during an equipment stop";
}

// The condition the start gate already checked keeps working.
TEST(PrechargeControlTests, DoesNotStartWithoutInverterPermission) {
  setUpRunnableConditions();
  datalayer.system.status.inverter_allows_contactor_closing = false;

  runTicks();

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);
}

// The fix must not stop a legitimate sequence from starting.
TEST(PrechargeControlTests, StartsWhenEveryConditionHolds) {
  setUpRunnableConditions();

  handle_precharge_control(0);
  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_START);

  handle_precharge_control(kTickMs);
  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);
  EXPECT_TRUE(inverterDisconnectContactorWasDriven()) << "starting the sequence does drive the contactor";
}

// A running sequence still aborts when a condition drops away - the abort side
// of the shared predicate.
TEST(PrechargeControlTests, RunningSequenceAbortsWhenEquipmentStopArrives) {
  setUpRunnableConditions();

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);

  clear_pin_writes();
  datalayer.system.info.equipment_stop_active = true;
  handle_precharge_control(2 * kTickMs);

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);
  // Aborting is not just a state change: the precharge PWM has to stop and the
  // inverter-disconnect contactor has to go back to its resting position, or
  // the resistor stays in circuit with the sequence no longer running.
  EXPECT_TRUE(pin_driven_to(esp32hal->HIA4V1_PIN(), LOW)) << "the precharge PWM pin must be driven low on abort";
  EXPECT_TRUE(pin_driven(esp32hal->INVERTER_DISCONNECT_CONTACTOR_PIN()))
      << "the inverter-disconnect contactor must be switched back on abort";
}

// ...and having aborted, it must not immediately start again, which is the loop
// itself: before the fix this cycled IDLE -> START -> PRECHARGING -> IDLE at the
// core-loop rate for as long as the blocking condition lasted.
TEST(PrechargeControlTests, DoesNotRestartAfterAbortingWhileStillBlocked) {
  setUpRunnableConditions();

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);

  datalayer.system.info.equipment_stop_active = true;
  handle_precharge_control(2 * kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);

  clear_pin_writes();
  runTicks();

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_IDLE);
  EXPECT_FALSE(inverterDisconnectContactorWasDriven()) << "the contactor rattle is exactly this write repeating";
}

// --- The failure paths -----------------------------------------------------

// A precharge that never completes must fail rather than run forever with the
// resistor in circuit - that is what the timeout is for.
TEST(PrechargeControlTests, TimeoutDuringPrechargingCausesFailure) {
  setUpRunnableConditions();
  precharge_max_precharge_time_before_fault = 100;

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);

  handle_precharge_control(kTickMs + 100);

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_FAILURE);
  EXPECT_TRUE(event_is_active(EVENT_AUTOMATIC_PRECHARGE_FAILURE));
  EXPECT_FALSE(datalayer.system.info.start_precharging) << "a failed sequence must not be retried";
}

// A BMS fault mid-precharge is a hard failure, not an ordinary abort: it takes
// the first branch, so it must latch rather than returning to idle where the
// sequence could immediately restart.
TEST(PrechargeControlTests, BmsFaultDuringPrechargingCausesFailureNotIdle) {
  setUpRunnableConditions();
  if (!battery) {
    user_selected_battery_type = BatteryType::TestFake;
    setup_battery();
  }
  ASSERT_NE(battery, nullptr);
  datalayer.system.info.start_precharging = true;
  datalayer.battery.status.real_bms_status = BMS_STANDBY;

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);

  datalayer.battery.status.real_bms_status = BMS_FAULT;
  handle_precharge_control(2 * kTickMs);

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_FAILURE)
      << "a BMS fault must latch, not fall through to the recoverable idle path";
}

// The failure state requires a reboot to leave. Nothing - not even every
// condition being healthy again - may restart the sequence.
TEST(PrechargeControlTests, FailureStateIsTerminal) {
  setUpRunnableConditions();
  precharge_max_precharge_time_before_fault = 100;

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  handle_precharge_control(kTickMs + 100);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_FAILURE);

  // Everything healthy again, and a fresh request.
  datalayer.system.info.start_precharging = true;
  datalayer.battery.status.real_bms_status = BMS_STANDBY;
  datalayer.system.status.system_status = ACTIVE;
  datalayer.system.info.equipment_stop_active = false;
  clear_pin_writes();
  runTicks();

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_FAILURE)
      << "only a reboot may clear a precharge failure";
  // The failure branch re-asserts the safe position on every pass rather than
  // leaving the pin alone, so the property is that it is never driven to the
  // opposite state - the resistor must not be switched back in.
  EXPECT_FALSE(pin_driven_to(esp32hal->INVERTER_DISCONNECT_CONTACTOR_PIN(), kContactorOff))
      << "a latched failure must never re-open the inverter-disconnect contactor";
}

// The success path: the battery reporting its contactors closed is what ends
// the sequence, and it must end as COMPLETED rather than idle.
TEST(PrechargeControlTests, CompletesWhenBatteryAllowsContactorClosing) {
  setUpRunnableConditions();

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  ASSERT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_PRECHARGING);

  datalayer.system.status.battery_allows_contactor_closing = true;
  handle_precharge_control(2 * kTickMs);

  EXPECT_EQ(datalayer.system.status.precharge_status, AUTO_PRECHARGE_COMPLETED);
}

// --- The PWM regulation loop -----------------------------------------------
//
// While precharging, the firmware drives the resistor with a PWM whose
// frequency it walks towards the pack voltage: a large gap moves in fixed
// steps, a closing gap moves proportionally, and the result is clamped to the
// configured band. Only the firmware's half of that loop is exercised here -
// the generator's actual response to it is a bench matter - but the control
// law itself is arithmetic and needs no hardware.

namespace {

// Puts the sequence into PRECHARGING with a known starting frequency, then
// clears the recorder so a test sees only what its own step drove.
void beginPrechargingAt(int32_t pack_dV, int32_t external_dV) {
  setUpRunnableConditions();
  datalayer.battery.status.voltage_dV = pack_dV;
  datalayer_extended.meb.BMS_voltage_intermediate_dV = external_dV;
  handle_precharge_control(0);
  handle_precharge_control(kTickMs);
  clear_tone_writes();
}

uint32_t last_tone() {
  const std::vector<ToneWrite>& writes = get_tone_writes();
  return writes.empty() ? 0 : writes.back().freq;
}

}  // namespace

// Starting the sequence sets the default frequency, so every run begins from
// the same place regardless of where the last one ended.
TEST(PrechargeControlTests, StartingTheSequenceDrivesTheDefaultFrequency) {
  setUpRunnableConditions();
  clear_tone_writes();

  handle_precharge_control(0);
  handle_precharge_control(kTickMs);

  ASSERT_FALSE(get_tone_writes().empty());
  EXPECT_EQ(get_tone_writes().front().freq, Precharge_default_PWM_Freq);
}

// External below pack: the DC link still has to charge, so the frequency rises.
TEST(PrechargeControlTests, FrequencyRisesWhileTheLinkIsBelowThePack) {
  beginPrechargingAt(4000, 3000);

  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3100;
  handle_precharge_control(2 * kTickMs);

  ASSERT_FALSE(get_tone_writes().empty());
  EXPECT_GT(last_tone(), Precharge_default_PWM_Freq);
}

// External above pack: back off.
TEST(PrechargeControlTests, FrequencyFallsWhenTheLinkOvershootsThePack) {
  beginPrechargingAt(3000, 4000);

  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3900;
  handle_precharge_control(2 * kTickMs);

  ASSERT_FALSE(get_tone_writes().empty());
  EXPECT_LT(last_tone(), Precharge_default_PWM_Freq);
}

// The step size is banded: far away moves a fixed 2000, and closer in moves
// proportionally to the gap. Getting these bands wrong makes the loop either
// crawl or oscillate.
TEST(PrechargeControlTests, StepSizeFollowsTheDistanceBands) {
  // Gap of 200 dV, beyond the 150 threshold: a flat 2000 step.
  beginPrechargingAt(4000, 3000);
  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3800;
  handle_precharge_control(2 * kTickMs);
  EXPECT_EQ(last_tone(), Precharge_default_PWM_Freq + 2000) << "a far gap steps by a flat amount";

  // Gap of 100 dV, inside 150 but beyond 80: six times the gap.
  beginPrechargingAt(4000, 3000);
  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3900;
  handle_precharge_control(2 * kTickMs);
  EXPECT_EQ(last_tone(), Precharge_default_PWM_Freq + 100 * 6);

  // Gap of 50 dV, the closest band: three times the gap.
  beginPrechargingAt(4000, 3000);
  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3950;
  handle_precharge_control(2 * kTickMs);
  EXPECT_EQ(last_tone(), Precharge_default_PWM_Freq + 50 * 3);
}

// The band is a hard limit: repeated steps towards it must saturate, not run
// away past the configured ceiling.
TEST(PrechargeControlTests, FrequencyIsClampedToTheConfiguredCeiling) {
  beginPrechargingAt(4000, 1000);

  int32_t external = 1000;
  for (int i = 0; i < 100; ++i) {
    external += 1;  // change every pass so the loop keeps regulating
    datalayer_extended.meb.BMS_voltage_intermediate_dV = external;
    handle_precharge_control((2 + i) * kTickMs);
  }

  EXPECT_EQ(last_tone(), Precharge_max_PWM_Freq) << "the frequency must saturate at the ceiling";
}

TEST(PrechargeControlTests, FrequencyIsClampedToTheConfiguredFloor) {
  beginPrechargingAt(1000, 4000);

  int32_t external = 4000;
  for (int i = 0; i < 100; ++i) {
    external -= 1;
    datalayer_extended.meb.BMS_voltage_intermediate_dV = external;
    handle_precharge_control((2 + i) * kTickMs);
  }

  EXPECT_EQ(last_tone(), Precharge_min_PWM_Freq);
}

// The loop only reacts to a NEW reading. Some packs report the external
// voltage every 100 ms while this runs every 10 ms, so regulating on a
// repeated value would step ten times per measurement and overshoot.
TEST(PrechargeControlTests, AnUnchangedReadingDoesNotMoveTheFrequency) {
  beginPrechargingAt(4000, 3000);

  datalayer_extended.meb.BMS_voltage_intermediate_dV = 3800;
  handle_precharge_control(2 * kTickMs);
  const uint32_t after_first = last_tone();
  clear_tone_writes();

  // Same reading again, several times.
  for (int i = 0; i < 5; ++i) {
    handle_precharge_control((3 + i) * kTickMs);
  }

  EXPECT_TRUE(get_tone_writes().empty()) << "a repeated reading must not drive the PWM again";
  EXPECT_EQ(after_first, Precharge_default_PWM_Freq + 2000);
}

// A zero reading means the pack has not reported yet, not that the link is
// flat - regulating on it would drive the frequency straight to the ceiling.
TEST(PrechargeControlTests, AZeroReadingIsIgnoredRatherThanTreatedAsEmpty) {
  beginPrechargingAt(4000, 3000);

  datalayer_extended.meb.BMS_voltage_intermediate_dV = 0;
  handle_precharge_control(2 * kTickMs);

  EXPECT_TRUE(get_tone_writes().empty()) << "an absent measurement must not regulate the loop";
}
