#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/parallel_safety.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

class VoltageSyncTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    // The warning counters in check_parallel_battery_safety() are function-local statics, so a
    // preceding test can leave them counting. They are not reachable from a fixture; one matching
    // pass through the public API is the reset. Any decoded, i.e. non-zero, voltage gets there.
    battery2_detected = true;
    battery3_detected = true;
    datalayer.battery.status.voltage_dV = 3750;
    datalayer.battery2.status.voltage_dV = 3750;
    datalayer.battery3.status.voltage_dV = 3750;
    check_parallel_battery_safety(2);
    check_parallel_battery_safety(3);
    // Reset datalayer to known state: no pack has decoded a voltage yet
    datalayer.battery.status.voltage_dV = 0;
    datalayer.battery2.status.voltage_dV = 0;
    datalayer.battery3.status.voltage_dV = 0;
    init_events();
  }

  static bool matches(uint8_t n) {
    return n == 2 ? datalayer.system.status.battery2_voltage_matches : datalayer.system.status.battery3_voltage_matches;
  }
  static bool warned(EVENTS_ENUM_TYPE event) { return get_event_pointer(event)->state == EVENT_STATE_ACTIVE; }
};

TEST_F(VoltageSyncTest, MatchesWithin1V5) {
  datalayer.battery.status.voltage_dV = 3715;
  datalayer.battery2.status.voltage_dV = 3700;  // 1.5V apart
  check_parallel_battery_safety(2);
  EXPECT_TRUE(matches(2));

  datalayer.battery2.status.voltage_dV = 3699;  // 1.6V apart
  check_parallel_battery_safety(2);
  EXPECT_FALSE(matches(2));
}

// The flag is the latest reading alone: no memory in either direction
TEST_F(VoltageSyncTest, FollowsTheLatestReading) {
  datalayer.battery.status.voltage_dV = 3710;
  datalayer.battery3.status.voltage_dV = 3500;
  check_parallel_battery_safety(3);
  EXPECT_FALSE(matches(3));

  datalayer.battery3.status.voltage_dV = 3710;
  check_parallel_battery_safety(3);
  EXPECT_TRUE(matches(3));

  datalayer.battery3.status.voltage_dV = 3500;
  check_parallel_battery_safety(3);
  EXPECT_FALSE(matches(3));
}

// Every pack reads 0 until its integration has decoded a voltage: with nothing to compare there is
// no match, whatever matched before
TEST_F(VoltageSyncTest, NoReadingNoMatch) {
  check_parallel_battery_safety(2);
  EXPECT_FALSE(matches(2));

  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3700;
  check_parallel_battery_safety(2);
  ASSERT_TRUE(matches(2));

  datalayer.battery.status.voltage_dV = 0;  // Battery 1 not decoded
  check_parallel_battery_safety(2);
  EXPECT_FALSE(matches(2));
}

// A pack not seen on CAN never matches, even with a plausible voltage in the datalayer
TEST_F(VoltageSyncTest, UndetectedPackNeverMatches) {
  battery2_detected = false;
  datalayer.battery.status.voltage_dV = 3750;
  datalayer.battery2.status.voltage_dV = 3750;
  check_parallel_battery_safety(2);
  EXPECT_FALSE(matches(2));
}

// The warning is raised once the voltages have been apart for more than 3 seconds, and cleared
// as soon as they match again
TEST_F(VoltageSyncTest, WarnsAfter3SecondsApart) {
  datalayer.battery.status.voltage_dV = 3710;
  datalayer.battery2.status.voltage_dV = 3500;
  for (int second = 1; second <= 3; second++) {
    check_parallel_battery_safety(2);
    EXPECT_FALSE(warned(EVENT_VOLTAGE_DIFFERENCE_BAT2)) << "warned at second " << second;
  }
  check_parallel_battery_safety(2);
  EXPECT_TRUE(warned(EVENT_VOLTAGE_DIFFERENCE_BAT2));
  EXPECT_FALSE(warned(EVENT_VOLTAGE_DIFFERENCE_BAT3));

  datalayer.battery2.status.voltage_dV = 3710;
  check_parallel_battery_safety(2);
  EXPECT_FALSE(warned(EVENT_VOLTAGE_DIFFERENCE_BAT2));
}

// Long mismatches keep the warning up rather than wrapping the counter round
TEST_F(VoltageSyncTest, WarningStaysUpThroughALongMismatch) {
  datalayer.battery.status.voltage_dV = 3710;
  datalayer.battery3.status.voltage_dV = 3500;
  for (int second = 0; second < 600; second++) {
    check_parallel_battery_safety(3);
  }
  EXPECT_TRUE(warned(EVENT_VOLTAGE_DIFFERENCE_BAT3));
  EXPECT_FALSE(matches(3));
}
