#include <Preferences.h>
#include <gtest/gtest.h>
#include <limits>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/BYD-ATTO-3-BATTERY.h"
#include "../Software/src/battery/KIA-HYUNDAI-64-BATTERY.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/energy_counter.h"

namespace {
constexpr uint32_t DAY_MS = 86400000;
constexpr const char* KEYS[] = {"ENERGY_CHG_WH", "ENERGY_DIS_WH", "CAP_CHG_DAH", "CAP_DIS_DAH"};

class CounterBattery : public TestFakeBattery {
 public:
  CounterBattery(bool wh, bool ah, DATALAYER_BATTERY_TYPE* pack = &datalayer.battery)
      : TestFakeBattery(pack, CAN_Interface::CAN_NATIVE), native_wh(wh), native_ah(ah) {}
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
  bool previous_double_control;
  bool previous_triple_control;

  void SetUp() override {
    Preferences::reset();
    previous_double_control = contactor_control_enabled_double_battery;
    previous_triple_control = contactor_control_enabled_triple_battery;
    contactor_control_enabled_double_battery = false;
    contactor_control_enabled_triple_battery = false;
    persistence(true);
    select(false, false);
    ready();
  }
  void TearDown() override {
    Preferences::reset();
    contactor_control_enabled_double_battery = previous_double_control;
    contactor_control_enabled_triple_battery = previous_triple_control;
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
    status.current_dA = current_dA;
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
  void persistence(bool enabled) {
    Preferences prefs;
    ASSERT_TRUE(prefs.begin("batterySettings"));
    prefs.putBool("ENERGYPERSIST", enabled);
    prefs.end();
  }
  unsigned writes(unsigned index) { return Preferences::writeCount("batterySettings", KEYS[index]); }
};

TEST_F(EnergyCounterTest, MissingOrDisabledPersistenceKeepsAllPacksInRamAndPreservesSavedTotals) {
  battery2 = new CounterBattery(false, false, &datalayer.battery2);
  battery3 = new CounterBattery(false, false, &datalayer.battery3);
  datalayer.system.info.configured_batteries = 3;
  datalayer.system.status.battery2_allowed_contactor_closing = true;
  datalayer.system.status.battery3_allowed_contactor_closing = true;
  for (bool explicit_false : {false, true}) {
    SCOPED_TRACE(explicit_false);
    Preferences::reset();
    if (explicit_false) {
      persistence(false);
    }
    Preferences prefs;
    ASSERT_TRUE(prefs.begin("batterySettings"));
    for (unsigned i = 0; i < 3; ++i) {
      auto& pack = i == 0 ? status : (i == 1 ? datalayer.battery2.status : datalayer.battery3.status);
      pack.total_charged_battery_Wh = pack.total_discharged_battery_Wh = 0;
      pack.total_charged_battery_dAh = pack.total_discharged_battery_dAh = 0;
      pack.CAN_battery_still_alive = CAN_STILL_ALIVE;
      pack.real_bms_status = BMS_ACTIVE;
      pack.voltage_dV = 100;
      pack.current_dA = 100;
      for (unsigned key = 0; key < 4; ++key) {
        const std::string name = std::string(KEYS[key]) + (i == 0 ? "" : (i == 1 ? "_2" : "_3"));
        if (key < 2) {
          prefs.putInt(name.c_str(), 77);
        } else {
          prefs.putUInt(name.c_str(), 77);
        }
      }
    }
    prefs.end();
    now = 0;
    init_energy_counters(now);
    const unsigned opens = Preferences::begin_calls;
    step(100, 36000);
    store_energy_counters(DAY_MS);
    EXPECT_EQ(Preferences::begin_calls, opens);
    ASSERT_TRUE(prefs.begin("batterySettings", true));
    for (unsigned i = 0; i < 3; ++i) {
      const auto& pack = i == 0 ? status : (i == 1 ? datalayer.battery2.status : datalayer.battery3.status);
      EXPECT_EQ(pack.total_charged_battery_Wh, 1);
      EXPECT_EQ(pack.total_discharged_battery_Wh, 0);
      EXPECT_EQ(pack.total_charged_battery_dAh, 1U);
      EXPECT_EQ(pack.total_discharged_battery_dAh, 0U);
      for (unsigned key = 0; key < 4; ++key) {
        const std::string name = std::string(KEYS[key]) + (i == 0 ? "" : (i == 1 ? "_2" : "_3"));
        EXPECT_EQ(Preferences::writeCount("batterySettings", name.c_str()), 1U);
        EXPECT_EQ(key < 2 ? prefs.getInt(name.c_str(), 0) : prefs.getUInt(name.c_str(), 0), 77U);
      }
    }
    prefs.end();
  }
}

TEST_F(EnergyCounterTest, EnablingRequiresRestartThenAutomaticallyRestoresAndCheckpoints) {
  seed();
  persistence(false);
  init_energy_counters(0);
  expect(0, 0);
  step(100, 36000);
  expect(1, 0);
  persistence(true);
  store_energy_counters(DAY_MS);
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1U);
  }
  now = 0;
  init_energy_counters(now);
  EXPECT_EQ(status.total_charged_battery_Wh, 11);
  EXPECT_EQ(status.total_discharged_battery_Wh, 22);
  EXPECT_EQ(status.total_charged_battery_dAh, 33U);
  EXPECT_EQ(status.total_discharged_battery_dAh, 44U);
  step(100, 36000);
  store_energy_counters(DAY_MS - 1);
  EXPECT_EQ(writes(0), 1U);
  store_energy_counters(DAY_MS);
  EXPECT_EQ(writes(0), 2U);
  EXPECT_EQ(writes(2), 2U);
  init_energy_counters(0);
  EXPECT_EQ(status.total_charged_battery_Wh, 12);
  EXPECT_EQ(status.total_charged_battery_dAh, 34U);
}

