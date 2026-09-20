#include <Preferences.h>
#include <gtest/gtest.h>
#include <limits>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/BYD-ATTO-3-BATTERY.h"
#include "../Software/src/battery/KIA-HYUNDAI-64-BATTERY.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/energy_counter.h"

namespace {
constexpr uint32_t DAY_MS = 86400000;
constexpr const char* KEYS[] = {"ENERGY_CHG_WH", "ENERGY_DIS_WH", "CAP_CHG_DAH", "CAP_DIS_DAH"};

class CounterBattery : public TestFakeBattery {
 public:
  CounterBattery(bool wh, bool ah) : native_wh(wh), native_ah(ah) {}
  bool supports_charged_energy() override { return native_wh; }
  bool supports_directional_capacity() override { return native_ah; }

 private:
  bool native_wh;
  bool native_ah;
};

class EnergyCounterTest : public testing::Test {
 protected:
  uint32_t now = 0;
  DATALAYER_BATTERY_STATUS_TYPE& status = datalayer.battery.status;

  void SetUp() override {
    Preferences::reset();

    Preferences prefs;
    ASSERT_TRUE(prefs.begin("batterySettings"));
    prefs.putBool("ENERGYPERSIST", true);
    prefs.end();

    select(false, false);
    ready();
  }
  void TearDown() override {
    Preferences::reset();
    set_millis64(0);
  }
  void select(bool native_wh, bool native_ah) {
    delete battery;
    battery = new CounterBattery(native_wh, native_ah);
    init_energy_counters(now);
  }
  void ready() {
    status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    status.real_bms_status = BMS_ACTIVE;
    status.voltage_dV = 100;  // 10 V: 1 Wh and 0.1 Ah complete together.
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.battery_allows_contactor_closing = true;
    datalayer.system.status.contactors_engaged = 1;
  }
  void step(int16_t current_dA, uint32_t elapsed_ms) {
    status.reported_current_dA = current_dA;
    now += elapsed_ms;
    update_energy_counters(now);
  }
  void expect(uint32_t charge, uint32_t discharge) {
    EXPECT_EQ(status.total_charged_battery_Wh, charge);
    EXPECT_EQ(status.total_discharged_battery_Wh, discharge);
    EXPECT_EQ(status.total_charged_battery_dAh, charge);
    EXPECT_EQ(status.total_discharged_battery_dAh, discharge);
  }
  void seed(int32_t charge = 11, int32_t discharge = 22, uint32_t charge_ah = 33, uint32_t discharge_ah = 44) {
    Preferences prefs;
    ASSERT_TRUE(prefs.begin("batterySettings"));
    prefs.putInt(KEYS[0], charge);
    prefs.putInt(KEYS[1], discharge);
    prefs.putUInt(KEYS[2], charge_ah);
    prefs.putUInt(KEYS[3], discharge_ah);
    prefs.end();
  }
  unsigned writes(unsigned index) { return Preferences::writeCount("batterySettings", KEYS[index]); }
};

TEST_F(EnergyCounterTest, DisabledPersistenceCountsInRamWithoutTouchingSavedTotals) {
  seed();

  Preferences prefs;
  ASSERT_TRUE(prefs.begin("batterySettings"));
  prefs.putBool("ENERGYPERSIST", false);
  prefs.end();

  const unsigned saved_writes[4] = {writes(0), writes(1), writes(2), writes(3)};

  select(false, false);
  expect(0, 0);

  step(100, 36000);
  expect(1, 0);

  store_energy_counters(DAY_MS);

  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), saved_writes[i]);
  }

  Preferences saved;
  ASSERT_TRUE(saved.begin("batterySettings"));
  EXPECT_EQ(saved.getInt(KEYS[0], 0), 11);
  EXPECT_EQ(saved.getInt(KEYS[1], 0), 22);
  EXPECT_EQ(saved.getUInt(KEYS[2], 0), 33u);
  EXPECT_EQ(saved.getUInt(KEYS[3], 0), 44u);
  saved.end();
}

