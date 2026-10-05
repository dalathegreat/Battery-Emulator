#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
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

// The unit test HAL is a LilyGo, which routes no ADC pin for a current sensor. The settings page
// leaves out the shunt types without a name.
TEST(QnhckBoardTest, NotOfferedWithoutAnAdcPin) {
  EXPECT_EQ(name_for_shunt_type(ShuntType::Qnhck2_16), nullptr);
}

// Only the board decides: saving another battery or inverter does not reset it
TEST(QnhckBoardTest, WorksWithAnyBatteryAndInverter) {
  EXPECT_TRUE(shunt_type_supported_by_battery(ShuntType::Qnhck2_16, BatteryType::NissanLeaf));
  EXPECT_TRUE(shunt_type_supported_by_battery(ShuntType::Qnhck2_16, BatteryType::None));
  EXPECT_TRUE(shunt_type_supported_by_inverter(ShuntType::Qnhck2_16, InverterProtocolType::BydModbus));
  EXPECT_TRUE(shunt_type_supported_by_inverter(ShuntType::Qnhck2_16, InverterProtocolType::None));
}

TEST(QnhckBoardTest, SetupWithoutAnAdcPinRaisesAnEventAndLeavesTheBatteriesInCharge) {
  Qnhck2_16Shunt sensor;
  sensor.setup();

  EXPECT_GE(get_event_pointer(EVENT_GPIO_NOT_DEFINED)->occurences, 1);
  EXPECT_FALSE(datalayer.shunt.replaces_battery_current);
  EXPECT_FALSE(datalayer.shunt.available);
  EXPECT_STREQ(datalayer.system.info.shunt_protocol, "QNHCK2-16 (50A ±0.625V)");

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

// --- Automatic calibration while the contactors are open ---

class QnhckAutoCalibrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer (contactors_engaged 0, open)
    qnhck_rated_current_A = 50;  // 12.5 mV per A
    qnhck_rated_output_mV = 625;
    qnhck_zero_mV = QNHCK_NOMINAL_ZERO_MV;
    qnhck_auto_calibration = true;
    contactor_control_enabled = true;
    contactor_control_enabled_double_battery = false;
    contactor_control_enabled_triple_battery = false;
  }

  void TearDown() override {
    qnhck_rated_current_A = QNHCK_DEFAULT_RATED_CURRENT_A;
    qnhck_rated_output_mV = QNHCK_DEFAULT_RATED_OUTPUT_MV;
    qnhck_zero_mV = QNHCK_NOMINAL_ZERO_MV;
    qnhck_auto_calibration = true;
    contactor_control_enabled = false;
    contactor_control_enabled_double_battery = false;
    contactor_control_enabled_triple_battery = false;
  }

  // One sample per millisecond from from_ms to to_ms, both included, all reading mV
  static void feed(Qnhck2_16Shunt& sensor, uint32_t from_ms, uint32_t to_ms, uint32_t mV) {
    for (uint32_t ms = from_ms; ms <= to_ms; ms++) {
      sensor.add_sample(ms, mV);
    }
  }

  static void close_contactors() { datalayer.system.status.contactors_engaged = 1; }
  static void open_contactors() { datalayer.system.status.contactors_engaged = 0; }
};

TEST_F(QnhckAutoCalibrationTest, MeasuresTheZeroPointWhileTheContactorsAreOpen) {
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);

  EXPECT_EQ(qnhck_zero_mV, 1640);
  EXPECT_TRUE(datalayer.shunt.available);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 0);

  // Closed, the zero point holds and 125 mV above it is 10 A
  close_contactors();
  feed(sensor, 2001, 3000, 1765);
  EXPECT_EQ(qnhck_zero_mV, 1640);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 10000);
  EXPECT_TRUE(datalayer.shunt.available);
}

TEST_F(QnhckAutoCalibrationTest, NotPassedOnBeforeTheZeroPointIsKnown) {
  contactor_control_enabled = false;  // Nothing says the contactors are open
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 3000, 1650);

  EXPECT_FALSE(datalayer.shunt.available);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 0);  // Still published, just not trusted
}

TEST_F(QnhckAutoCalibrationTest, ClosingContactorsBeforeTheyHaveSettledMeasureNothing) {
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 200, 1640);  // Open, but less than the settle time
  close_contactors();
  feed(sensor, 201, 3000, 1640);

  EXPECT_FALSE(datalayer.shunt.available);
  EXPECT_EQ(qnhck_zero_mV, QNHCK_NOMINAL_ZERO_MV);
}

TEST_F(QnhckAutoCalibrationTest, PrechargeIsNotOpen) {
  datalayer.system.status.contactors_engaged = 3;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 3000, 1640);

  EXPECT_FALSE(datalayer.shunt.available);
}

TEST_F(QnhckAutoCalibrationTest, AFaultLatchedOpenCounts) {
  datalayer.system.status.contactors_engaged = 2;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);

  EXPECT_EQ(qnhck_zero_mV, 1640);
  EXPECT_TRUE(datalayer.shunt.available);
}