TEST_F(EnergyCounterTest, DisablingRequiresRestartAndLeavesSavedTotalsIntact) {
  step(100, 36000);
  persistence(false);
  store_energy_counters(DAY_MS);  // This boot still uses its enabled startup setting.
  EXPECT_EQ(writes(0), 1U);
  status.total_charged_battery_Wh = status.total_discharged_battery_Wh = 0;
  status.total_charged_battery_dAh = status.total_discharged_battery_dAh = 0;
  now = 0;
  init_energy_counters(now);
  expect(0, 0);
  const unsigned opens = Preferences::begin_calls;
  step(-100, 36000);
  expect(0, 1);
  store_energy_counters(DAY_MS);
  EXPECT_EQ(Preferences::begin_calls, opens);
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1U);
  }
}

TEST_F(EnergyCounterTest, NativePrimaryDoesNotPreventCalculatedSecondaryRestoration) {
  select(true, true);
  battery2 = new CounterBattery(false, false, &datalayer.battery2);
  datalayer.system.info.configured_batteries = 2;
  Preferences prefs;
  ASSERT_TRUE(prefs.begin("batterySettings"));
  prefs.putInt("ENERGY_CHG_WH_2", 123);
  prefs.putUInt("CAP_DIS_DAH_2", 456);
  prefs.end();
  init_energy_counters(0);
  EXPECT_EQ(status.total_charged_battery_Wh, 0);
  EXPECT_EQ(datalayer.battery2.status.total_charged_battery_Wh, 123);
  EXPECT_EQ(datalayer.battery2.status.total_discharged_battery_dAh, 456U);
  store_energy_counters(DAY_MS);
  EXPECT_EQ(writes(0), 0U);
  EXPECT_EQ(writes(2), 0U);
  EXPECT_EQ(Preferences::writeCount("batterySettings", "ENERGY_CHG_WH_2"), 1U);
  EXPECT_EQ(Preferences::writeCount("batterySettings", "CAP_DIS_DAH_2"), 1U);
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

  // Also check +/-10 dA; upstream LEAF averaging can report finer values
  // than its raw 5 dA CAN quantisation.
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
      persistence(true);
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
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 0);
  }
  store_energy_counters(DAY_MS);
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1);
  }
  store_energy_counters(2 * DAY_MS);
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1);
  }
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
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1);
  }
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
  for (unsigned i = 0; i < 4; ++i) {
    EXPECT_EQ(writes(i), 1);
  }
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

