#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/parallel_safety.h"
#include "../Software/src/devboard/utils/events.h"

/* Does a fake triple setup raise a voltage-difference event?
 *
 * Reported on #2725 against the sentinel-latch change. The claim is worth
 * settling in a test rather than by reading, because the answer decides whether
 * the latch made the check fire where it should not, or whether it exposed
 * something that was always true and merely silent.
 *
 * These cases reproduce the fake triple as the firmware actually runs it:
 *
 *   Software.cpp:645-656   battery1 update_values(), then battery2
 *                          update_values() followed immediately by
 *                          check_parallel_battery_safety(2), then the same for 3.
 *   TEST-FAKE-BATTERY.cpp  instances 2 and 3 mirror battery 1's pack voltage
 *                          on every update_values() call.
 *
 * The mirror therefore lands in the same tick as the check that reads it, which
 * is the whole question: an instance that mirrors cannot drift.
 */

namespace {

constexpr uint16_t SENTINEL_DV = 3700;  // Datalayer init default, and 370.0 V

bool voltage_difference_active(EVENTS_ENUM_TYPE event) {
  const EVENTS_STRUCT_TYPE* e = get_event_pointer(event);
  return e->state == EVENT_STATE_ACTIVE || e->state == EVENT_STATE_ACTIVE_LATCHED;
}

}  // namespace

class FakeTripleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    reset_parallel_safety_state();  // Start every case from the module's boot state
    datalayer.battery.status.voltage_dV = SENTINEL_DV;
    datalayer.battery2.status.voltage_dV = SENTINEL_DV;
    datalayer.battery3.status.voltage_dV = SENTINEL_DV;
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.battery2_allowed_contactor_closing = false;
    datalayer.system.status.battery3_allowed_contactor_closing = false;
    battery2_detected = true;
    battery3_detected = true;
  }

  /* One pass of the main loop with all three instances driven by the fake
     battery, including the mirror that instances 2 and 3 perform. */
  void fake_triple_tick(uint16_t main_voltage_dV) {
    datalayer.battery.status.voltage_dV = main_voltage_dV;

    datalayer.battery2.status.voltage_dV = datalayer.battery.status.voltage_dV;  // TestFakeBattery mirror
    check_parallel_battery_safety(2);

    datalayer.battery3.status.voltage_dV = datalayer.battery.status.voltage_dV;  // TestFakeBattery mirror
    check_parallel_battery_safety(3);
  }

  void expect_no_difference_events(const char* context) {
    EXPECT_FALSE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT2)) << context;
    EXPECT_FALSE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT3)) << context;
  }
};

/* At the default fake voltage every instance reads exactly the sentinel, so the
   startup grace never lifts. Nothing fires - and nothing joins either, which is
   the accepted residual the change documents rather than a new symptom. */
TEST_F(FakeTripleTest, DefaultFakeVoltageRaisesNoEventAndNeverLatches) {
  for (int second = 0; second < 60; ++second) {
    fake_triple_tick(SENTINEL_DV);
  }

  expect_no_difference_events("fake triple resting at the default 370.0 V");
  EXPECT_FALSE(datalayer.system.status.battery2_allowed_contactor_closing);
  EXPECT_FALSE(datalayer.system.status.battery3_allowed_contactor_closing);
}

/* The user drags the fake pack voltage across its whole range, which is the
   only way a fake setup's voltage moves at all. The mirror runs in the same
   tick as the check, so the difference is zero at every step. */
TEST_F(FakeTripleTest, SweepingTheUserSetVoltageRaisesNoEvent) {
  for (uint16_t voltage_dV = 2450; voltage_dV <= 4040; voltage_dV += 10) {
    fake_triple_tick(voltage_dV);
    ASSERT_FALSE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT2))
        << "fired while sweeping upwards at " << voltage_dV << " dV";
  }
  for (uint16_t voltage_dV = 4040; voltage_dV >= 2450; voltage_dV -= 10) {
    fake_triple_tick(voltage_dV);
    ASSERT_FALSE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT3))
        << "fired while sweeping downwards at " << voltage_dV << " dV";
  }

  expect_no_difference_events("after a full sweep of the user set voltage");
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
  EXPECT_TRUE(datalayer.system.status.battery3_allowed_contactor_closing);
}

/* Passing back through exactly 370.0 V after the latch has engaged is the case
   the change is about. The old code would have skipped the check here; the new
   code runs it, and with a mirroring instance it still finds no difference. */
TEST_F(FakeTripleTest, ReturningToTheSentinelVoltageAfterLatchingRaisesNoEvent) {
  for (int second = 0; second < 5; ++second) {
    fake_triple_tick(3800);  // Off-sentinel, both instances agree: latch engages
  }
  ASSERT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);

  for (int second = 0; second < 30; ++second) {
    fake_triple_tick(SENTINEL_DV);
  }

  expect_no_difference_events("back at 370.0 V with the latch engaged");
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
  EXPECT_TRUE(datalayer.system.status.battery3_allowed_contactor_closing);
}

/* A single tick of disagreement - the webserver writing the user's new voltage
   between an instance's mirror and its check - is absorbed by the 3 second
   grace before the event is raised. */
TEST_F(FakeTripleTest, SingleTickOfMirrorLagRaisesNoEvent) {
  for (int second = 0; second < 5; ++second) {
    fake_triple_tick(3800);
  }

  // Battery 2 keeps last tick's value while battery 1 already has the new one
  datalayer.battery.status.voltage_dV = 4000;
  check_parallel_battery_safety(2);

  fake_triple_tick(4000);  // Mirror catches up on the next pass

  expect_no_difference_events("one tick of mirror lag");
}

/* The discriminating case. If an instance stops tracking battery 1 for several
   seconds the event does fire - and should. This is what a real sighting on a
   fake triple would mean: not that the check is wrong, but that instance 2 is
   not mirroring the way the fake driver intends. */
TEST_F(FakeTripleTest, InstanceThatStopsMirroringDoesRaiseTheEvent) {
  for (int second = 0; second < 5; ++second) {
    fake_triple_tick(3800);
  }
  ASSERT_FALSE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT2));

  // Battery 2 stops updating and is left behind as battery 1 moves away
  for (int second = 0; second < 5; ++second) {
    datalayer.battery.status.voltage_dV = 4000;
    check_parallel_battery_safety(2);
  }

  EXPECT_TRUE(voltage_difference_active(EVENT_VOLTAGE_DIFFERENCE_BAT2))
      << "an instance frozen 20 V away from the main pack must be reported";
}