TEST_F(QnhckAutoCalibrationTest, EveryOpeningMeasuresAfresh) {
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  close_contactors();
  feed(sensor, 2001, 3000, 1765);
  open_contactors();
  feed(sensor, 3001, 5000, 1660);

  EXPECT_EQ(qnhck_zero_mV, 1660);  // Not averaged with the previous opening
}

TEST_F(QnhckAutoCalibrationTest, ALongOpeningKeepsTheLatestTenSeconds) {
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 10000, 1600);
  feed(sensor, 10001, 20000, 1680);

  EXPECT_EQ(qnhck_zero_mV, 1680);
}

TEST_F(QnhckAutoCalibrationTest, AZeroPointThatCannotBeIsNotTaken) {
  // In range for the 150 A model, but no zero point: something flows, or it is on the wrong pin
  qnhck_rated_current_A = 150;
  qnhck_rated_output_mV = 1650;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 3000, 1900);

  EXPECT_EQ(qnhck_zero_mV, QNHCK_NOMINAL_ZERO_MV);
  EXPECT_FALSE(datalayer.shunt.available);
}

TEST_F(QnhckAutoCalibrationTest, APackWithoutItsOwnContactorControlMayStillBeOnTheLink) {
  battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  EXPECT_FALSE(datalayer.shunt.available);

  contactor_control_enabled_double_battery = true;  // Now its contactor is ours, and open
  feed(sensor, 2001, 4000, 1640);
  EXPECT_EQ(qnhck_zero_mV, 1640);
  EXPECT_TRUE(datalayer.shunt.available);
}

TEST_F(QnhckAutoCalibrationTest, ByHandTheStoredZeroPointIsUsedRightAway) {
  qnhck_auto_calibration = false;
  contactor_control_enabled = false;
  qnhck_zero_mV = 1640;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 1000, 1765);

  EXPECT_TRUE(datalayer.shunt.available);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 10000);
  EXPECT_EQ(qnhck_zero_mV, 1640);
}

// --- The zero point following the batteries' temperature ---

TEST(QnhckTemperatureTest, DriftIsTheCoefficientTimesTheChange) {
  EXPECT_EQ(qnhck_zero_drift_uV(1000, 200, 300), 10000);   // 1 mV/°C, 20.0 -> 30.0 °C
  EXPECT_EQ(qnhck_zero_drift_uV(1000, 300, 200), -10000);  // and back
  EXPECT_EQ(qnhck_zero_drift_uV(-500, 200, 300), -5000);   // A sensor drifting the other way
  EXPECT_EQ(qnhck_zero_drift_uV(300, -150, -145), 150);    // 0.5 °C below zero
  EXPECT_EQ(qnhck_zero_drift_uV(0, 200, 850), 0);
}

class QnhckTemperatureCompensationTest : public QnhckAutoCalibrationTest {
 protected:
  void SetUp() override {
    QnhckAutoCalibrationTest::SetUp();
    battery_detected = true;  // The datalayer reset has left pack 1 alive
    set_temperature(200);
  }

  void TearDown() override {
    qnhck_zero_tempco_uV_per_C = 0;
    battery_detected = false;
    QnhckAutoCalibrationTest::TearDown();
  }

  // The batteries' coldest and warmest reading, 2 °C either side of temperature_dC
  static void set_temperature(int16_t temperature_dC) {
    datalayer.aggregate.temperature_min_dC = temperature_dC - 20;
    datalayer.aggregate.temperature_max_dC = temperature_dC + 20;
  }
};

TEST_F(QnhckTemperatureCompensationTest, TakesTheTemperatureMidwayBetweenTheBatteriesColdestAndWarmest) {
  int16_t temperature_dC = 0;
  EXPECT_TRUE(qnhck_battery_temperature_dC(temperature_dC));
  EXPECT_EQ(temperature_dC, 200);

  set_temperature(-250);  // The sensor's operating range ends here
  EXPECT_TRUE(qnhck_battery_temperature_dC(temperature_dC));
  EXPECT_EQ(temperature_dC, -250);
  set_temperature(-251);
  EXPECT_FALSE(qnhck_battery_temperature_dC(temperature_dC));
  set_temperature(851);
  EXPECT_FALSE(qnhck_battery_temperature_dC(temperature_dC));
}

TEST_F(QnhckTemperatureCompensationTest, NoTemperatureUntilTheBatteryIsHeardFrom) {
  int16_t temperature_dC = 0;
  battery_detected = false;
  EXPECT_FALSE(qnhck_battery_temperature_dC(temperature_dC));

  battery_detected = true;
  datalayer.battery.status.CAN_battery_still_alive = 0;  // Detected once, silent since
  EXPECT_FALSE(qnhck_battery_temperature_dC(temperature_dC));
}