TEST_F(EnergyCounterTest, RestoreUsesOriginalNamespaceAndNumericTypes) {
  seed(1234567, 2345678, 0xFEDCBA98U, 0x87654321U);
  Preferences other;
  ASSERT_TRUE(other.begin("otherNamespace"));
  other.putInt(KEYS[0], 999);
  other.end();
  init_energy_counters(now);
  EXPECT_EQ(status.total_charged_battery_Wh, 1234567);
  EXPECT_EQ(status.total_discharged_battery_Wh, 2345678);
  EXPECT_EQ(status.total_charged_battery_dAh, 0xFEDCBA98U);
  EXPECT_EQ(status.total_discharged_battery_dAh, 0x87654321U);
  store_energy_counters(DAY_MS);
  Preferences prefs;
  ASSERT_TRUE(prefs.begin("batterySettings", true));
  EXPECT_EQ(prefs.getInt(KEYS[0], -1), 1234567);
  EXPECT_EQ(prefs.getUInt(KEYS[0], 99), 99U);  // Signed key must remain signed.
  EXPECT_EQ(prefs.getUInt(KEYS[2], 0), 0xFEDCBA98U);
  EXPECT_EQ(prefs.getInt(KEYS[2], -1), -1);    // Unsigned key must remain unsigned.
}

TEST_F(EnergyCounterTest, SinglePackUsesReplacementShuntAndFallsBackWhenUnavailable) {
  datalayer.shunt.replaces_battery_current = true;
  datalayer.shunt.available = true;
  datalayer.shunt.measured_amperage_mA = -1000;  // -10 dA, despite pack current +100 dA.
  step(100, 360000);
  expect(0, 1);
  datalayer.shunt.available = false;
  step(100, 36000);
  expect(1, 1);
}

TEST_F(EnergyCounterTest, ThreePacksUseOwnCurrentsAndPersistToIndependentKeys) {
  battery2 = new CounterBattery(false, false, &datalayer.battery2);
  battery3 = new CounterBattery(false, false, &datalayer.battery3);
  datalayer.system.info.configured_batteries = 3;
  datalayer.system.status.battery2_allowed_contactor_closing = true;
  datalayer.system.status.battery3_allowed_contactor_closing = true;
  auto& second = datalayer.battery2.status;
  auto& third = datalayer.battery3.status;
  second.voltage_dV = third.voltage_dV = 100;
  second.current_dA = 200;
  third.current_dA = -300;
  second.real_bms_status = third.real_bms_status = BMS_ACTIVE;
  // A sum/shunt substituted into every pack would count the same current repeatedly.
  status.reported_current_dA = 1234;
  datalayer.shunt.replaces_battery_current = true;
  datalayer.shunt.available = true;
  datalayer.shunt.measured_amperage_mA = 99000;
  init_energy_counters(now);
  step(100, 36000);
  expect(1, 0);
  EXPECT_EQ(second.total_charged_battery_Wh, 2);
  EXPECT_EQ(second.total_charged_battery_dAh, 2U);
  EXPECT_EQ(third.total_discharged_battery_Wh, 3);
  EXPECT_EQ(third.total_discharged_battery_dAh, 3U);
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.total_charged_battery_Wh, 3);
  EXPECT_EQ(datalayer.aggregate.total_discharged_battery_Wh, 3);
  EXPECT_EQ(datalayer.aggregate.total_charged_battery_dAh, 3U);
  EXPECT_EQ(datalayer.aggregate.total_discharged_battery_dAh, 3U);
  store_energy_counters(DAY_MS);
  EXPECT_EQ(Preferences::writeCount("batterySettings", "ENERGY_CHG_WH_2"), 1U);
  EXPECT_EQ(Preferences::writeCount("batterySettings", "CAP_DIS_DAH_3"), 1U);
  status.total_charged_battery_Wh = second.total_charged_battery_Wh = third.total_discharged_battery_Wh = 0;
  second.total_charged_battery_dAh = third.total_discharged_battery_dAh = 0;
  init_energy_counters(0);
  expect(1, 0);
  EXPECT_EQ(second.total_charged_battery_Wh, 2);
  EXPECT_EQ(second.total_charged_battery_dAh, 2U);
  EXPECT_EQ(third.total_discharged_battery_Wh, 3);
  EXPECT_EQ(third.total_discharged_battery_dAh, 3U);
}

