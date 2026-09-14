#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"

/* datalayer.battery, .battery2 and .battery3 hold one pack each. datalayer.aggregate is the
   installation: what the inverter protocols read and what the combined card on the main page
   shows. These tests pin the three rules that are easy to break by accident - a single pack
   must come out byte for byte as it went in, a configured pack must count towards the energy
   whether or not it has joined the DC link yet, and a pack that has never spoken must not
   contribute its power-on defaults to the extremes. */

namespace {

class BatteryAggregateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and deleted the packs.
    datalayer.battery.settings.soc_scaling_active = false;
    datalayer.system.info.configured_batteries = 1;
    battery2_detected = false;
    battery3_detected = false;
  }

  // Only the pointer matters to the aggregate: it decides whether the pack exists at all.
  void add_second_pack() {
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 2;
  }

  void add_third_pack() {
    battery3 = new TestFakeBattery(&datalayer.battery3, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 3;
  }

  static void scale_all() {
    scale_pack_values(datalayer.battery);
    if (battery2) {
      scale_pack_values(datalayer.battery2);
    }
    if (battery3) {
      scale_pack_values(datalayer.battery3);
    }
  }
};

// A one battery system must come out of the aggregate exactly as it went in. This is what lets
// every inverter protocol read datalayer.aggregate unconditionally.
TEST_F(BatteryAggregateTest, SinglePackIsCopiedThrough) {
  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery.status.remaining_capacity_Wh = 15000;
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.reported_current_dA = -120;
  datalayer.battery.status.soh_pptt = 9500;
  datalayer.battery.status.cell_max_voltage_mV = 3900;
  datalayer.battery.status.cell_min_voltage_mV = 3850;
  datalayer.battery.status.temperature_max_dC = 250;
  datalayer.battery.status.temperature_min_dC = 200;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.total_capacity_Wh, 30000u);
  EXPECT_EQ(datalayer.aggregate.remaining_capacity_Wh, 15000u);
  EXPECT_EQ(datalayer.aggregate.reported_total_capacity_Wh, 30000u);
  EXPECT_EQ(datalayer.aggregate.real_soc, 5000);
  EXPECT_EQ(datalayer.aggregate.reported_soc, 5000);
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 3700);
  EXPECT_EQ(datalayer.aggregate.current_dA, -120);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9500);
  EXPECT_EQ(datalayer.aggregate.cell_max_voltage_mV, 3900);
  EXPECT_EQ(datalayer.aggregate.cell_min_voltage_mV, 3850);
  EXPECT_EQ(datalayer.aggregate.temperature_max_dC, 250);
  EXPECT_EQ(datalayer.aggregate.temperature_min_dC, 200);
}

// Energy adds up across the packs, and the per-pack structs are left holding their own numbers.
TEST_F(BatteryAggregateTest, EnergySumsAcrossPacksWithoutTouchingThem) {
  add_second_pack();
  add_third_pack();
  battery2_detected = true;
  battery3_detected = true;

  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery.status.remaining_capacity_Wh = 15000;
  datalayer.battery2.info.total_capacity_Wh = 20000;
  datalayer.battery2.status.remaining_capacity_Wh = 10000;
  datalayer.battery3.info.total_capacity_Wh = 10000;
  datalayer.battery3.status.remaining_capacity_Wh = 5000;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.total_capacity_Wh, 60000u);
  EXPECT_EQ(datalayer.aggregate.remaining_capacity_Wh, 30000u);
  // Each card still reads its own pack
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 30000u);
  EXPECT_EQ(datalayer.battery2.info.total_capacity_Wh, 20000u);
  EXPECT_EQ(datalayer.battery3.info.total_capacity_Wh, 10000u);
}