TEST_F(EnergyCounterTest, EnablingPersistenceRequiresRestartBeforeSaving) {
  seed();

  Preferences prefs;
  ASSERT_TRUE(prefs.begin("batterySettings"));
  prefs.putBool("ENERGYPERSIST", false);
  prefs.end();

  select(false, false);
  expect(0, 0);

  step(100, 36000);
  expect(1, 0);

  ASSERT_TRUE(prefs.begin("batterySettings"));
  prefs.putBool("ENERGYPERSIST", true);
  prefs.end();

  store_energy_counters(DAY_MS);

  Preferences saved;
  ASSERT_TRUE(saved.begin("batterySettings"));
  EXPECT_EQ(saved.getInt(KEYS[0], 0), 11);
  EXPECT_EQ(saved.getInt(KEYS[1], 0), 22);
  EXPECT_EQ(saved.getUInt(KEYS[2], 0), 33u);
  EXPECT_EQ(saved.getUInt(KEYS[3], 0), 44u);
  saved.end();

  now = 0;
  select(false, false);
  EXPECT_EQ(status.total_charged_battery_Wh, 11);
  EXPECT_EQ(status.total_discharged_battery_Wh, 22);
  EXPECT_EQ(status.total_charged_battery_dAh, 33u);
  EXPECT_EQ(status.total_discharged_battery_dAh, 44u);
}

TEST_F(EnergyCounterTest, DirectionAndActualElapsedMilliseconds) {
  step(100, 12345);
  step(100, 23655);
  expect(1, 0);
  step(-200, 36000);
  expect(1, 2);
  step(0, 360000);
  expect(1, 2);
}

TEST_F(EnergyCounterTest, ChargedAndDischargedFractionsAreIndependent) {
  step(10, 180000);
  step(-10, 180000);
  expect(0, 0);
  step(10, 180000);
  expect(1, 0);
  step(-10, 180000);
  expect(1, 1);
}

TEST_F(EnergyCounterTest, CalculatedCountersIgnoreDeadbandAndCountFullCurrentAboveIt) {
  step(0, 720000);
  step(5, 720000);
  step(-5, 720000);
  expect(0, 0);

  step(6, 600000);
  expect(1, 0);
  step(-6, 600000);
  expect(1, 1);

  // LEAF current is reported in 5 dA steps, so +/-10 dA is its first
  // representable magnitude above the deadband.
  step(10, 360000);
  step(-10, 360000);
  expect(2, 2);
}

TEST_F(EnergyCounterTest, DeadbandPreservesChargedAndDischargedFractionalRemainders) {
  step(6, 300000);
  step(-6, 300000);
  expect(0, 0);

  step(5, 720000);
  step(-5, 720000);
  expect(0, 0);

  step(6, 300000);
  expect(1, 0);
  step(-6, 300000);
  expect(1, 1);
}

TEST_F(EnergyCounterTest, DeadbandLeavesNativeAndCalculatedCapabilitiesIndependent) {
  for (bool native_wh : {false, true}) {
    for (bool native_ah : {false, true}) {
      SCOPED_TRACE(testing::Message() << "native Wh=" << native_wh << " Ah=" << native_ah);
      Preferences::reset();
      status.total_charged_battery_Wh = native_wh ? 101 : 0;
      status.total_discharged_battery_Wh = native_wh ? 102 : 0;
      status.total_charged_battery_dAh = native_ah ? 103 : 0;
      status.total_discharged_battery_dAh = native_ah ? 104 : 0;
      select(native_wh, native_ah);
      ready();

      step(5, 720000);
      EXPECT_EQ(status.total_charged_battery_Wh, native_wh ? 101 : 0);
      EXPECT_EQ(status.total_charged_battery_dAh, native_ah ? 103 : 0);

      step(10, 360000);
      EXPECT_EQ(status.total_charged_battery_Wh, native_wh ? 101 : 1);
      EXPECT_EQ(status.total_charged_battery_dAh, native_ah ? 103 : 1);
    }
  }
}

TEST_F(EnergyCounterTest, WhUsesVoltageWithoutEarlyRoundingAndAhDoesNot) {
  status.voltage_dV = 4001;
  step(100, 3600000);
  EXPECT_EQ(status.total_charged_battery_Wh, 4001);
  EXPECT_EQ(status.total_charged_battery_dAh, 100);
  status.voltage_dV = 2000;
  step(-100, 3600000);
  EXPECT_EQ(status.total_discharged_battery_Wh, 2000);
  EXPECT_EQ(status.total_discharged_battery_dAh, 100);
}

TEST_F(EnergyCounterTest, TenMillisecondPathNeverOpensNvs) {
  const unsigned opens = Preferences::begin_calls;
  for (int i = 0; i < 3600; ++i) {
    step(100, 10);
  }
  expect(1, 0);
  EXPECT_EQ(Preferences::begin_calls, opens);
}

