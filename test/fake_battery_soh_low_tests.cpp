#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* The fake battery's More Battery Info page can hold EVENT_SOH_LOW off, so any SOH can be tried
   without faulting. The flag is runtime only and must never reach a real battery. */

namespace {

class FakeBatterySohLowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and events and deleted the packs.
    init_events();  // Event levels are only assigned here, and EVENT_SOH_LOW has to fault
    user_selected_battery_type = BatteryType::TestFake;
    battery = new TestFakeBattery();
    battery->setup();
    datalayer.system.info.CPU_free_heap = 200000;  // Keep the low-heap check quiet
    emulator_pause_request_ON = false;             // A global other suites leave behind
    battery_detected = true;
    datalayer.battery.status.real_soc = 5000;
    datalayer.aggregate.reported_soc = 5000;
  }

  void TearDown() override {
    user_selected_battery_type = BatteryType::None;
    battery2_detected = false;  // A detection latch, global in safety.cpp
  }

  // The core loop's order: the packs are rolled up into the aggregate, whose SOH is what
  // EVENT_SOH_LOW judges, and then the safety layer runs
  static void run_cycle() {
    datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    scale_pack_values(datalayer.battery);
    if (battery2) {
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      scale_pack_values(datalayer.battery2);
    }
    update_aggregate_values();
    update_machineryprotection(0);
  }

  static EVENTS_STATE_TYPE soh_low_state() { return get_event_pointer(EVENT_SOH_LOW)->state; }
};

}  // namespace

TEST_F(FakeBatterySohLowTest, LowSohFaultsByDefault) {
  battery->set_fake_soh(10.0f);
  run_cycle();
  EXPECT_EQ(soh_low_state(), EVENT_STATE_ACTIVE);
}

TEST_F(FakeBatterySohLowTest, CheckboxHoldsTheEventOffAndClearsAnActiveOne) {
  battery->set_fake_soh(10.0f);
  run_cycle();
  ASSERT_EQ(soh_low_state(), EVENT_STATE_ACTIVE);

  datalayer.battery_settings.user_disables_soh_low_event = true;
  run_cycle();
  EXPECT_NE(soh_low_state(), EVENT_STATE_ACTIVE);

  battery->set_fake_soh(0.0f);
  run_cycle();
  EXPECT_NE(soh_low_state(), EVENT_STATE_ACTIVE);
}

TEST_F(FakeBatterySohLowTest, UntickingRestoresTheEvent) {
  datalayer.battery_settings.user_disables_soh_low_event = true;
  battery->set_fake_soh(10.0f);
  run_cycle();
  ASSERT_NE(soh_low_state(), EVENT_STATE_ACTIVE);

  datalayer.battery_settings.user_disables_soh_low_event = false;
  run_cycle();
  EXPECT_EQ(soh_low_state(), EVENT_STATE_ACTIVE);
}

TEST_F(FakeBatterySohLowTest, FlagIsIgnoredForARealBattery) {
  user_selected_battery_type = BatteryType::NissanLeaf;
  datalayer.battery_settings.user_disables_soh_low_event = true;
  datalayer.battery.status.soh_pptt = 1000;
  run_cycle();
  EXPECT_EQ(soh_low_state(), EVENT_STATE_ACTIVE);
}

// EVENT_SOH_LOW judges the weakest pack, so a worn fake pack 2 raises it as well, and the one
// installation-wide checkbox holds it off for every pack
TEST_F(FakeBatterySohLowTest, CheckboxAlsoHoldsOffALowSohOnPack2) {
  battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
  battery2->battery_index = 2;
  battery2->setup();
  datalayer.system.info.configured_batteries = 2;
  battery2_detected = true;
  battery2->set_fake_soh(10.0f);  // pack 1 stays on its 99.00%

  run_cycle();
  ASSERT_EQ(soh_low_state(), EVENT_STATE_ACTIVE);

  datalayer.battery_settings.user_disables_soh_low_event = true;
  run_cycle();
  EXPECT_NE(soh_low_state(), EVENT_STATE_ACTIVE);
}

TEST_F(FakeBatterySohLowTest, MoreBatteryInfoShowsVoltageRangeAndCheckboxOnEveryPack) {
  using ::testing::HasSubstr;
  using ::testing::Not;

  std::string html = static_cast<TestFakeBattery*>(battery)->get_status_html().str();
  EXPECT_THAT(html, HasSubstr("Max battery voltage: 404.0 V"));
  EXPECT_THAT(html, HasSubstr("Min battery voltage: 245.0 V"));
  EXPECT_THAT(html, HasSubstr("SOHLowOff"));
  EXPECT_THAT(html, Not(HasSubstr(" checked>")));

  datalayer.battery_settings.user_disables_soh_low_event = true;
  html = static_cast<TestFakeBattery*>(battery)->get_status_html().str();
  EXPECT_THAT(html, HasSubstr(" checked>"));

  TestFakeBattery pack2(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
  pack2.battery_index = 2;
  pack2.setup();
  html = pack2.get_status_html().str();
  EXPECT_THAT(html, HasSubstr("Max battery voltage: 404.0 V"));
  EXPECT_THAT(html, HasSubstr("SOHLowOff"));
  EXPECT_THAT(html, HasSubstr(" checked>"));  // the same flag battery 1's tab ticked
}
