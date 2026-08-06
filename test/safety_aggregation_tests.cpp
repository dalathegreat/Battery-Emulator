#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

// Covers the per-battery protection suite in update_machineryprotection() and
// the semantics of the events that several packs share. The aliveness and
// corrupted-CAN parts of that function are covered by battery_alive_tests.cpp;
// this file starts where those leave off.

namespace {

// Thresholds are compile-time constants in safety.cpp/safety.h. Tests derive
// their values from them rather than hardcoding numbers, so a changed limit
// moves the tests with it instead of silently invalidating them.
constexpr int16_t kOverheatDeciC = BATTERY_MAXTEMPERATURE + 10;
constexpr int16_t kFrozenDeciC = BATTERY_MINTEMPERATURE - 10;
constexpr uint16_t kDeviationLimitMv = 300;

bool event_active(EVENTS_ENUM_TYPE event) {
  const EVENTS_STRUCT_TYPE* entry = get_event_pointer(event);
  return entry->state == EVENT_STATE_ACTIVE || entry->state == EVENT_STATE_ACTIVE_LATCHED;
}

class SafetyAggregationTest : public ::testing::Test {
 protected:
  // Puts one pack's values comfortably inside every limit this file exercises.
  static void make_healthy(DATALAYER_BATTERY_TYPE& pack) {
    pack.info.max_design_voltage_dV = 4040;
    pack.info.min_design_voltage_dV = 2450;
    pack.info.max_cell_voltage_mV = 4250;
    pack.info.min_cell_voltage_mV = 2500;
    pack.info.max_cell_voltage_deviation_mV = kDeviationLimitMv;
    pack.status.voltage_dV = 3700;
    pack.status.cell_max_voltage_mV = 3800;
    pack.status.cell_min_voltage_mV = 3750;
    pack.status.temperature_max_dC = 200;
    pack.status.temperature_min_dC = 180;
    pack.status.soh_pptt = 9900;
    pack.status.real_soc = 5000;
    pack.status.reported_soc = 5000;
    pack.status.max_charge_power_W = 5000;
    pack.status.max_discharge_power_W = 5000;
    pack.status.active_power_W = 0;
    pack.status.CAN_error_counter = 0;
    pack.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  }

  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    init_hal();
    // The low-heap check runs before the battery blocks and would otherwise
    // raise an unrelated event on every call (CPU_free_heap defaults to 0).
    datalayer.system.info.CPU_free_heap = 200000;
    datalayer.system.status.system_status = ACTIVE;

    if (!battery3) {
      // The fake battery is the only integration that can be instantiated
      // three times, which the multi-pack cases below need.
      user_selected_battery_type = BatteryType::TestFake;
      user_selected_second_battery = true;
      user_selected_triple_battery = true;
      setup_battery();
    }
    ASSERT_NE(battery, nullptr);
    ASSERT_NE(battery2, nullptr);
    ASSERT_NE(battery3, nullptr);

    battery_detected = true;
    battery2_detected = true;
    battery3_detected = true;
    emulator_pause_request_ON = false;

    make_healthy(datalayer.battery);
    make_healthy(datalayer.battery2);
    make_healthy(datalayer.battery3);

    // update_machineryprotection() keeps several latches in function-static
    // bools that no reset touches, so a run that trips one would leak into the
    // next test. One all-clean pass releases them; the events it may have set
    // are cleared straight after.
    update_machineryprotection();
    reset_all_events();
  }
};

// --- Group A: the shared cell-deviation event ------------------------------
//
// Each battery block used to set AND clear EVENT_CELL_DEVIATION_HIGH from its
// own pack, so the last block to run decided the outcome for every pack.

TEST_F(SafetyAggregationTest, DeviationHighOnBattery1NotClearedByHealthyBattery2) {
  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 3600;  // 400mV spread, over the limit
  datalayer.battery2.status.cell_max_voltage_mV = 3800;
  datalayer.battery2.status.cell_min_voltage_mV = 3790;  // healthy

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH))
      << "battery 2 being healthy must not clear battery 1's deviation warning";
}

