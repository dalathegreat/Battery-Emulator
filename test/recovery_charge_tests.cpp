#include <gtest/gtest.h>

#include <array>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* Forced recovery charge, end to end: the safety layer runs the mode for at most 30 minutes and
   ends it at once when a cell is below 2.0 V, and update_aggregate_limits() hands the inverter
   the small current it allows. Two deeply discharged packs, both on the DC link, cells below the
   critical undervoltage limit - the situation the mode exists for - and a BMS that will not
   charge them. */

namespace {

constexpr uint32_t RECOVERY_PERIOD_MS = 1800000;  // 30 minutes, the datalayer default

class RecoveryChargeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and events and deleted the packs.
    init_events();  // Event levels are only assigned here, and the critical events have to fault
    battery = new TestFakeBattery();
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    battery2->battery_index = 2;
    datalayer.system.info.configured_batteries = 2;
    datalayer.system.info.CPU_free_heap = 200000;  // Keep the low-heap check quiet
    datalayer.battery_settings.soc_scaling_active = false;
    emulator_pause_request_ON = false;  // Globals other suites leave behind
    emulator_pause_status = NORMAL;
    battery_detected = true;
    battery2_detected = true;
    battery3_detected = false;
    datalayer.system.status.battery2_allowed_contactor_closing = true;

    for (DATALAYER_BATTERY_TYPE* pack : packs()) {
      pack->status.voltage_dV = 2600;
      pack->status.real_soc = 100;
      pack->status.cell_max_voltage_mV = 2550;
      pack->status.cell_min_voltage_mV = 2500;
    }

    run_cycle(0);
    reset_all_events();
  }

  void TearDown() override { set_millis64(0); }

  static std::array<DATALAYER_BATTERY_TYPE*, 2> packs() { return {&datalayer.battery, &datalayer.battery2}; }

  // One pass of the core loop, in its order: the drivers publish what the BMS allows (nothing to
  // charge with), the packs are rolled up, the safety layer runs, the inverter limits are folded.
  // The clock reads the time the pass started with, as millis() does in the core loop.
  static void run_cycle(uint32_t now_ms) {
    set_millis64(now_ms);
    for (DATALAYER_BATTERY_TYPE* pack : packs()) {
      pack->status.max_charge_power_W = 0;
      pack->status.max_discharge_power_W = 0;
      pack->status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Still talking
    }
    update_aggregate_values();
    update_machineryprotection(now_ms);
    update_aggregate_limits();
  }

  static bool recovering() { return datalayer.battery_settings.user_requests_forced_charging_recovery_mode; }
  static const EVENTS_STRUCT_TYPE* event(EVENTS_ENUM_TYPE e) { return get_event_pointer(e); }
};

}  // namespace

TEST_F(RecoveryChargeTest, TheInverterIsGivenTheRecoveryCurrent) {
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;

  run_cycle(1000);

  EXPECT_TRUE(recovering());
  EXPECT_EQ(event(EVENT_RECOVERY_START)->occurences, 1);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);  // 5.0 A
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 1300u);  // 5.0 A at 260.0 V
}

TEST_F(RecoveryChargeTest, EndsAfterThirtyMinutes) {
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;
  run_cycle(1000);

  run_cycle(1000 + RECOVERY_PERIOD_MS - 1);
  EXPECT_TRUE(recovering());
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);

  run_cycle(1000 + RECOVERY_PERIOD_MS);
  EXPECT_FALSE(recovering());
  EXPECT_EQ(event(EVENT_RECOVERY_END)->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event(EVENT_RECOVERY_END)->data, 0);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
}

// Any cell below 2.0 V ends it at once, whichever pack it is in. Only pack 1's was checked, so a
// pack 2 cell far too low to charge safely kept receiving the recovery current.
TEST_F(RecoveryChargeTest, ACellBelowTwoVoltsOnPack2EndsItAtOnce) {
  datalayer.battery2.status.cell_min_voltage_mV = 1900;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;

  run_cycle(1000);

  EXPECT_FALSE(recovering());
  EXPECT_EQ(event(EVENT_RECOVERY_END)->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event(EVENT_RECOVERY_END)->data, 255);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);  // Not even for the one cycle
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
}

// A stop left the old start time behind, so starting again more than 30 minutes after the first
// start ended the mode within a second, without a start event.
TEST_F(RecoveryChargeTest, StartsAFullPeriodAgainAfterBeingStopped) {
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;
  run_cycle(1000);
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = false;  // Stopped from the web page
  run_cycle(60000);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);

  const uint32_t restart_ms = 1000 + RECOVERY_PERIOD_MS + 5000;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;
  run_cycle(restart_ms);

  EXPECT_TRUE(recovering());
  EXPECT_EQ(event(EVENT_RECOVERY_START)->occurences, 2);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);

  run_cycle(restart_ms + RECOVERY_PERIOD_MS - 1);  // The second run gets its own 30 minutes
  EXPECT_TRUE(recovering());
}
