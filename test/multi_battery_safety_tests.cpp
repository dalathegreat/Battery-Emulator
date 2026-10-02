#include <gtest/gtest.h>

#include <array>
#include <string>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* Packs 2 and 3 are held to the same safety layer as pack 1. Every joined pack takes its share of
   whatever current the inverter pushes or pulls, so a cell at its limit in any one pack has to
   stop the whole installation - which the limits do once they reach the aggregate the inverter
   reads. Until v8.2.0 a pack 2 cell excursion faulted the system; after it, packs 2 and 3 only
   raised a warning and kept charging and discharging. */

namespace {

class MultiBatterySafetyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and events and deleted the packs.
    init_events();  // Event levels are only assigned here, and the critical events have to fault
    battery = new TestFakeBattery();
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    battery3 = new TestFakeBattery(&datalayer.battery3, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 3;
    datalayer.system.info.CPU_free_heap = 200000;  // Keep the low-heap check quiet
    emulator_pause_request_ON = false;             // A global other suites leave behind
    // The detection latches are globals with no reset inside safety.cpp
    battery_detected = true;
    battery2_detected = true;
    battery3_detected = true;
    datalayer.system.status.battery2_allowed_contactor_closing = true;
    datalayer.system.status.battery3_allowed_contactor_closing = true;

    for (DATALAYER_BATTERY_TYPE* pack : packs()) {
      pack->status.voltage_dV = 3700;
      pack->status.real_soc = 5000;
      pack->status.cell_max_voltage_mV = 3800;
      pack->status.cell_min_voltage_mV = 3750;
    }
    datalayer.aggregate.reported_soc = 5000;

    // The charge latches are statics inside safety.cpp: one healthy pass releases any that an
    // earlier test left set, so these tests do not depend on the order they run in
    run_cycle();
    reset_all_events();
  }

  static std::array<DATALAYER_BATTERY_TYPE*, 3> packs() {
    return {&datalayer.battery, &datalayer.battery2, &datalayer.battery3};
  }

  // One pass of the core loop: the drivers publish fresh BMS limits, then the safety layer runs
  // and the aggregate folds the joined packs into what the inverter is told
  static void run_cycle() {
    const bool detected[3] = {battery_detected, battery2_detected, battery3_detected};
    uint8_t i = 0;
    for (DATALAYER_BATTERY_TYPE* pack : packs()) {
      pack->status.max_charge_power_W = 5000;
      pack->status.max_discharge_power_W = 5000;
      if (detected[i++]) {
        pack->status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Still talking
      }
    }
    update_machineryprotection();
    update_aggregate_limits();
  }

  static EVENTS_STATE_TYPE state(EVENTS_ENUM_TYPE event) { return get_event_pointer(event)->state; }
};

}  // namespace

TEST_F(MultiBatterySafetyTest, CellOvervoltageOnPack2StopsChargingTheInstallation) {
  datalayer.battery2.status.cell_max_voltage_mV = datalayer.battery2.info.max_cell_voltage_mV;

  run_cycle();

  EXPECT_EQ(state(EVENT_CELL_OVER_VOLTAGE_BAT2), EVENT_STATE_ACTIVE);
  EXPECT_EQ(state(EVENT_CELL_OVER_VOLTAGE), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  // Only charging is blocked, and it is a warning: the system keeps running
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 5000u);
  EXPECT_EQ(datalayer.system.status.system_status, ACTIVE);
}

TEST_F(MultiBatterySafetyTest, CellUndervoltageOnPack3StopsDischargingTheInstallation) {
  datalayer.battery3.status.cell_min_voltage_mV = datalayer.battery3.info.min_cell_voltage_mV;

  run_cycle();

  EXPECT_EQ(state(EVENT_CELL_UNDER_VOLTAGE_BAT3), EVENT_STATE_ACTIVE);
  EXPECT_EQ(state(EVENT_CELL_UNDER_VOLTAGE), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery3.status.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.system.status.system_status, ACTIVE);
}