TEST_F(SafetyAggregationTest, DeviationHighOnBattery2NotClearedByHealthyBattery1) {
  datalayer.battery.status.cell_max_voltage_mV = 3800;
  datalayer.battery.status.cell_min_voltage_mV = 3790;  // healthy
  datalayer.battery2.status.cell_max_voltage_mV = 4000;
  datalayer.battery2.status.cell_min_voltage_mV = 3600;  // over the limit

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH));
}

TEST_F(SafetyAggregationTest, DeviationHighOnBattery3NotClearedByHealthierPacks) {
  datalayer.battery3.status.cell_max_voltage_mV = 4000;
  datalayer.battery3.status.cell_min_voltage_mV = 3600;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH));
}

TEST_F(SafetyAggregationTest, DeviationHighClearsOnlyWhenAllPacksClean) {
  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 3600;
  datalayer.battery2.status.cell_max_voltage_mV = 4000;
  datalayer.battery2.status.cell_min_voltage_mV = 3600;

  update_machineryprotection();
  ASSERT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH));

  // Battery 1 recovers; battery 2 is still deviating, so the warning stands.
  datalayer.battery.status.cell_max_voltage_mV = 3800;
  datalayer.battery.status.cell_min_voltage_mV = 3790;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH));

  // Both clean: only now does it clear.
  datalayer.battery2.status.cell_max_voltage_mV = 3800;
  datalayer.battery2.status.cell_min_voltage_mV = 3790;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_CELL_DEVIATION_HIGH));
}

TEST_F(SafetyAggregationTest, DeviationHighUnaffectedBySingleBatterySetup) {
  // What almost every user runs, and the configuration the aggregation must
  // leave exactly as it was. The production code gates each extra pack on its
  // battery pointer, so removing them is what "single battery" means here.
  Battery* saved_battery2 = battery2;
  Battery* saved_battery3 = battery3;
  battery2 = nullptr;
  battery3 = nullptr;

  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 3600;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_DEVIATION_HIGH));

  datalayer.battery.status.cell_max_voltage_mV = 3800;
  datalayer.battery.status.cell_min_voltage_mV = 3790;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_CELL_DEVIATION_HIGH));

  battery2 = saved_battery2;
  battery3 = saved_battery3;
}

// EVENT_SOH_DIFFERENCE compares battery 1 against each extra pack and is shared
// the same way, so it had the same defect: the battery 3 comparison decided the
// outcome for the battery 2 comparison too.
TEST_F(SafetyAggregationTest, SohDifferenceOnPack2NotClearedByMatchingPack3) {
  datalayer.battery.status.soh_pptt = 9000;
  datalayer.battery2.status.soh_pptt = 5000;  // 40.00% apart, over the limit
  datalayer.battery3.status.soh_pptt = 9000;  // identical to battery 1

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_SOH_DIFFERENCE))
      << "battery 3 matching battery 1 must not clear the battery 2 SOH warning";
}

// The mirror, and the one that actually proves the battery 3 pair is evaluated:
// with pack 2 matching pack 1, only the pack 3 comparison can raise the event.
// Deleting the battery 3 SOH block entirely would leave the test above green.
TEST_F(SafetyAggregationTest, SohDifferenceOnPack3AloneRaisesTheEvent) {
  datalayer.battery.status.soh_pptt = 9000;
  datalayer.battery2.status.soh_pptt = 9000;  // identical to battery 1
  datalayer.battery3.status.soh_pptt = 5000;  // 40.00% apart, over the limit

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_SOH_DIFFERENCE));
}

TEST_F(SafetyAggregationTest, SohDifferenceClearsWhenEveryPairIsClose) {
  datalayer.battery.status.soh_pptt = 9000;
  datalayer.battery2.status.soh_pptt = 5000;
  update_machineryprotection();
  ASSERT_TRUE(event_active(EVENT_SOH_DIFFERENCE));

  datalayer.battery2.status.soh_pptt = 8900;
  datalayer.battery3.status.soh_pptt = 9000;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_SOH_DIFFERENCE));
}

// --- Group B: the per-battery protection suite -----------------------------

