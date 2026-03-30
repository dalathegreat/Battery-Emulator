#include <gtest/gtest.h>

#include "../Software/src/core/parallel_safety.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"

class VoltageSyncTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    // Reset datalayer to known state
    datalayer.battery.status.voltage_dV = 3700;   // 370.0V
    datalayer.battery2.status.voltage_dV = 3700;  // 370.0V
    datalayer.battery3.status.voltage_dV = 3700;  // 370.0V
    datalayer.battery.status.bms_status = ACTIVE;
    datalayer.system.status.battery2_allowed_contactor_closing = true;
    datalayer.system.status.battery3_allowed_contactor_closing = true;
  }
};

// Test: When voltages are in sync, battery2 is allowed to close contactors
TEST_F(VoltageSyncTest, Battery2AllowedWhenVoltagesInSync) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3700;

  check_parallel_battery_safety(2);

  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
}

// Test: When voltage drifts >1.5V, battery2 should be disconnected after 10 seconds
TEST_F(VoltageSyncTest, Battery2DisconnectedAfterVoltageDriftTimeout) {
  datalayer.battery.status.voltage_dV = 3700;   // 370.0V
  datalayer.battery2.status.voltage_dV = 3500;  // 350.0V — 20V difference, way over 1.5V

  // Simulate 10 seconds of calls (function called once per second)
  for (int i = 0; i < 10; i++) {
    check_parallel_battery_safety(2);
    // During the first 10 calls, battery2 should still be allowed (counting up)
    EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing)
        << "Should still be allowed at second " << i;
  }

  // 11th call — counter reaches 10, battery2 should be disconnected
  check_parallel_battery_safety(2);
  EXPECT_FALSE(datalayer.system.status.battery2_allowed_contactor_closing);
}

// Test: After voltage drift timeout, re-syncing resets the counter and allows reconnection
TEST_F(VoltageSyncTest, Battery2ReconnectsAfterVoltagesResync) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3500;  // Out of sync

  // Drift for 11 seconds — battery2 disconnected
  for (int i = 0; i < 11; i++) {
    check_parallel_battery_safety(2);
  }
  EXPECT_FALSE(datalayer.system.status.battery2_allowed_contactor_closing);

  // Voltages re-sync
  datalayer.battery2.status.voltage_dV = 3700;
  check_parallel_battery_safety(2);
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
}

// Test: Battery3 has the same voltage drift timeout behavior
TEST_F(VoltageSyncTest, Battery3DisconnectedAfterVoltageDriftTimeout) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery3.status.voltage_dV = 3500;  // 20V difference

  for (int i = 0; i < 10; i++) {
    check_parallel_battery_safety(3);
    EXPECT_TRUE(datalayer.system.status.battery3_allowed_contactor_closing)
        << "Should still be allowed at second " << i;
  }

  check_parallel_battery_safety(3);
  EXPECT_FALSE(datalayer.system.status.battery3_allowed_contactor_closing);
}

// Test: Battery1 fault disengages battery2 even when voltages match
TEST_F(VoltageSyncTest, Battery1FaultDisengagesBattery2) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3700;  // In sync
  datalayer.battery.status.bms_status = FAULT;

  check_parallel_battery_safety(2);
  EXPECT_FALSE(datalayer.system.status.battery2_allowed_contactor_closing);
}

// Test: Zero voltage skips the check entirely (no crash, no state change)
TEST_F(VoltageSyncTest, ZeroVoltageSkipsCheck) {
  datalayer.battery.status.voltage_dV = 0;
  datalayer.battery2.status.voltage_dV = 3700;
  datalayer.system.status.battery2_allowed_contactor_closing = true;

  check_parallel_battery_safety(2);
  // Should remain unchanged — early return
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
}
