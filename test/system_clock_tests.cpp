#include <gtest/gtest.h>

#include <Arduino.h>  // set_millis64(), from test/emul

#include "../Software/src/devboard/utils/system_clock.h"

/* A restart of the firmware keeps the time the inverter last sent, worked out again from the RTC
   counter, which runs on through the restart. Here a restart is clock_valid going back to false
   while the emulated time, which the RTC counter is emulated on, runs on. A power cycle also takes
   the emulated time back towards 0. */
namespace {

class SystemClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(10000);
    clock_valid = false;
  }

  void TearDown() override {
    clock_valid = false;
    set_millis64(0);
  }

  void restart() {
    clock_valid = false;
    clock_restore();
  }
};

constexpr uint64_t HOUR_MS = 3600 * 1000;

}  // namespace

TEST_F(SystemClockTest, ARestartKeepsTheClock) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(10000 + HOUR_MS / 2);
  restart();

  EXPECT_TRUE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 1794299900u + 1800);
}

// Counted from when the inverter last sent its time, not from the last restart, so restarting
// again and again does not stretch how long the RTC counts alone
TEST_F(SystemClockTest, KeptOnlyUpToTwoHoursAfterTheInvertersTime) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(10000 + HOUR_MS);
  restart();
  EXPECT_TRUE(clock_valid);

  set_millis64(10000 + 2 * HOUR_MS + 1000);
  restart();
  EXPECT_FALSE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 0u);
}

TEST_F(SystemClockTest, EachTimeFromTheInverterStartsTheCountAgain) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(10000 + 3 * HOUR_MS);
  ASSERT_TRUE(clock_set_utc(1794310702));
  set_millis64(10000 + 3 * HOUR_MS + 60000);
  restart();

  EXPECT_TRUE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 1794310702u + 60);
}

TEST_F(SystemClockTest, APowerCycleDoesNotKeepIt) {
  set_millis64(10 * 60 * 1000);
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(2000);  // The RTC counter started again from 0
  restart();

  EXPECT_FALSE(clock_valid);
}
