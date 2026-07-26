#include <gtest/gtest.h>

#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

#include "Arduino.h"

const unsigned long bmsWarmupDuration = 3000;

// Test a BMS reqest sequence from end to end. This is for the case where the
// contactors are powered directly by BE, so the reset doesn't need to wait for
// zero current before cutting the BMS power.
TEST(BmsResetTests, BmsResetSequenceDirectSuccess) {
  set_millis64(0xffffffffffffffff - 10);  // Test overflow handling
  remote_bms_reset = true;
  contactor_control_enabled = true;
  datalayer.battery.settings.user_set_bms_reset_duration_ms = 30000;  // 30 seconds

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Nothing has happened yet, should still be idle.
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  start_bms_reset();

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should have gone straight to powered-off
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);
  EXPECT_EQ(emulator_pause_request_ON, true);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powered off
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  set_millis64(20000);  // Simulate 20 seconds passing

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powered off
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);
  EXPECT_EQ(emulator_pause_request_ON, true);

  set_millis64(31000);  // Simulate 31 seconds passing

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should now be powering on
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERING_ON);
  EXPECT_EQ(emulator_pause_request_ON, true);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powering on
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERING_ON);
  EXPECT_EQ(emulator_pause_request_ON, true);

  set_millis64(31000 + bmsWarmupDuration + 1000);  // Simulate warmup duration passing

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should now be idle again
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  EXPECT_EQ(emulator_pause_request_ON, false);
}

// Test a BMS reqest sequence from end to end. This is for the case where the
// contactors are powered by the BMS, so the reset needs to wait for zero
// current before cutting power to avoid arcing.
TEST(BmsResetTests, BmsResetSequenceWaitSuccess) {
  set_millis64(0xffffffffffffffff - 10);
  remote_bms_reset = true;
  contactor_control_enabled = false;
  datalayer.battery.settings.user_set_bms_reset_duration_ms = 30000;  // 30 seconds
  datalayer.battery.status.current_dA = -50;                          // Simulate battery under load

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Nothing has happened yet, should still be idle.
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  start_bms_reset();

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should be waiting for current to drop to zero
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_WAITING_FOR_PAUSE);
  EXPECT_EQ(emulator_pause_request_ON, true);

  datalayer.battery.status.current_dA = 5;  // Reduce to a tiny load

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be waiting for current to drop to zero
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_WAITING_FOR_PAUSE);
  EXPECT_EQ(emulator_pause_request_ON, true);

  set_millis64(6000);  // Simulate 6 seconds passing

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should now be powered off (past 5s low-current threshold)
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);
  EXPECT_EQ(emulator_pause_request_ON, true);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powered off
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  set_millis64(26000);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powered off
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);
  EXPECT_EQ(emulator_pause_request_ON, true);

  set_millis64(37000);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should now be powering on
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERING_ON);
  EXPECT_EQ(emulator_pause_request_ON, true);

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // Should still be powering on
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERING_ON);
  EXPECT_EQ(emulator_pause_request_ON, true);

  set_millis64(37000 + bmsWarmupDuration + 1000);  // Simulate warmup duration passing

  for (int i = 0; i < 10; i++)
    handle_BMSpower();

  // BMS should now be idle again
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  EXPECT_EQ(emulator_pause_request_ON, false);
}

// Bring the reset scheduling state into the test so we can decide when the
// configured interval has elapsed instead of having to simulate a full day.
extern unsigned long lastPowerRemovalTime;
extern bool periodicResetDeferred;
extern bool balancingPeriodSkipped;

static const unsigned long ONE_HOUR_MS = 60UL * 60UL * 1000UL;

// Puts the state machine back to a known good starting point, with both guards
// enabled but neither condition met.
static void setup_periodic_reset_test(uint16_t interval_h) {
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  datalayer.system.info.equipment_stop_active = false;
  remote_bms_reset = false;
  periodic_bms_reset = true;
  contactor_control_enabled = true;  // Lets the reset jump straight to powered off
  periodic_bms_reset_interval_h = interval_h;
  periodic_bms_reset_defer_low_soc = false;
  periodic_bms_reset_skip_balancing = false;

  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.reported_soc = 5000;
  datalayer.battery.status.balancing_status = BALANCING_STATUS_READY;
  datalayer.battery2.status.balancing_status = BALANCING_STATUS_UNKNOWN;
  datalayer.battery3.status.balancing_status = BALANCING_STATUS_UNKNOWN;

  set_millis64(0);
  lastPowerRemovalTime = 0;
  periodicResetDeferred = false;
  balancingPeriodSkipped = false;
}

