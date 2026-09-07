#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64() to control the test clock

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/parallel_safety.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

// Mirrors the file-scope contactor FSM in comm_contactorcontrol.cpp so the
// gate test can place it in a known state. Must match the definition there.
enum State { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
extern State contactorStatus;

// Tests for the symmetric parallel-join gate (#2711): the 1.5 V rule used to
// gate only battery 2/3 toward battery 1 - battery 1's own (re-)close was
// never checked, so after a main-pack dropout it could close onto a link
// another pack holds live, with the surge limited only by pack resistances.

class ParallelJoinSymmetryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    reset_parallel_safety_state();  // Latch and main-gate state start from boot, not from the previous case
    init_hal();
    battery2_detected = true;
    battery3_detected = false;
    datalayer.system.status.system_status = ACTIVE;
    // Off the 3700 sentinel so the startup grace lifts on the first pass;
    // MainGateStillEngagesWhenAPackSitsAtTheSentinelVoltage covers what
    // happens when a pack genuinely reads 370.0 V afterwards.
    datalayer.battery.status.voltage_dV = 3900;
    datalayer.battery2.status.voltage_dV = 3900;
  }

  void TearDown() override {
    battery2_detected = false;
    if (battery2) {
      delete battery2;
      battery2 = nullptr;
    }
  }
};

TEST_F(ParallelJoinSymmetryTest, EngagedPackWithLargeDiffBlocksMainClose) {
  datalayer.system.status.contactors_battery2_engaged = true;
  datalayer.battery2.status.voltage_dV = 3700 + 250;  // 25 V below main

  check_parallel_battery_safety(2);

  EXPECT_FALSE(datalayer.system.status.battery1_allowed_contactor_closing)
      << "Main battery must not close onto a live link with a large voltage difference";
}

TEST_F(ParallelJoinSymmetryTest, EngagedPackWithinWindowAllowsMainClose) {
  datalayer.system.status.contactors_battery2_engaged = true;
  datalayer.battery2.status.voltage_dV = 3910;  // 1.0 V difference

  check_parallel_battery_safety(2);

  EXPECT_TRUE(datalayer.system.status.battery1_allowed_contactor_closing);
}

TEST_F(ParallelJoinSymmetryTest, DisengagedPackDoesNotBlockMain) {
  datalayer.system.status.contactors_battery2_engaged = false;
  datalayer.battery2.status.voltage_dV = 3600;  // 30 V difference, but link is dead

  check_parallel_battery_safety(2);

  EXPECT_TRUE(datalayer.system.status.battery1_allowed_contactor_closing)
      << "A pack with open contactors holds no link - any voltage difference is fine";
}

TEST_F(ParallelJoinSymmetryTest, ExistingBattery2GatingUnchanged) {
  // Regression guard for the original direction of the rule
  datalayer.battery2.status.voltage_dV = 3905;
  check_parallel_battery_safety(2);
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);

  datalayer.battery2.status.voltage_dV = 3600;
  for (int i = 0; i < 11; i++) {
    check_parallel_battery_safety(2);
  }
  EXPECT_FALSE(datalayer.system.status.battery2_allowed_contactor_closing)
      << "Battery 2 must still disengage after 10 s out of sync";
}

TEST_F(ParallelJoinSymmetryTest, UnknownReportedStateFallsBackToCommanded) {
  // TestFake does not override reported_contactor_state() -> Unknown
  battery2 = new TestFakeBattery(&datalayer.battery2, CAN_NATIVE);
  datalayer.system.status.contactors_battery2_engaged = true;
  datalayer.battery2.status.voltage_dV = 3600;

  check_parallel_battery_safety(2);

  EXPECT_FALSE(datalayer.system.status.battery1_allowed_contactor_closing)
      << "Unknown reported state must fall back to the BE-commanded engaged state";

  datalayer.system.status.contactors_battery2_engaged = false;
  check_parallel_battery_safety(2);
  EXPECT_TRUE(datalayer.system.status.battery1_allowed_contactor_closing);
}

TEST_F(ParallelJoinSymmetryTest, GateBlocksStartPrecharge) {
  set_millis64(100000);  // Past the 10 s startup window
  contactor_control_enabled = true;
  contactorStatus = DISCONNECTED;
  battery_detected = true;
  datalayer.system.status.inverter_allows_contactor_closing = true;
  datalayer.system.info.equipment_stop_active = false;

  datalayer.system.status.battery1_allowed_contactor_closing = false;
  handle_contactors();
  EXPECT_EQ(contactorStatus, DISCONNECTED) << "The join gate must hold the main battery in DISCONNECTED";

  datalayer.system.status.battery1_allowed_contactor_closing = true;
  handle_contactors();
  // The FSM transitions DISCONNECTED -> START_PRECHARGE -> PRECHARGE within one tick
  EXPECT_EQ(contactorStatus, PRECHARGE);

  contactor_control_enabled = false;
  contactorStatus = DISCONNECTED;
  set_millis64(0);
}

/* Raised by @jonny5532 in review: 3700 dV means "no voltage decoded yet", but it
 * is also an ordinary reading for a pack sitting at 370.0 V. The skip used to be
 * a continuous condition placed ABOVE the line that computes
 * main_blocked_by_joiner, so a pack genuinely at that voltage left
 * battery1_allowed_contactor_closing at its fail-open default and the symmetric
 * gate never engaged at all - the hazard this PR exists to prevent, silently
 * disabled. The latch makes the skip a startup grace, so once both packs have
 * been seen off the sentinel the gate keeps working through it. */
TEST_F(ParallelJoinSymmetryTest, MainGateStillEngagesWhenAPackSitsAtTheSentinelVoltage) {
  datalayer.system.status.contactors_battery2_engaged = true;

  // One pass with both packs off the sentinel and in sync: the grace lifts
  check_parallel_battery_safety(2);
  ASSERT_TRUE(datalayer.system.status.battery1_allowed_contactor_closing);

  // The main pack now genuinely reads 370.0 V while the engaged pack sits 25 V away
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3950;

  check_parallel_battery_safety(2);

  EXPECT_FALSE(datalayer.system.status.battery1_allowed_contactor_closing)
      << "A main pack reading exactly 370.0 V must not suspend the symmetric gate";
}