TEST_F(SafetyAggregationTest, OverheatSetsAboveThresholdAndClearsBelow) {
  datalayer.battery.status.temperature_max_dC = kOverheatDeciC;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_OVERHEAT));
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY_OVERHEAT)->data, (uint8_t)kOverheatDeciC);

  datalayer.battery.status.temperature_max_dC = 200;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_BATTERY_OVERHEAT));
}

TEST_F(SafetyAggregationTest, FrozenSetsBelowThresholdAndClearsAbove) {
  datalayer.battery.status.temperature_min_dC = kFrozenDeciC;
  // Keep the spread inside its own limit so only the frozen event fires.
  datalayer.battery.status.temperature_max_dC = kFrozenDeciC + 100;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_FROZEN));

  datalayer.battery.status.temperature_min_dC = 180;
  datalayer.battery.status.temperature_max_dC = 200;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_BATTERY_FROZEN));
}

TEST_F(SafetyAggregationTest, TempDeviationIsLatched) {
  datalayer.battery.status.temperature_max_dC = 300;
  datalayer.battery.status.temperature_min_dC = 300 - (BATTERY_MAX_TEMPERATURE_DEVIATION + 10);
  update_machineryprotection();
  ASSERT_TRUE(event_active(EVENT_BATTERY_TEMP_DEVIATION_HIGH));

  // Latched events survive the condition going away; only a user action clears
  // them. Losing this in a refactor would turn a sticky fault into a blip.
  datalayer.battery.status.temperature_max_dC = 200;
  datalayer.battery.status.temperature_min_dC = 190;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_TEMP_DEVIATION_HIGH));
}

TEST_F(SafetyAggregationTest, OvervoltageZeroesChargePower) {
  datalayer.battery.status.voltage_dV = datalayer.battery.info.max_design_voltage_dV + 10;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_BATTERY_OVERVOLTAGE));
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
}

TEST_F(SafetyAggregationTest, UndervoltageZeroesDischargePower) {
  datalayer.battery.status.voltage_dV = datalayer.battery.info.min_design_voltage_dV - 10;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_BATTERY_UNDERVOLTAGE));
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
}

TEST_F(SafetyAggregationTest, CellOverVoltageIsLatchingAndCriticalAboveMargin) {
  datalayer.battery.status.cell_max_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_OVER_VOLTAGE));
  EXPECT_FALSE(event_active(EVENT_CELL_CRITICAL_OVER_VOLTAGE));

  datalayer.battery.status.cell_max_voltage_mV = datalayer.battery.info.max_cell_voltage_mV + CELL_CRITICAL_MV;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_CRITICAL_OVER_VOLTAGE));

  // Neither is cleared by the cells coming back down: both are set-only.
  datalayer.battery.status.cell_max_voltage_mV = 3800;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_OVER_VOLTAGE));
  EXPECT_TRUE(event_active(EVENT_CELL_CRITICAL_OVER_VOLTAGE));
}

TEST_F(SafetyAggregationTest, CellUnderVoltageIsLatchingAndCriticalBelowMargin) {
  datalayer.battery.status.cell_min_voltage_mV = datalayer.battery.info.min_cell_voltage_mV;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_UNDER_VOLTAGE));
  EXPECT_FALSE(event_active(EVENT_CELL_CRITICAL_UNDER_VOLTAGE));
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);

  datalayer.battery.status.cell_min_voltage_mV = datalayer.battery.info.min_cell_voltage_mV - CELL_CRITICAL_MV;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_CRITICAL_UNDER_VOLTAGE));

  datalayer.battery.status.cell_min_voltage_mV = 3750;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CELL_UNDER_VOLTAGE));
  EXPECT_TRUE(event_active(EVENT_CELL_CRITICAL_UNDER_VOLTAGE));
}