static void teardown_periodic_reset_test() {
  periodic_bms_reset = false;
  periodic_bms_reset_interval_h = 24;
  periodic_bms_reset_defer_low_soc = false;
  periodic_bms_reset_skip_balancing = false;
  periodicResetDeferred = false;
  balancingPeriodSkipped = false;
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  setBatteryPause(false, false, EquipmentStop::UNCHANGED, false);
}

// The reset should follow the user configured interval rather than a hardcoded 24h.
TEST(BmsResetTests, PeriodicBmsResetInterval) {
  setup_periodic_reset_test(48);

  set_millis64(25 * ONE_HOUR_MS);
  handle_BMSpower();
  // Only 25h in, a 48h interval hasn't elapsed yet
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  set_millis64(49 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  teardown_periodic_reset_test();
}

// A low real SOC holds the reset back until SOC recovers, and the following period
// then starts from the moment the deferred reset actually ran.
TEST(BmsResetTests, PeriodicBmsResetDeferLowRealSoc) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_defer_low_soc = true;

  // Interval elapsed, but the pack is below 15.00%
  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery.status.real_soc = 1400;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  // Still low much later, still nothing
  set_millis64(40 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  // SOC recovers, the reset runs immediately rather than waiting for the next period
  set_millis64(45 * ONE_HOUR_MS);
  datalayer.battery.status.real_soc = 5000;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  // The interval is re-anchored to the deferred point, not to the original due time
  EXPECT_EQ(lastPowerRemovalTime, 45 * ONE_HOUR_MS);

  teardown_periodic_reset_test();
}

// The scaled SOC defers the reset just like the real one does.
TEST(BmsResetTests, PeriodicBmsResetDeferLowScaledSoc) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_defer_low_soc = true;

  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery.status.reported_soc = 500;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  datalayer.battery.status.reported_soc = 5000;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  teardown_periodic_reset_test();
}

// The threshold is "less than 15%", so exactly 15.00% still resets.
TEST(BmsResetTests, PeriodicBmsResetDeferSocThreshold) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_defer_low_soc = true;

  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery.status.real_soc = 1500;
  datalayer.battery.status.reported_soc = 1500;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  teardown_periodic_reset_test();
}

// Balancing costs the reset exactly one period. The next occurrence goes ahead even
// though balancing never stopped.
TEST(BmsResetTests, PeriodicBmsResetSkipBalancingOnePeriodOnly) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_skip_balancing = true;
  datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;

  // First occurrence is given up
  set_millis64(25 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  EXPECT_EQ(lastPowerRemovalTime, 25 * ONE_HOUR_MS);

  // Part way through the next period, still nothing
  set_millis64(40 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  // Second occurrence runs despite balancing still being active
  set_millis64(50 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  teardown_periodic_reset_test();
}

// After a reset has happened, balancing is allowed to skip a period again.
TEST(BmsResetTests, PeriodicBmsResetBalancingAllowanceRestored) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_skip_balancing = true;
  datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;

  set_millis64(25 * ONE_HOUR_MS);
  handle_BMSpower();  // Skipped
  set_millis64(50 * ONE_HOUR_MS);
  handle_BMSpower();  // Ran, allowance restored
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  // Pretend the reset sequence finished
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  lastPowerRemovalTime = 50 * ONE_HOUR_MS;

  set_millis64(75 * ONE_HOUR_MS);
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  teardown_periodic_reset_test();
}

// Balancing on a secondary battery costs a period as well.
TEST(BmsResetTests, PeriodicBmsResetSkipBalancingSecondBattery) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_skip_balancing = true;

  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery2.status.balancing_status = BALANCING_STATUS_ACTIVE;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);

  teardown_periodic_reset_test();
}

// A deferring SOC takes precedence over the balancing skip, and must not consume the
// one period balancing allowance while it holds the reset back.
TEST(BmsResetTests, PeriodicBmsResetDeferDoesNotConsumeBalancingAllowance) {
  setup_periodic_reset_test(24);
  periodic_bms_reset_defer_low_soc = true;
  periodic_bms_reset_skip_balancing = true;

  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery.status.real_soc = 1000;
  datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  EXPECT_FALSE(balancingPeriodSkipped);

  // SOC recovers while balancing is still active, so the balancing skip applies now
  datalayer.battery.status.real_soc = 5000;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_IDLE);
  EXPECT_TRUE(balancingPeriodSkipped);

  teardown_periodic_reset_test();
}

// With both guards disabled the reset must run regardless of SOC and balancing.
TEST(BmsResetTests, PeriodicBmsResetGuardsDisabled) {
  setup_periodic_reset_test(24);

  set_millis64(25 * ONE_HOUR_MS);
  datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;
  datalayer.battery.status.real_soc = 100;
  datalayer.battery.status.reported_soc = 100;
  handle_BMSpower();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_POWERED_OFF);

  teardown_periodic_reset_test();
}