TEST_F(EnergyCounterTest, AllReadinessGatesExcludeInactiveTime) {
  for (unsigned gate = 0; gate < 6; ++gate) {
    SCOPED_TRACE(gate);
    ready();
    switch (gate) {
      case 0:
        status.CAN_battery_still_alive = 0;
        break;
      case 1:
        datalayer.system.status.system_status = FAULT;
        break;
      case 2:
        datalayer.system.status.battery_allows_contactor_closing = false;
        break;
      case 3:
        datalayer.system.status.contactors_engaged = 0;
        break;
      case 4:
        status.real_bms_status = BMS_FAULT;
        break;
      case 5:
        status.voltage_dV = 0;
        break;
    }
    step(100, 360000);
    step(-100, 360000);
    expect(0, 0);
  }
  ready();
  step(100, 36000);
  expect(1, 0);
}

TEST_F(EnergyCounterTest, MissingBatteryDoesNotAccumulateOrAccessNvs) {
  delete battery;
  battery = nullptr;
  unsigned opens = Preferences::begin_calls;
  init_energy_counters(now);
  step(100, DAY_MS);
  store_energy_counters(now);
  expect(0, 0);
  EXPECT_EQ(Preferences::begin_calls, opens);
}

TEST_F(EnergyCounterTest, RestoreOnlyCalculatedTotalsForEveryCapabilityCombination) {
  for (bool wh : {false, true}) {
    for (bool ah : {false, true}) {
      SCOPED_TRACE(testing::Message() << "native Wh=" << wh << " Ah=" << ah);
      Preferences::reset();
      seed();
      status.total_charged_battery_Wh = 101;
      status.total_discharged_battery_Wh = 102;
      status.total_charged_battery_dAh = 103;
      status.total_discharged_battery_dAh = 104;
      const unsigned opens = Preferences::begin_calls;
      select(wh, ah);
      EXPECT_EQ(status.total_charged_battery_Wh, wh ? 101 : 11);
      EXPECT_EQ(status.total_discharged_battery_Wh, wh ? 102 : 22);
      EXPECT_EQ(status.total_charged_battery_dAh, ah ? 103 : 33);
      EXPECT_EQ(status.total_discharged_battery_dAh, ah ? 104 : 44);
      step(100, 36000);
      step(-100, 36000);
      EXPECT_EQ(status.total_charged_battery_Wh, wh ? 101 : 12);
      EXPECT_EQ(status.total_discharged_battery_Wh, wh ? 102 : 23);
      EXPECT_EQ(status.total_charged_battery_dAh, ah ? 103 : 34);
      EXPECT_EQ(status.total_discharged_battery_dAh, ah ? 104 : 45);
      now += DAY_MS;
      store_energy_counters(now);
      EXPECT_EQ(writes(0), wh ? 1 : 2);
      EXPECT_EQ(writes(1), wh ? 1 : 2);
      EXPECT_EQ(writes(2), ah ? 1 : 2);
      EXPECT_EQ(writes(3), ah ? 1 : 2);
      if (wh && ah) {
        EXPECT_EQ(Preferences::begin_calls, opens);
      }
    }
  }
}

TEST_F(EnergyCounterTest, DailyCheckpointOnlyWritesChangedValues) {
  step(100, 36000);
  step(-200, 36000);
  store_energy_counters(DAY_MS - 1);
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_EQ(writes(i), 0);
  store_energy_counters(DAY_MS);
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_EQ(writes(i), 1);
  store_energy_counters(2 * DAY_MS);
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_EQ(writes(i), 1);
  step(100, 36000);
  store_energy_counters(3 * DAY_MS);
  EXPECT_EQ(writes(0), 2);
  EXPECT_EQ(writes(1), 1);
  EXPECT_EQ(writes(2), 2);
  EXPECT_EQ(writes(3), 1);
}

TEST_F(EnergyCounterTest, CheckpointTracksUptimeEvenWhileBatteryIsInactive) {
  datalayer.system.status.contactors_engaged = 0;
  step(100, DAY_MS);
  store_energy_counters(now);
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_EQ(writes(i), 1);
  expect(0, 0);
}

TEST_F(EnergyCounterTest, SimulatedRebootRestoresCheckpointAndResetsFractionsAndTimer) {
  step(100, 36000);
  step(-200, 36000);
  store_energy_counters(DAY_MS);
  step(100, 54000);  // 1.5 Wh/dAh since checkpoint; intentionally not saved.
  now = 0;
  init_energy_counters(now);
  expect(1, 2);
  step(100, 18000);
  expect(1, 2);  // No fractional carry from previous boot.
  store_energy_counters(DAY_MS - 1);
  EXPECT_EQ(writes(0), 1);
  step(100, 18000);
  expect(2, 2);
  store_energy_counters(DAY_MS);
  EXPECT_EQ(writes(0), 2);
  EXPECT_EQ(writes(2), 2);
}