// Same hysteresis as pack 1: charging resumes only once the highest cell is 20 mV below the
// ceiling, so it does not chatter at the knee
TEST_F(MultiBatterySafetyTest, Pack2ChargeBlockReleasesOnlyBelowTheHysteresis) {
  const uint16_t ceiling_mV = datalayer.battery2.info.max_cell_voltage_mV;

  datalayer.battery2.status.cell_max_voltage_mV = ceiling_mV;
  run_cycle();
  ASSERT_EQ(datalayer.aggregate.max_charge_power_W, 0u);

  datalayer.battery2.status.cell_max_voltage_mV = ceiling_mV - 10;
  run_cycle();
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);

  datalayer.battery2.status.cell_max_voltage_mV = ceiling_mV - 21;
  run_cycle();
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 5000u);
}

// Each pack has its own latch: one pack dropping back below its ceiling must not release the
// block another pack still needs
TEST_F(MultiBatterySafetyTest, OnePackReleasingDoesNotReleaseAnother) {
  datalayer.battery2.status.cell_max_voltage_mV = datalayer.battery2.info.max_cell_voltage_mV;
  datalayer.battery3.status.cell_max_voltage_mV = datalayer.battery3.info.max_cell_voltage_mV;
  run_cycle();

  datalayer.battery3.status.cell_max_voltage_mV = 3800;
  datalayer.battery2.status.cell_max_voltage_mV = datalayer.battery2.info.max_cell_voltage_mV - 10;
  run_cycle();

  EXPECT_EQ(datalayer.battery3.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
}

TEST_F(MultiBatterySafetyTest, CriticalCellOvervoltageOnPack2FaultsTheSystem) {
  datalayer.battery2.status.cell_max_voltage_mV = datalayer.battery2.info.max_cell_voltage_mV + 100;

  run_cycle();
  run_cycle();  // The fault zeroes pack 1 from the next pass on

  EXPECT_EQ(state(EVENT_CELL_CRITICAL_OVER_VOLTAGE_BAT2), EVENT_STATE_ACTIVE);
  EXPECT_EQ(state(EVENT_CELL_CRITICAL_OVER_VOLTAGE), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.system.status.system_status, FAULT);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
}

TEST_F(MultiBatterySafetyTest, CriticalCellUndervoltageOnPack3FaultsTheSystem) {
  datalayer.battery3.status.cell_min_voltage_mV = datalayer.battery3.info.min_cell_voltage_mV - 100;

  run_cycle();
  run_cycle();

  EXPECT_EQ(state(EVENT_CELL_CRITICAL_UNDER_VOLTAGE_BAT3), EVENT_STATE_ACTIVE);
  EXPECT_EQ(state(EVENT_CELL_CRITICAL_UNDER_VOLTAGE), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.system.status.system_status, FAULT);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
}

// A pack that has not spoken yet still holds its 3700 mV power-on default, which is above an LFP
// ceiling. That is not a measurement and must not trip anything; its first real reading is.
TEST_F(MultiBatterySafetyTest, SilentPackDefaultsAreNotCheckedButItsReadingsAre) {
  battery2_detected = false;
  datalayer.battery2.info.max_cell_voltage_mV = 3650;
  datalayer.battery2.status.cell_max_voltage_mV = 3700;
  datalayer.battery2.status.cell_min_voltage_mV = 3700;

  run_cycle();

  EXPECT_EQ(state(EVENT_CELL_OVER_VOLTAGE_BAT2), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 5000u);

  // The pack speaks, and its reading is at the ceiling
  datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  datalayer.battery2.status.cell_max_voltage_mV = 3650;
  datalayer.battery2.status.cell_min_voltage_mV = 3600;
  run_cycle();

  EXPECT_TRUE(battery2_detected);
  EXPECT_EQ(state(EVENT_CELL_OVER_VOLTAGE_BAT2), EVENT_STATE_ACTIVE);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 0u);
}

// The pack is named in the event text, and pack 1 keeps the event name it always had
TEST_F(MultiBatterySafetyTest, CellEventsNameThePack) {
  datalayer.battery.status.cell_max_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  datalayer.battery3.status.cell_max_voltage_mV = datalayer.battery3.info.max_cell_voltage_mV;

  run_cycle();

  EXPECT_EQ(state(EVENT_CELL_OVER_VOLTAGE), EVENT_STATE_ACTIVE);
  EXPECT_STREQ(get_event_enum_string(EVENT_CELL_OVER_VOLTAGE), "CELL_OVER_VOLTAGE");
  const std::string pack3_message = get_event_message_string(EVENT_CELL_OVER_VOLTAGE_BAT3).c_str();
  EXPECT_NE(pack3_message.find("(Battery 3)"), std::string::npos);
}
