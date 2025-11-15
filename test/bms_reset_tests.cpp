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