TEST_F(EnergyCounterTest, AccumulationAndCheckpointCrossMillisWrap) {
  now = UINT32_MAX - 17999;
  init_energy_counters(now);
  const uint32_t start = now;
  step(100, 36000);
  expect(1, 0);
  step(-100, 36000);
  expect(1, 1);
  store_energy_counters(start + DAY_MS - 1);
  EXPECT_EQ(writes(0), 0);
  store_energy_counters(start + DAY_MS);
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_EQ(writes(i), 1);
}

TEST_F(EnergyCounterTest, SaturatesAllTotalsWithoutWrapping) {
  seed(INT32_MAX - 1, INT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1);
  init_energy_counters(now);
  step(100, 108000);
  step(-100, 108000);
  step(100, 108000);
  step(-100, 108000);
  EXPECT_EQ(status.total_charged_battery_Wh, INT32_MAX);
  EXPECT_EQ(status.total_discharged_battery_Wh, INT32_MAX);
  EXPECT_EQ(status.total_charged_battery_dAh, UINT32_MAX);
  EXPECT_EQ(status.total_discharged_battery_dAh, UINT32_MAX);
}

TEST_F(EnergyCounterTest, ExtremeInputProductAndMinimumSignedCurrentAreSafe) {
  status.voltage_dV = UINT16_MAX;
  step(INT16_MIN, UINT32_MAX);
  EXPECT_EQ(status.total_discharged_battery_Wh, INT32_MAX);
  const uint64_t expected = uint64_t(32768) * UINT32_MAX / 3600000;
  EXPECT_EQ(status.total_discharged_battery_dAh, expected);
  EXPECT_EQ(status.total_charged_battery_Wh, 0);
  EXPECT_EQ(status.total_charged_battery_dAh, 0);
}

TEST_F(EnergyCounterTest, NegativePersistedWhIsSanitized) {
  seed(-1, INT32_MIN);
  init_energy_counters(now);
  EXPECT_EQ(status.total_charged_battery_Wh, 0);
  EXPECT_EQ(status.total_discharged_battery_Wh, 0);
}

TEST_F(EnergyCounterTest, BydNativeWholeAhDecodingPromotesBeforeConversion) {
  delete battery;
  auto* byd = new BydAttoBattery();
  battery = byd;
  EXPECT_TRUE(byd->supports_directional_capacity());
  EXPECT_TRUE(byd->supports_charged_energy());
  CAN_frame frame = {};
  frame.ID = 0x7EF;
  frame.DLC = 8;
  frame.data.u8[0] = 0x05;
  frame.data.u8[1] = 0x62;
  frame.data.u8[3] = 0x0F;
  frame.data.u8[4] = 0xFF;
  frame.data.u8[5] = 0xFF;
  byd->handle_incoming_can_frame(frame);
  frame.data.u8[3] = 0x10;
  frame.data.u8[4] = 0x34;
  frame.data.u8[5] = 0x12;
  byd->handle_incoming_can_frame(frame);
  byd->update_values();
  EXPECT_EQ(status.total_charged_battery_dAh, 655350U);
  EXPECT_EQ(status.total_discharged_battery_dAh, 46600U);
}

class TestKia : public KiaHyundai64Battery {
 public:
  using KiaHyundai64Battery::handle_pid;
};

TEST_F(EnergyCounterTest, KiaNativeDiagnosticDAhMapsWithoutScalingOrSignedNarrowing) {
  delete battery;
  auto* kia = new TestKia();
  battery = kia;
  EXPECT_TRUE(kia->supports_directional_capacity());
  EXPECT_FALSE(kia->supports_charged_energy());
  uint8_t data[59] = {};
  data[30] = 0xFE;
  data[31] = 0xDC;
  data[32] = 0xBA;
  data[33] = 0x98;
  data[34] = 0x12;
  data[35] = 0x34;
  data[36] = 0x56;
  data[37] = 0x78;
  kia->handle_pid(0x0101, 0, data, sizeof(data));
  kia->update_values();
  EXPECT_EQ(status.total_charged_battery_dAh, 0xFEDCBA98U);
  EXPECT_EQ(status.total_discharged_battery_dAh, 0x12345678U);
}
}  // namespace
