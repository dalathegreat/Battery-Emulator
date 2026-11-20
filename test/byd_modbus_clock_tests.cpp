#include <gtest/gtest.h>

#include <Arduino.h>  // set_millis64(), from test/emul

#include "../Software/src/devboard/utils/system_clock.h"
#include "../Software/src/inverter/BYD-MODBUS.h"

/* The inverter writes its clock into ControlData (UTC, 403-406) once an hour. Each new time sets the
   ESP32's clock, which runs on by itself in between. */
namespace {

class BydModbusClockProbe : public BydModbusInverter {
 public:
  using BydModbusInverter::mbPV;

  // UTC as the inverter writes it: Unix seconds in a uint64, high word first
  void write_utc(uint32_t epoch_s) {
    mbPV[403] = 0;
    mbPV[404] = 0;
    mbPV[405] = epoch_s >> 16;
    mbPV[406] = epoch_s & 0xFFFF;
  }
};

class BydModbusClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(1000);
    clock_valid = false;
  }

  void TearDown() override {
    clock_valid = false;
    set_millis64(0);
  }

  BydModbusClockProbe inverter;
};

}  // namespace

TEST_F(BydModbusClockTest, TheInvertersTimeSetsTheClock) {
  inverter.update_values();
  EXPECT_FALSE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 0u);

  inverter.write_utc(1794299900);
  inverter.update_values();
  EXPECT_TRUE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 1794299900u);

  // The registers still hold the time sent 90 s ago. It must not set the clock back
  set_millis64(1000 + 90000);
  inverter.update_values();
  EXPECT_EQ(clock_utc_s(), 1794299990u);

  // An hour later the inverter sends its time again, and the clock follows it
  inverter.write_utc(1794303502);
  inverter.update_values();
  EXPECT_EQ(clock_utc_s(), 1794303502u);
}

// An inverter clock that was never set does not make ours valid
TEST_F(BydModbusClockTest, ATimeBefore2025DoesNotSetTheClock) {
  inverter.write_utc(86400);  // 1970-01-02
  inverter.update_values();

  EXPECT_FALSE(clock_valid);
  EXPECT_EQ(clock_utc_s(), 0u);
}