// Every pack scales into its own reported_ fields, instead of pack 2 and 3 carrying pack 1's.
TEST_F(BatteryAggregateTest, EachPackScalesItsOwnCapacity) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.settings.soc_scaling_active = true;
  datalayer.battery.settings.min_percentage = 1000;  // 10.00%
  datalayer.battery.settings.max_percentage = 9000;  // 90.00%

  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery2.info.total_capacity_Wh = 10000;
  datalayer.battery2.status.real_soc = 5000;

  scale_all();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 24000u);  // 30000 * 80%
  EXPECT_EQ(datalayer.battery2.info.reported_total_capacity_Wh, 8000u);  // 10000 * 80%
  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000);                // midpoint of the window
  EXPECT_EQ(datalayer.battery2.status.reported_soc, 5000);

  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.reported_total_capacity_Wh, 32000u);
}

// A pack that is configured but has not joined the link yet still counts towards the energy,
// so the inverter's picture of the installation does not jump when the contactors close.
TEST_F(BatteryAggregateTest, NotYetJoinedPackStillCountsForEnergy) {
  add_second_pack();
  battery2_detected = true;
  datalayer.system.status.battery2_allowed_contactor_closing = false;

  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery2.info.total_capacity_Wh = 30000;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.total_capacity_Wh, 60000u);
}

// ...but it does not get to hand over the SOC until it has joined.
TEST_F(BatteryAggregateTest, SocHandoverNeedsAJoinedPack) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery2.status.real_soc = 10000;  // full

  datalayer.system.status.battery2_allowed_contactor_closing = false;
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.reported_soc, 5000);

  datalayer.system.status.battery2_allowed_contactor_closing = true;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.reported_soc, 10000);
}

// A configured pack that has never been seen on the bus holds its power-on defaults. Those are
// not measurements and must not reach the aggregate extremes.
TEST_F(BatteryAggregateTest, SilentPackDoesNotDragTheExtremes) {
  add_second_pack();
  battery2_detected = false;  // never spoke

  datalayer.battery.status.cell_max_voltage_mV = 4100;
  datalayer.battery.status.cell_min_voltage_mV = 4050;
  datalayer.battery.status.temperature_max_dC = 250;
  datalayer.battery.status.temperature_min_dC = 200;
  datalayer.battery.status.soh_pptt = 9500;
  // datalayer.battery2 keeps 3700 mV cells, 0 dC and 9900 pptt

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.cell_min_voltage_mV, 4050);
  EXPECT_EQ(datalayer.aggregate.temperature_min_dC, 200);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9500);

  battery2_detected = true;  // now it is talking, and it really is colder
  datalayer.battery2.status.temperature_min_dC = 50;
  datalayer.battery2.status.cell_min_voltage_mV = 3950;
  datalayer.battery2.status.soh_pptt = 9000;
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.cell_min_voltage_mV, 3950);
  EXPECT_EQ(datalayer.aggregate.temperature_min_dC, 50);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9000);
}

// The limits the inverter is told about are the weakest pack's, and a pack 2 fault that the
// safety layer zeroes reaches the inverter - which it did not when the cap ran before safety.
TEST_F(BatteryAggregateTest, LimitsAreCappedToTheWeakestPack) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.settings.max_user_set_charge_dA = 3000;
  datalayer.battery.settings.max_user_set_discharge_dA = 3000;

  datalayer.battery.status.max_charge_power_W = 10000;
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery2.status.max_charge_power_W = 6000;
  datalayer.battery2.status.max_discharge_power_W = 8000;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 6000u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 8000u);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 162);     // 6000 W at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 216);  // 8000 W at 370.0 V

  // Pack 2 faults and the safety layer zeroes it. Pack 1 is untouched and still happy.
  datalayer.battery2.status.max_charge_power_W = 0;
  datalayer.battery2.status.max_discharge_power_W = 0;
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 0);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 10000u);  // pack 1's own card is honest
}

// The user's current ceiling still applies on top of the power derived value.
TEST_F(BatteryAggregateTest, UserCurrentLimitCapsTheAggregate) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 10000;  // would be 270 dA
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery.settings.max_user_set_charge_dA = 100;
  datalayer.battery.settings.max_user_set_discharge_dA = 3000;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 100);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 270);
}

}  // namespace