// The hysteresis governs the CHARGE BLOCK, not the event: the event is set-only
// at the ceiling, while charge power stays at zero until the max cell has come
// back down a full CELL_HYSTERESIS_MV. Sitting between the two thresholds must
// keep charging blocked, which is what stops contactor chatter at the knee.
TEST_F(SafetyAggregationTest, CellOverVoltageChargeBlockUsesHysteresis) {
  const uint16_t limit_mV = datalayer.battery.info.max_cell_voltage_mV;

  datalayer.battery.status.cell_max_voltage_mV = limit_mV;
  update_machineryprotection();
  ASSERT_EQ(datalayer.battery.status.max_charge_power_W, 0u);

  // Just below the ceiling but inside the hysteresis band: still blocked.
  datalayer.battery.status.cell_max_voltage_mV = limit_mV - (CELL_HYSTERESIS_MV / 2);
  datalayer.battery.status.max_charge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u) << "charging must stay blocked inside the hysteresis band";

  // Below the hysteresis point: released.
  datalayer.battery.status.cell_max_voltage_mV = limit_mV - CELL_HYSTERESIS_MV - 1;
  datalayer.battery.status.max_charge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);
}

TEST_F(SafetyAggregationTest, SohLowSetsAndClears) {
  datalayer.battery.status.soh_pptt = 2000;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_SOH_LOW));

  datalayer.battery.status.soh_pptt = 9900;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_SOH_LOW));
}

TEST_F(SafetyAggregationTest, PauseRequestZeroesBothPowerLimitsOnEveryPack) {
  emulator_pause_request_ON = true;

  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery2.status.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.battery3.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery3.status.max_discharge_power_W, 0u);

  emulator_pause_request_ON = false;
}

TEST_F(SafetyAggregationTest, FaultStateZeroesBothPowerLimits) {
  // The other half of the same condition as the pause request. A system in
  // FAULT must not keep offering power.
  datalayer.system.status.system_status = FAULT;

  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
}

// The user-set charge/discharge voltage window. Both sides latch and release on
// a hysteresis band, which is what stops the limit chattering at the knee, and
// both latches live in function-static bools that no reset touches.
TEST_F(SafetyAggregationTest, UserSetChargeVoltageLimitLatchesAndReleasesOnHysteresis) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 3800;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 3000;

  // Start from a released latch: well inside both bands.
  datalayer.battery.status.voltage_dV = 3400;
  update_machineryprotection();
  ASSERT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);

  // At the ceiling: charging blocked.
  datalayer.battery.status.voltage_dV = 3800;
  datalayer.battery.status.max_charge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);

  // Dropped below the target but still inside the hysteresis band: the latch
  // holds, so charging stays blocked.
  datalayer.battery.status.voltage_dV = 3800 - (HYSTERESIS_OFFSET_DV / 2);
  datalayer.battery.status.max_charge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u)
      << "charge limit must not release inside the hysteresis band";

  // Below the release point: charging allowed again.
  datalayer.battery.status.voltage_dV = 3800 - HYSTERESIS_OFFSET_DV - 1;
  datalayer.battery.status.max_charge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);

  datalayer.battery.settings.user_set_voltage_limits_active = false;
}

TEST_F(SafetyAggregationTest, UserSetDischargeVoltageLimitLatchesAndReleasesOnHysteresis) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 3900;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 3000;

  datalayer.battery.status.voltage_dV = 3400;
  update_machineryprotection();
  ASSERT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);

  // At the floor: discharging blocked.
  datalayer.battery.status.voltage_dV = 3000;
  datalayer.battery.status.max_discharge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);

  // Risen above the floor but inside the band: still blocked.
  datalayer.battery.status.voltage_dV = 3000 + (HYSTERESIS_OFFSET_DV / 2);
  datalayer.battery.status.max_discharge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);

  // Above the release point: allowed again.
  datalayer.battery.status.voltage_dV = 3000 + HYSTERESIS_OFFSET_DV + 1;
  datalayer.battery.status.max_discharge_power_W = 5000;
  update_machineryprotection();
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);

  datalayer.battery.settings.user_set_voltage_limits_active = false;
}

TEST_F(SafetyAggregationTest, BatteryFullSetsOnceAndZeroesChargePower) {
  datalayer.battery.status.real_soc = 10000;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_BATTERY_FULL));
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);

  datalayer.battery.status.real_soc = 5000;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_BATTERY_FULL));
}

TEST_F(SafetyAggregationTest, BatteryEmptySetsOnceAndZeroesDischargePower) {
  datalayer.battery.status.real_soc = 0;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_BATTERY_EMPTY));
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);

  datalayer.battery.status.real_soc = 5000;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_BATTERY_EMPTY));
}

