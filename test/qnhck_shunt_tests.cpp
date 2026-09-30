#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/shunt/QNHCK2-16.h"
#include "../Software/src/shunt/Shunt.h"

/* The QNHCK2-16 Hall sensor: turning its output voltage into a current, deciding whether a
   reading can be its zero point, and the aggregate handing its current to the inverter in place
   of what the packs report. */

namespace {

// --- Output voltage to current ---

TEST(QnhckCurrentTest, ZeroPointIsZeroAmps) {
  EXPECT_EQ(qnhck_current_mA(1650000, 1650, 50, 625), 0);
  // A calibrated zero point replaces the nominal one
  EXPECT_EQ(qnhck_current_mA(1637000, 1637, 50, 625), 0);
}

TEST(QnhckCurrentTest, RatedOutputIsRatedCurrent) {
  EXPECT_EQ(qnhck_current_mA(2275000, 1650, 50, 625), 50000);
  EXPECT_EQ(qnhck_current_mA(1025000, 1650, 50, 625), -50000);
  EXPECT_EQ(qnhck_current_mA(3300000, 1650, 150, 1650), 150000);
  EXPECT_EQ(qnhck_current_mA(150000, 1650, 100, 1500), -100000);
  // Half the swing is half the current
  EXPECT_EQ(qnhck_current_mA(1962500, 1650, 50, 625), 25000);
}

TEST(QnhckCurrentTest, OffsetFromCalibratedZeroPoint) {
  // 12.5 mV/A: 125 mV above a zero point of 1.640 V is 10 A
  EXPECT_EQ(qnhck_current_mA(1765000, 1640, 50, 625), 10000);
}

TEST(QnhckCurrentTest, RoundsAlikeInBothDirections) {
  // 10 A / 0.625 V is 62.5 mV/A: 1 mV is 16 mA exactly, 0.1 mV is 1.6 mA
  EXPECT_EQ(qnhck_current_mA(1651000, 1650, 10, 625), 16);
  EXPECT_EQ(qnhck_current_mA(1649000, 1650, 10, 625), -16);
  EXPECT_EQ(qnhck_current_mA(1650100, 1650, 10, 625), 2);
  EXPECT_EQ(qnhck_current_mA(1649900, 1650, 10, 625), -2);
}

TEST(QnhckCurrentTest, NoRatedOutputGivesNoCurrent) {
  EXPECT_EQ(qnhck_current_mA(2275000, 1650, 50, 0), 0);
}

TEST(QnhckCurrentTest, RangeIsWhatAWorkingSensorCanRead) {
  EXPECT_TRUE(qnhck_current_in_range(0, 50));
  EXPECT_TRUE(qnhck_current_in_range(55000, 50));  // 1.1 x Ipn, the measuring range
  EXPECT_TRUE(qnhck_current_in_range(-60000, 50));
  EXPECT_FALSE(qnhck_current_in_range(60001, 50));
  EXPECT_FALSE(qnhck_current_in_range(-132000, 50));  // 0 V on the ±0.625 V model
  EXPECT_TRUE(qnhck_current_in_range(180000, 150));
  EXPECT_FALSE(qnhck_current_in_range(180001, 150));
}

// --- Zero point and model checks ---

TEST(QnhckZeroTest, AcceptsWhatASensorCanRead) {
  EXPECT_TRUE(qnhck_zero_plausible(1650));
  EXPECT_TRUE(qnhck_zero_plausible(1450));
  EXPECT_TRUE(qnhck_zero_plausible(1850));
}

TEST(QnhckZeroTest, RejectsWhatNoWorkingSensorReads) {
  EXPECT_FALSE(qnhck_zero_plausible(0));     // Unpowered, or no reading yet
  EXPECT_FALSE(qnhck_zero_plausible(1449));  // Beyond what offset, drift and ADC error explain
  EXPECT_FALSE(qnhck_zero_plausible(1851));
  EXPECT_FALSE(qnhck_zero_plausible(2500));  // A 5 V version at 0 A
}

TEST(QnhckZeroTest, OnlyTheModelsThatExist) {
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_CURRENTS, 10));
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_CURRENTS, 150));
  EXPECT_FALSE(qnhck_is_model(QNHCK_RATED_CURRENTS, 40));
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_OUTPUTS, 625));
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_OUTPUTS, 1650));
  EXPECT_FALSE(qnhck_is_model(QNHCK_RATED_OUTPUTS, 0));
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_CURRENTS, QNHCK_DEFAULT_RATED_CURRENT_A));
  EXPECT_TRUE(qnhck_is_model(QNHCK_RATED_OUTPUTS, QNHCK_DEFAULT_RATED_OUTPUT_MV));
}

// --- Boards without an ADC pin for it ---