TEST_F(EnergyCounterTest, ExtraPackGatesDoNotLoseHistoricalTotalsOrCatchUpInactiveTime) {
  for (unsigned index = 1; index < 3; ++index) {
    auto& extra = index == 1 ? datalayer.battery2.status : datalayer.battery3.status;
    auto*& driver = index == 1 ? battery2 : battery3;
    driver = new CounterBattery(false, false, index == 1 ? &datalayer.battery2 : &datalayer.battery3);
    auto& joined = index == 1 ? datalayer.system.status.battery2_allowed_contactor_closing
                              : datalayer.system.status.battery3_allowed_contactor_closing;
    auto& engaged = index == 1 ? datalayer.system.status.contactors_battery2_engaged
                               : datalayer.system.status.contactors_battery3_engaged;
    auto& control = index == 1 ? contactor_control_enabled_double_battery : contactor_control_enabled_triple_battery;
    datalayer.system.info.configured_batteries = index + 1;
    init_energy_counters(now);
    extra.current_dA = 100;
    for (unsigned gate = 0; gate < 5; ++gate) {
      SCOPED_TRACE(testing::Message() << "pack=" << index + 1 << " gate=" << gate);
      joined = engaged = control = true;
      extra.CAN_battery_still_alive = CAN_STILL_ALIVE;
      extra.real_bms_status = BMS_ACTIVE;
      extra.voltage_dV = 100;
      if (gate == 0) {
        joined = false;
      }
      if (gate == 1) {
        extra.CAN_battery_still_alive = 0;
      }
      if (gate == 2) {
        extra.real_bms_status = BMS_FAULT;
      }
      if (gate == 3) {
        extra.voltage_dV = 0;
      }
      if (gate == 4) {
        engaged = false;
      }
      step(0, 360000);
      EXPECT_EQ(extra.total_charged_battery_Wh, 0);
      EXPECT_EQ(extra.total_charged_battery_dAh, 0U);
    }
    engaged = true;
    step(0, 36000);
    EXPECT_EQ(extra.total_charged_battery_Wh, 1);
    EXPECT_EQ(extra.total_charged_battery_dAh, 1U);
    joined = false;
    update_aggregate_values();
    EXPECT_GE(datalayer.aggregate.total_charged_battery_Wh, 1);
    EXPECT_GE(datalayer.aggregate.total_charged_battery_dAh, 1U);
  }
}

TEST_F(EnergyCounterTest, AggregateLifetimeTotalsSaturateWithoutSignedOverflow) {
  battery2 = new CounterBattery(true, true, &datalayer.battery2);
  datalayer.system.info.configured_batteries = 2;
  status.total_charged_battery_Wh = status.total_discharged_battery_Wh = INT32_MAX;
  status.total_charged_battery_dAh = status.total_discharged_battery_dAh = UINT32_MAX;
  datalayer.battery2.status.total_charged_battery_Wh = 1;
  datalayer.battery2.status.total_discharged_battery_Wh = 2;
  datalayer.battery2.status.total_charged_battery_dAh = 3;
  datalayer.battery2.status.total_discharged_battery_dAh = 4;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.total_charged_battery_Wh, INT32_MAX);
  EXPECT_EQ(datalayer.aggregate.total_discharged_battery_Wh, INT32_MAX);
  EXPECT_EQ(datalayer.aggregate.total_charged_battery_dAh, UINT32_MAX);
  EXPECT_EQ(datalayer.aggregate.total_discharged_battery_dAh, UINT32_MAX);
}

TEST_F(EnergyCounterTest, NegativeNativeWhCannotCancelOtherPacksOrUndoSaturation) {
  battery2 = new CounterBattery(true, true, &datalayer.battery2);
  battery3 = new CounterBattery(true, true, &datalayer.battery3);
  datalayer.system.info.configured_batteries = 3;
  const int32_t cases[][3] = {
      {-100, 150, 200}, {150, -100, 200}, {150, 200, -100},
      {INT32_MIN, -1, -100}, {INT32_MAX, 1, -100}, {-100, INT32_MAX, 1},
  };
  for (const auto& values : cases) {
    int64_t expected = 0;
    for (unsigned i = 0; i < 3; ++i) {
      auto& pack = i == 0 ? status : (i == 1 ? datalayer.battery2.status : datalayer.battery3.status);
      pack.total_charged_battery_Wh = values[i];
      pack.total_discharged_battery_Wh = values[2 - i];
      expected += values[i] > 0 ? values[i] : 0;
    }
    update_aggregate_values();
    const int32_t saturated = expected > INT32_MAX ? INT32_MAX : static_cast<int32_t>(expected);
    EXPECT_EQ(datalayer.aggregate.total_charged_battery_Wh, saturated);
    EXPECT_EQ(datalayer.aggregate.total_discharged_battery_Wh, saturated);
    EXPECT_EQ(status.total_charged_battery_Wh, values[0]);  // Native source stays driver-owned.
    EXPECT_EQ(datalayer.battery2.status.total_charged_battery_Wh, values[1]);
    EXPECT_EQ(datalayer.battery3.status.total_charged_battery_Wh, values[2]);
  }
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
