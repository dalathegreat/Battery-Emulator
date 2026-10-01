#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* A pack reads 0 V until its integration has decoded a voltage. The safety layer must treat that
   as "no reading": no undervoltage event, no discharge, and no user voltage-limit latch taken on
   a number that was never measured. */

namespace {

class UnreadVoltageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and deleted the packs.
    // setup() is deliberately not called: it would give the fake pack its 370.0 V.
    battery = new TestFakeBattery();
    datalayer.system.info.CPU_free_heap = 200000;  // Keep the low-heap check quiet
    emulator_pause_request_ON = false;             // A global other suites leave behind
    datalayer.battery.status.real_soc = 5000;
    datalayer.aggregate.reported_soc = 5000;
    datalayer.battery.status.max_discharge_power_W = 5000;
    datalayer.battery.status.max_charge_power_W = 5000;
  }
};

}  // namespace

TEST_F(UnreadVoltageTest, UndecodedVoltageBlocksDischargeWithoutAnEvent) {
  ASSERT_EQ(datalayer.battery.status.voltage_dV, 0);

  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_BATTERY_UNDERVOLTAGE)->occurences, 0);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
}

TEST_F(UnreadVoltageTest, GenuineUndervoltageIsStillReported) {
  datalayer.battery.status.voltage_dV = datalayer.battery.info.min_design_voltage_dV - 10;

  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_BATTERY_UNDERVOLTAGE)->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
}

// The discharge latch releases only 2.0 V above the limit. Taking it on the 0 V a pack reads
// before its first decode would hold discharge back for a pack sitting just above the limit.
TEST_F(UnreadVoltageTest, UserDischargeLimitDoesNotLatchOnUndecodedVoltage) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 4000;
  datalayer.battery_settings.max_user_set_discharge_voltage_dV = 3500;

  update_machineryprotection();  // 0 V, nothing decoded yet

  datalayer.battery.status.voltage_dV = 3510;  // decoded: 1.0 V above the limit
  datalayer.battery.status.max_discharge_power_W = 5000;
  update_machineryprotection();

  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);
}