// The two are NOT symmetric: the empty check sits behind a system_status ==
// ACTIVE gate that the full check does not have, so a non-active system reports
// a full battery but never an empty one. Pinned because it looks like an
// oversight and a rewrite would plausibly "tidy" it in either direction.
TEST_F(SafetyAggregationTest, BatteryEmptyIsGatedOnActiveButBatteryFullIsNot) {
  datalayer.system.status.system_status = UPDATING;
  datalayer.battery.status.real_soc = 0;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_BATTERY_EMPTY)) << "the empty check is skipped entirely when not ACTIVE";

  datalayer.system.status.system_status = UPDATING;
  datalayer.battery.status.real_soc = 10000;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_FULL)) << "the full check has no such gate";
}

// The inverter overdraw alarms tolerate a 2 kW margin and only fire after
// MAX_CHARGE_DISCHARGE_LIMIT_FAILURES consecutive offending cycles, so a brief
// overshoot is not reported. Both the margin and the count are easy to lose.
TEST_F(SafetyAggregationTest, ChargeLimitExceededOnlyAfterRepeatedOvershoot) {
  // Establish a known counter: one in-tolerance cycle resets it.
  datalayer.battery.status.active_power_W = 1000;
  update_machineryprotection();

  // Inside the 2 kW margin: never fires, however long it lasts.
  datalayer.battery.status.active_power_W = datalayer.battery.status.max_charge_power_W + 1000;
  for (int i = 0; i < 10; ++i) {
    update_machineryprotection();
  }
  EXPECT_FALSE(event_active(EVENT_CHARGE_LIMIT_EXCEEDED)) << "an overshoot within the 2 kW margin is tolerated";

  // Beyond the margin: tolerated until the failure count is exceeded.
  datalayer.battery.status.active_power_W = datalayer.battery.status.max_charge_power_W + 3000;
  for (int i = 0; i <= MAX_CHARGE_DISCHARGE_LIMIT_FAILURES; ++i) {
    update_machineryprotection();
    EXPECT_FALSE(event_active(EVENT_CHARGE_LIMIT_EXCEEDED)) << "fired too early, at cycle " << i;
  }
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_CHARGE_LIMIT_EXCEEDED));

  // Back inside the limit clears it and resets the count.
  datalayer.battery.status.active_power_W = 1000;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_CHARGE_LIMIT_EXCEEDED));
}

TEST_F(SafetyAggregationTest, DischargeLimitExceededOnlyAfterRepeatedOvershoot) {
  datalayer.battery.status.active_power_W = -1000;
  update_machineryprotection();

  datalayer.battery.status.active_power_W = -(int32_t)(datalayer.battery.status.max_discharge_power_W + 1000);
  for (int i = 0; i < 10; ++i) {
    update_machineryprotection();
  }
  EXPECT_FALSE(event_active(EVENT_DISCHARGE_LIMIT_EXCEEDED));

  datalayer.battery.status.active_power_W = -(int32_t)(datalayer.battery.status.max_discharge_power_W + 3000);
  for (int i = 0; i <= MAX_CHARGE_DISCHARGE_LIMIT_FAILURES; ++i) {
    update_machineryprotection();
    EXPECT_FALSE(event_active(EVENT_DISCHARGE_LIMIT_EXCEEDED)) << "fired too early, at cycle " << i;
  }
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_DISCHARGE_LIMIT_EXCEEDED));

  datalayer.battery.status.active_power_W = -1000;
  update_machineryprotection();
  EXPECT_FALSE(event_active(EVENT_DISCHARGE_LIMIT_EXCEEDED));
}

// Whatever zeroed a power limit, the matching current limit has to follow it
// down - the inverter side reads the current, so a stale ampere value would
// undo the safety that just fired.
TEST_F(SafetyAggregationTest, ZeroedPowerLimitsAlsoZeroTheCurrentLimits) {
  datalayer.battery.status.max_charge_current_dA = 300;
  datalayer.battery.status.max_discharge_current_dA = 300;
  datalayer.battery.status.max_charge_power_W = 0;
  datalayer.battery.status.max_discharge_power_W = 0;

  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 0);
}