TEST_F(QnhckTemperatureCompensationTest, TheZeroPointFollowsTheTemperatureUntilTheNextOpening) {
  qnhck_zero_tempco_uV_per_C = 1000;  // 1 mV/°C: 10 °C warmer moves 1.640 V to 1.650 V
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);  // Measured at 20.0 °C
  close_contactors();
  set_temperature(300);
  feed(sensor, 2001, 3000, 1650);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 0);  // Not 0.8 A
  EXPECT_EQ(qnhck_zero_mV, 1640);                      // The measured zero point itself stays

  // 125 mV above where the zero point has moved to is still 10 A
  feed(sensor, 3001, 4000, 1775);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 10000);

  // The next opening measures afresh at the temperature it ends at, and starts over from there
  open_contactors();
  feed(sensor, 4001, 6000, 1650);
  EXPECT_EQ(qnhck_zero_mV, 1650);
  close_contactors();
  feed(sensor, 6001, 7000, 1650);
  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 0);
}

TEST_F(QnhckTemperatureCompensationTest, ASensorDriftingTheOtherWay) {
  qnhck_zero_tempco_uV_per_C = -500;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  close_contactors();
  set_temperature(300);
  feed(sensor, 2001, 3000, 1635);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 0);
}

TEST_F(QnhckTemperatureCompensationTest, OffByDefault) {
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  close_contactors();
  set_temperature(300);
  feed(sensor, 2001, 3000, 1650);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 800);
}

TEST_F(QnhckTemperatureCompensationTest, NotWithoutATemperatureAsTheZeroPointWasMeasured) {
  qnhck_zero_tempco_uV_per_C = 1000;
  battery_detected = false;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  close_contactors();
  battery_detected = true;  // Heard from only once the contactors have closed
  set_temperature(300);
  feed(sensor, 2001, 3000, 1650);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 800);
}

TEST_F(QnhckTemperatureCompensationTest, NotWhileTheBatteriesGiveNoTemperature) {
  qnhck_zero_tempco_uV_per_C = 1000;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 2000, 1640);
  close_contactors();
  datalayer.battery.status.CAN_battery_still_alive = 0;
  set_temperature(300);
  feed(sensor, 2001, 3000, 1650);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 800);  // As measured, rather than a stale guess
}

TEST_F(QnhckTemperatureCompensationTest, NotForAZeroPointSetByHand) {
  qnhck_zero_tempco_uV_per_C = 1000;
  qnhck_auto_calibration = false;
  contactor_control_enabled = false;
  qnhck_zero_mV = 1640;
  Qnhck2_16Shunt sensor;
  feed(sensor, 1, 1000, 1640);
  set_temperature(300);
  feed(sensor, 1001, 2000, 1650);

  EXPECT_EQ(datalayer.shunt.measured_amperage_mA, 800);
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

// --- Everything else that reads a battery's current: MQTT, ESP-NOW, the web pages, the display ---

TEST_F(QnhckAggregateTest, ASinglePackShowsTheMeasuredCurrent) {
  datalayer.battery.status.current_dA = -120;
  datalayer.battery.status.active_power_W = -4440;
  shunt_reads(-15049);

  EXPECT_EQ(pack_current_dA(datalayer.battery.status), -150);
  EXPECT_EQ(pack_power_W(datalayer.battery.status), -5550);  // 370.0 V x -15.0 A
  EXPECT_EQ(installation_current_dA(), -150);
  // The datalayer keeps what the battery reported
  EXPECT_EQ(datalayer.battery.status.current_dA, -120);
}

TEST_F(QnhckAggregateTest, ASinglePackShowsItsOwnCurrentUntilTheShuntHasAReading) {
  datalayer.battery.status.current_dA = -120;
  datalayer.battery.status.active_power_W = -4440;
  shunt_reads(-15049);
  datalayer.shunt.available = false;

  EXPECT_EQ(pack_current_dA(datalayer.battery.status), -120);
  EXPECT_EQ(pack_power_W(datalayer.battery.status), -4440);
  EXPECT_EQ(installation_current_dA(), -120);
}

TEST_F(QnhckAggregateTest, SeveralPacksKeepTheirOwnAndTheShuntStandsInForTheirSum) {
  battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
  datalayer.system.info.configured_batteries = 2;
  datalayer.battery.status.current_dA = 30;
  datalayer.battery.status.active_power_W = 1110;
  datalayer.battery2.status.current_dA = 40;
  shunt_reads(7500);

  // One sensor cannot tell the packs apart
  EXPECT_EQ(pack_current_dA(datalayer.battery.status), 30);
  EXPECT_EQ(pack_power_W(datalayer.battery.status), 1110);
  EXPECT_EQ(pack_current_dA(datalayer.battery2.status), 40);
  EXPECT_EQ(installation_current_dA(), 75);

  datalayer.shunt.available = false;
  EXPECT_EQ(installation_current_dA(), 70);
}

TEST_F(QnhckAggregateTest, APauseCompletesOnTheMeasuredCurrent) {
  datalayer.battery.status.current_dA = 50;  // The battery's own sensor still sees 5 A
  shunt_reads(1000);                         // while 1 A flows

  setBatteryPause(true, false);
  update_pause_state();
  EXPECT_EQ(emulator_pause_status, PAUSED);

  setBatteryPause(false, false);
  update_pause_state();
  EXPECT_EQ(emulator_pause_status, NORMAL);
}

}  // namespace
