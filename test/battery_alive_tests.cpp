#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/DALY-BMS.h"
#include "../Software/src/charger/CHARGERS.h"
#include "../Software/src/charger/CanCharger.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

// Tests for the shared CAN-aliveness handling of battery 1/2/3 and the charger
// in update_machineryprotection(), and for the aggregated corrupted-CAN warning
// (EVENT_CAN_CORRUPTED_WARNING is one event shared by all batteries).

// Defined in safety.cpp; not exposed via safety.h like the battery latches are.
extern bool charger_detected;

namespace {

class BatteryAliveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    init_hal();
    // Avoid tripping the low-heap check (CPU_free_heap defaults to 0)
    datalayer.system.info.CPU_free_heap = 200000;
    if (!battery2) {
      // BMW i3 supports double battery, so one setup creates both instances
      user_selected_battery_type = BatteryType::BmwI3;
      user_selected_second_battery = true;
      setup_battery();
    }
    if (!charger) {
      user_selected_charger_type = ChargerType::ChevyVolt;
      setup_charger();
    }
    ASSERT_NE(battery, nullptr);
    ASSERT_NE(battery2, nullptr);
    ASSERT_NE(charger, nullptr);
    // The detection latches are globals with no reset inside safety.cpp;
    // set them explicitly so tests are order-independent.
    battery_detected = true;
    battery2_detected = true;
    battery3_detected = true;
    charger_detected = true;
  }
};

}  // namespace

// The key regression: one battery's corruption warning must not be cleared by
// another battery's clean state. Before the aggregation, the per-battery
// set/clear blocks ran in sequence and the last one to run won, so a healthy
// battery 2 silently cleared battery 1's active warning every cycle.
TEST_F(BatteryAliveTest, CorruptedWarningNotMaskedByHealthySecondBattery) {
  datalayer.battery.status.CAN_error_counter = MAX_CAN_FAILURES + 1;
  datalayer.battery2.status.CAN_error_counter = 0;

  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_CAN_CORRUPTED_WARNING)->state, EVENT_STATE_ACTIVE)
      << "Battery 2's clean CAN state must not clear battery 1's corruption warning";
}

TEST_F(BatteryAliveTest, SecondBatteryCorruptionRaisesWarning) {
  datalayer.battery.status.CAN_error_counter = 0;
  datalayer.battery2.status.CAN_error_counter = MAX_CAN_FAILURES + 1;

  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_CAN_CORRUPTED_WARNING)->state, EVENT_STATE_ACTIVE);
}

TEST_F(BatteryAliveTest, CorruptedWarningClearsWhenAllBatteriesHealthy) {
  datalayer.battery.status.CAN_error_counter = MAX_CAN_FAILURES + 1;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_CORRUPTED_WARNING)->state, EVENT_STATE_ACTIVE);

  datalayer.battery.status.CAN_error_counter = 0;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_CORRUPTED_WARNING)->state, EVENT_STATE_INACTIVE);
}

// Detection must latch on >= CAN_STILL_ALIVE, not equality: some drivers refresh
// the counter to a multiple for a longer timeout (SMA x3, Sofar x2 on the
// inverter side today; the battery/charger checks are kept equivalent).
TEST_F(BatteryAliveTest, BatteryDetectionFiresWithMultipliedRefreshValue) {
  battery_detected = false;
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE * 3;

  update_machineryprotection();

  EXPECT_TRUE(battery_detected);
  EXPECT_EQ(get_event_pointer(EVENT_CAN_BATTERY_DETECTED)->occurences, 1);
}

TEST_F(BatteryAliveTest, BatteryMissingSetsAtZeroAndClearsOnRefresh) {
  datalayer.battery.status.CAN_battery_still_alive = 0;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_BATTERY_MISSING)->state, EVENT_STATE_ACTIVE);

  datalayer.battery.status.CAN_battery_still_alive = 10;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_BATTERY_MISSING)->state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 9) << "Counter must decrement on every cycle";
}

TEST_F(BatteryAliveTest, SecondBatteryMissingSetsAtZeroAndClearsOnRefresh) {
  datalayer.battery2.status.CAN_battery_still_alive = 0;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_BATTERY2_MISSING)->state, EVENT_STATE_ACTIVE);

  datalayer.battery2.status.CAN_battery_still_alive = 10;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_BATTERY2_MISSING)->state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery2.status.CAN_battery_still_alive, 9);
}

TEST_F(BatteryAliveTest, ChargerMissingSetsAtZeroAndClearsOnRefresh) {
  datalayer.charger.CAN_charger_still_alive = 0;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_CHARGER_MISSING)->state, EVENT_STATE_ACTIVE);

  datalayer.charger.CAN_charger_still_alive = 10;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_CHARGER_MISSING)->state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.charger.CAN_charger_still_alive, 9);
}

TEST_F(BatteryAliveTest, ChargerDetectionFiresOnRefresh) {
  charger_detected = false;
  datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;

  update_machineryprotection();

  EXPECT_TRUE(charger_detected);
  EXPECT_EQ(get_event_pointer(EVENT_CAN_CHARGER_DETECTED)->occurences, 1);
}

// A battery on a non-CAN interface never refreshes CAN_battery_still_alive, so running the
// CAN liveness check on it raises EVENT_CAN_BATTERY_MISSING every cycle. That is an ERROR,
// which puts system_status in FAULT, which zeroes both power limits on a healthy battery.
TEST_F(BatteryAliveTest, NonCanBatteryIsNotPolicedByTheCanWatchdog) {
  DalyBms rs485_battery;
  Battery* saved = battery;
  battery = &rs485_battery;

  ASSERT_NE(battery->interface_type(), BatteryInterfaceType::Can);

  // A healthy battery, so only the CAN watchdog is under test. The other checks in
  // update_machineryprotection() zero the limits for under/overvoltage too.
  auto& b = datalayer.battery;
  b.info.min_design_voltage_dV = 3500;
  b.info.max_design_voltage_dV = 4500;
  b.info.min_cell_voltage_mV = 2700;
  b.info.max_cell_voltage_mV = 4300;
  b.info.max_cell_voltage_deviation_mV = 500;
  b.status.voltage_dV = 4100;
  b.status.cell_min_voltage_mV = 3700;
  b.status.cell_max_voltage_mV = 3750;
  b.status.temperature_min_dC = 250;
  b.status.temperature_max_dC = 280;
  b.status.reported_soc = 5000;
  b.status.real_soc = 5000;
  b.status.active_power_W = 0;

  // The state a non-CAN battery is permanently in: the CAN counter is never refreshed.
  b.status.CAN_battery_still_alive = 0;
  b.status.max_charge_power_W = 5000;
  b.status.max_discharge_power_W = 5000;

  for (int cycle = 0; cycle < 3; cycle++) {
    update_machineryprotection();
  }

  EXPECT_EQ(get_event_pointer(EVENT_CAN_BATTERY_MISSING)->state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);

  battery = saved;
}

// The guard above must not disable the check for batteries that do use CAN.
TEST_F(BatteryAliveTest, CanBatteryIsStillPolicedByTheCanWatchdog) {
  ASSERT_EQ(battery->interface_type(), BatteryInterfaceType::Can);
  datalayer.battery.status.CAN_battery_still_alive = 0;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_BATTERY_MISSING)->state, EVENT_STATE_ACTIVE);
}