TEST_F(SafetyAggregationTest, NonZeroPowerLimitsLeaveCurrentLimitsAlone) {
  datalayer.battery.status.max_charge_current_dA = 300;
  datalayer.battery.status.max_discharge_current_dA = 300;

  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 300);
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 300);
}

// --- Group C: multi-pack semantics -----------------------------------------

TEST_F(SafetyAggregationTest, CellOverVoltageOnEitherPackRaisesTheSharedEvent) {
  datalayer.battery2.status.cell_max_voltage_mV = datalayer.battery2.info.max_cell_voltage_mV;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_CELL_OVER_VOLTAGE)) << "battery 2 alone at its ceiling must raise the shared event";
}

TEST_F(SafetyAggregationTest, ThirdPackParticipatesInSharedEvents) {
  datalayer.battery3.status.cell_min_voltage_mV = datalayer.battery3.info.min_cell_voltage_mV;

  update_machineryprotection();

  EXPECT_TRUE(event_active(EVENT_CELL_UNDER_VOLTAGE));
}

TEST_F(SafetyAggregationTest, ExtraPackPowerLimitsAreIndependent) {
  // Battery 1 goes overvoltage, which zeroes its charge power. The other packs
  // are untouched - the array refactor makes the limits per-instance, and this
  // pins that they already are.
  datalayer.battery.status.voltage_dV = datalayer.battery.info.max_design_voltage_dV + 10;

  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery3.status.max_charge_power_W, 5000u);
}

}  // namespace

// --- Paths the coverage review found unpinned --------------------------------

// The reported SOC is what the inverter acts on, so the full/empty cutouts key
// off it as well as the real value - a scaled SOC at either end must stop the
// corresponding direction even when the real SOC is mid-range.
TEST_F(SafetyAggregationTest, BatteryFullAndEmptyFollowTheReportedSoc) {
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.reported_soc = 10000;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_FULL));
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);

  make_healthy(datalayer.battery);
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.reported_soc = 0;
  update_machineryprotection();
  EXPECT_TRUE(event_active(EVENT_BATTERY_EMPTY));
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
}

// Emergency recovery charge exists to revive a pack that has been left flat,
// but below a floor the cells are too far gone to charge safely. Reaching that
// floor must abort the mode outright rather than keep pushing current in.
TEST_F(SafetyAggregationTest, RecoveryChargeAbortsWhenCellsAreBelowTheSafetyFloor) {
  datalayer.battery.settings.user_requests_forced_charging_recovery_mode = true;
  datalayer.battery.settings.max_user_set_charge_dA = 20;
  datalayer.battery.settings.recovery_charge_start_time_ms = 0;
  datalayer.battery.settings.recovery_charge_max_time_ms = 600000;
  datalayer.battery.status.cell_min_voltage_mV = LOWEST_ALLOWED_CELLVOLTAGE_RECOVERY_CHARGE_MV - 1;

  update_machineryprotection();

  EXPECT_FALSE(datalayer.battery.settings.user_requests_forced_charging_recovery_mode)
      << "recovery charge must abort itself once the cells are below the floor";
  EXPECT_TRUE(event_active(EVENT_RECOVERY_END));
}

// Above the floor it stays enabled, and the charge current it allows is capped
// so a user-set value cannot exceed the recovery limit.
TEST_F(SafetyAggregationTest, RecoveryChargeCapsTheAllowedCurrent) {
  datalayer.battery.settings.user_requests_forced_charging_recovery_mode = true;
  datalayer.battery.settings.max_user_set_charge_dA = MAX_CHARGEPOWER_RECOVERY_CHARGE_DA + 500;
  datalayer.battery.settings.recovery_charge_start_time_ms = 0;
  datalayer.battery.settings.recovery_charge_max_time_ms = 600000;
  datalayer.battery.status.cell_min_voltage_mV = 3000;
  datalayer.battery.status.max_charge_power_W = 0;

  update_machineryprotection();

  EXPECT_TRUE(datalayer.battery.settings.user_requests_forced_charging_recovery_mode);
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, MAX_CHARGEPOWER_RECOVERY_CHARGE_DA)
      << "the user value must be clamped to the recovery limit";
}