// The unit test HAL is a LilyGo, which routes no ADC pin for a current sensor
TEST(QnhckBoardTest, NotOfferedWithoutAnAdcPin) {
  EXPECT_EQ(name_for_shunt_type(ShuntType::Qnhck2_16), nullptr);
  for (auto type : supported_shunt_types()) {
    EXPECT_NE(type, ShuntType::Qnhck2_16);
  }
}

TEST(QnhckBoardTest, SetupWithoutAnAdcPinRaisesAnEventAndLeavesTheBatteriesInCharge) {
  Qnhck2_16Shunt sensor;
  sensor.setup();

  EXPECT_GE(get_event_pointer(EVENT_GPIO_NOT_DEFINED)->occurences, 1);
  EXPECT_FALSE(datalayer.shunt.replaces_battery_current);
  EXPECT_FALSE(datalayer.shunt.available);
  EXPECT_STREQ(datalayer.system.info.shunt_protocol, "QNHCK2-16 50 A, ±0.625 V");

  uint16_t reading_mV = 1234;
  EXPECT_FALSE(sensor.calibrate_zero(reading_mV));
  EXPECT_EQ(reading_mV, 0);
  EXPECT_EQ(qnhck_zero_mV, QNHCK_NOMINAL_ZERO_MV);
}

// The host ADC reads 0 V, which is what an unpowered or disconnected sensor gives: a current far
// beyond its range, which must not reach the inverter.
TEST(QnhckBoardTest, AnUnpoweredSensorIsNotPassedOn) {
  Qnhck2_16Shunt sensor;
  for (uint32_t ms = 1; ms <= INTERVAL_1_S; ms++) {
    sensor.transmit(ms);
  }

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, -132000);  // -2.64 x 50 A
  EXPECT_FALSE(datalayer.shunt.available);

  uint16_t reading_mV = 1234;
  EXPECT_FALSE(sensor.calibrate_zero(reading_mV));
  EXPECT_EQ(reading_mV, 0);
}

// --- The inverter gets the measured current ---

class QnhckAggregateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and deleted the packs.
    datalayer.battery_settings.soc_scaling_active = false;
    datalayer.system.info.configured_batteries = 1;
    battery2_detected = false;
    battery3_detected = false;
    datalayer.battery.status.voltage_dV = 3700;
  }

  static void shunt_reads(int32_t mA) {
    datalayer.shunt.replaces_battery_current = true;
    datalayer.shunt.available = true;
    datalayer.shunt.measured_amperage_mA = mA;
  }
};

TEST_F(QnhckAggregateTest, MeasuredCurrentReplacesWhatTheBatteryReports) {
  datalayer.battery.status.current_dA = -120;
  datalayer.battery.status.reported_current_dA = -120;
  shunt_reads(-15049);

  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.current_dA, -150);
  EXPECT_EQ(datalayer.aggregate.active_power_W, -5550);  // 370.0 V x -15.0 A
  // The pack keeps its own reading
  EXPECT_EQ(datalayer.battery.status.current_dA, -120);
  EXPECT_EQ(datalayer.battery.status.reported_current_dA, -120);
}

TEST_F(QnhckAggregateTest, MeasuredCurrentReplacesTheSumOfSeveralPacks) {
  battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
  battery3 = new TestFakeBattery(&datalayer.battery3, CAN_Interface::CAN_NATIVE);
  datalayer.system.info.configured_batteries = 3;
  datalayer.battery.status.reported_current_dA = 10 + 20 + 30;
  shunt_reads(7000);

  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.current_dA, 70);
  EXPECT_EQ(datalayer.aggregate.active_power_W, 2590);
}

TEST_F(QnhckAggregateTest, BatteriesStayInChargeUntilTheShuntHasAReading) {
  datalayer.battery.status.reported_current_dA = -120;
  shunt_reads(-15049);
  datalayer.shunt.available = false;

  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.current_dA, -120);
}

// Shunts that only publish their readings, like the inverter provided one, change nothing
TEST_F(QnhckAggregateTest, OnlyAShuntThatAsksForItReplacesTheBatteries) {
  datalayer.battery.status.reported_current_dA = -120;
  shunt_reads(-15049);
  datalayer.shunt.replaces_battery_current = false;

  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.current_dA, -120);
}

TEST_F(QnhckAggregateTest, RoundsToTheNearestDeciAmpereAndStaysInRange) {
  shunt_reads(12349);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.current_dA, 123);

  shunt_reads(12350);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.current_dA, 124);

  shunt_reads(-12350);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.current_dA, -124);

  shunt_reads(4000000);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.current_dA, INT16_MAX);

  shunt_reads(-4000000);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.current_dA, INT16_MIN);
}

}  // namespace
