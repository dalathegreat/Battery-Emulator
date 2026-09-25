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
    datalayer.battery_settings.soc_scaling_active = false;
    datalayer.system.info.configured_batteries = 1;
    battery2_detected = false;
    battery3_detected = false;
  }

  // Only the pointer matters to the aggregate: it decides whether the pack exists at all.
  // A pack in a running installation is on the DC link. Tests that care about the
  // configured-but-not-joined case clear the flag themselves.
  void add_second_pack() {
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 2;
    datalayer.system.status.battery2_allowed_contactor_closing = true;
  }

  void add_third_pack() {
    battery3 = new TestFakeBattery(&datalayer.battery3, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 3;
    datalayer.system.status.battery3_allowed_contactor_closing = true;
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

// The SOC window belongs to the installation. With more than one pack the per-pack structs
// report what they would with scaling switched off - which is what MQTT and ESP-NOW carry -
// and the window is applied once, to the aggregate.
TEST_F(BatteryAggregateTest, PacksStayUnscaledAndOnlyTheAggregateGetsTheWindow) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery_settings.soc_scaling_active = true;
  datalayer.battery_settings.min_percentage = 1200;  // 12.00%
  datalayer.battery_settings.max_percentage = 8200;  // 82.00%

  datalayer.battery.info.total_capacity_Wh = 21800;
  datalayer.battery.status.real_soc = 4100;
  datalayer.battery.status.remaining_capacity_Wh = 8938;
  datalayer.battery2.info.total_capacity_Wh = 17900;
  datalayer.battery2.status.real_soc = 3930;
  datalayer.battery2.status.remaining_capacity_Wh = 7034;

  scale_all();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 4100);
  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 21800u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 8938u);
  EXPECT_EQ(datalayer.battery2.status.reported_soc, 3930);
  EXPECT_EQ(datalayer.battery2.info.reported_total_capacity_Wh, 17900u);

  update_aggregate_values();

  // 39.7 kWh across a 70 point window
  EXPECT_EQ(datalayer.aggregate.total_capacity_Wh, 39700u);
  EXPECT_EQ(datalayer.aggregate.reported_total_capacity_Wh, 27790u);
  // The reported energy and the reported SOC now agree: 27790 * reported_soc / 10000
  EXPECT_EQ(datalayer.aggregate.reported_remaining_capacity_Wh,
            (uint32_t)(27790ull * datalayer.aggregate.reported_soc) / 10000u);
}

// A single battery is the installation, so it keeps its scaled reported_ fields and MQTT,
// ESP-NOW and the display see exactly what they always did.
TEST_F(BatteryAggregateTest, SingleBatteryKeepsItsScaledFields) {
  datalayer.battery_settings.soc_scaling_active = true;
  datalayer.battery_settings.min_percentage = 1000;
  datalayer.battery_settings.max_percentage = 9000;
  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery.status.real_soc = 5000;

  scale_all();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000);  // midpoint of the window
  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 24000u);
}

// SOC follows the emptiest pack: that is what protects the weakest one on discharge.
TEST_F(BatteryAggregateTest, SocFollowsTheEmptiestPack) {
  add_second_pack();
  add_third_pack();
  battery2_detected = true;
  battery3_detected = true;

  datalayer.battery.status.real_soc = 4100;
  datalayer.battery2.status.real_soc = 3930;
  datalayer.battery3.status.real_soc = 5500;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.real_soc, 3930);
}

// Once the fullest pack climbs into the top tenth the reported SOC blends towards it, so the
// installation arrives at 100% smoothly rather than stepping there the moment one pack tops out.
TEST_F(BatteryAggregateTest, SocBlendsTowardsTheFullestPackNearTheTop) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.real_soc = 5000;  // half empty

  // Below the blend window the emptiest pack still has it alone
  datalayer.battery2.status.real_soc = 8900;
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 5000);

  // Halfway into the window, halfway across the spread
  datalayer.battery2.status.real_soc = 9500;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 7250);  // 5000 + (9500 - 5000) * 500 / 1000

  // A full pack hands over completely, so charging actually stops
  datalayer.battery2.status.real_soc = 10000;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 10000);
}

// The installation may only be charged as high as the lowest ceiling any pack reports, and
// discharged as low as the highest floor.
TEST_F(BatteryAggregateTest, DesignVoltagesTakeTheTighterBound) {
  add_second_pack();
  battery2_detected = true;

  datalayer.battery.info.max_design_voltage_dV = 4030;
  datalayer.battery.info.min_design_voltage_dV = 3100;
  datalayer.battery2.info.max_design_voltage_dV = 3950;
  datalayer.battery2.info.min_design_voltage_dV = 3250;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.max_design_voltage_dV, 3950);
  EXPECT_EQ(datalayer.aggregate.min_design_voltage_dV, 3250);
}

// An integration that has not decoded its design voltages yet must not set the limit for
// everyone else.
TEST_F(BatteryAggregateTest, UndecodedDesignVoltagesAreIgnored) {
  add_second_pack();
  battery2_detected = true;

  datalayer.battery.info.max_design_voltage_dV = 4030;
  datalayer.battery.info.min_design_voltage_dV = 3100;
  datalayer.battery2.info.max_design_voltage_dV = 0;
  datalayer.battery2.info.min_design_voltage_dV = 0;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.max_design_voltage_dV, 4030);
  EXPECT_EQ(datalayer.aggregate.min_design_voltage_dV, 3100);
}

// Power divides once at the end, so the fractional Volt survives: the obvious
// current_dA * (voltage_dV / 100) spends 386.0 V as 380 V and loses about 1.5%.
TEST_F(BatteryAggregateTest, PowerKeepsTheFractionalVolts) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.voltage_dV = 3860;
  datalayer.battery.status.current_dA = 5;
  datalayer.battery2.status.voltage_dV = 3860;
  datalayer.battery2.status.current_dA = 25;
  datalayer.battery.status.reported_current_dA = 30;  // Software.cpp sums these

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.current_dA, 30);
  EXPECT_EQ(datalayer.aggregate.active_power_W, 1158);  // 386.0 V x 3.0 A, not 380 V x 3.0 A
}

// State of health follows the weakest pack, like every other limit here.
TEST_F(BatteryAggregateTest, SohIsTheWeakestPack) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.soh_pptt = 7560;
  datalayer.battery2.status.soh_pptt = 6209;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.soh_pptt, 6209);
}

// A pack that has not decoded a state of health reports zero, which must not become the
// installation's.
TEST_F(BatteryAggregateTest, UndecodedSohIsIgnored) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.soh_pptt = 7560;
  datalayer.battery2.status.soh_pptt = 0;

  scale_all();
  update_aggregate_values();

  EXPECT_EQ(datalayer.aggregate.soh_pptt, 7560);
}

// A 19.0 A ceiling has to survive the trip out through Watts and back.
TEST_F(BatteryAggregateTest, UserCurrentLimitSurvivesTheRoundTrip) {
  datalayer.battery.status.voltage_dV = 3525;
  datalayer.battery_settings.max_user_set_charge_dA = 190;
  datalayer.battery_settings.max_user_set_discharge_dA = 190;
  // What filter_inverter_limits() derives from a 19.0 A ceiling at 352.5 V
  datalayer.battery.status.max_charge_power_W = 6697;
  datalayer.battery.status.max_discharge_power_W = 6697;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 190);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 190);
}

// A pack that has not joined the DC link yet still counts towards the energy, so the inverter's
// picture does not jump when the contactors close.
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

// ...but it does not get to move the SOC. It is talking, and its SOC is real, but it is not the
// SOC of anything the inverter can charge or discharge: an empty detached pack would read the
// whole installation empty and stop discharge on behalf of a battery that is not connected.
TEST_F(BatteryAggregateTest, NotYetJoinedPackDoesNotDriveSoc) {
  add_second_pack();
  battery2_detected = true;
  datalayer.system.status.battery2_allowed_contactor_closing = false;

  datalayer.battery.info.total_capacity_Wh = 30000;
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery2.info.total_capacity_Wh = 30000;
  datalayer.battery2.status.real_soc = 0;  // detached and empty

  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 5000);

  // A detached full pack must not lift the blend either
  datalayer.battery2.status.real_soc = 10000;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 5000);

  // Once it is on the link it counts, both ways
  datalayer.system.status.battery2_allowed_contactor_closing = true;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.real_soc, 10000);  // fully blended at 100%
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
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9000);  // now the weaker of the two
}

// The limits the inverter is told about are the weakest pack's, and a pack 2 fault that the
// safety layer zeroes reaches the inverter - which it did not when the cap ran before safety.
TEST_F(BatteryAggregateTest, LimitsAreCappedToTheWeakestPack) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery_settings.max_user_set_charge_dA = 3000;
  datalayer.battery_settings.max_user_set_discharge_dA = 3000;

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
  datalayer.battery_settings.max_user_set_charge_dA = 100;
  datalayer.battery_settings.max_user_set_discharge_dA = 3000;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 100);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 270);
}

// The per-pack card shows what that pack's BMS asked for. The safety layer and the filters
// rewrite max_charge_power_W in place, so without the snapshot pack 1's card would show the
// system's decision while pack 2's showed the raw BMS figure - which is what it used to do.
TEST_F(BatteryAggregateTest, BmsLimitsSurviveTheSafetyLayer) {
  add_second_pack();
  battery2_detected = true;

  datalayer.battery.status.max_charge_power_W = 70000;
  datalayer.battery.status.max_discharge_power_W = 110000;
  datalayer.battery2.status.max_charge_power_W = 70000;
  datalayer.battery2.status.max_discharge_power_W = 110000;

  snapshot_bms_limits(datalayer.battery);
  snapshot_bms_limits(datalayer.battery2);

  // Everything downstream now has its way with pack 1
  datalayer.battery.status.max_charge_power_W = 6700;
  datalayer.battery.status.max_discharge_power_W = 6700;

  EXPECT_EQ(datalayer.battery.status.bms_max_charge_power_W, 70000u);
  EXPECT_EQ(datalayer.battery.status.bms_max_discharge_power_W, 110000u);
  EXPECT_EQ(datalayer.battery2.status.bms_max_charge_power_W, 70000u);
}

// A full pack's BMS allows no more charge but still allows discharge. That zero is a limit, not
// a missing value, and the snapshot has to keep it as one.
TEST_F(BatteryAggregateTest, FullPackSnapshotKeepsZeroChargeLimit) {
  datalayer.battery.status.max_charge_power_W = 0;
  datalayer.battery.status.max_discharge_power_W = 10000;
  snapshot_bms_limits(datalayer.battery);
  EXPECT_EQ(datalayer.battery.status.bms_max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.status.bms_max_discharge_power_W, 10000u);
}

// A LEAF clears soh_available until it has decoded one. That pack must not drag the
// installation's health to its default, and when no pack has decoded one the aggregate must say
// so rather than present pack 1's default as a reading - while still feeding the inverter.
TEST_F(BatteryAggregateTest, SohIgnoresPacksThatHaveNotDecodedOne) {
  add_second_pack();
  battery2_detected = true;

  datalayer.battery.status.soh_available = false;  // pack 1 still waiting
  datalayer.battery.status.soh_pptt = 9900;        // its safe default
  datalayer.battery2.status.soh_available = true;
  datalayer.battery2.status.soh_pptt = 6209;

  scale_all();
  update_aggregate_values();
  EXPECT_TRUE(datalayer.aggregate.soh_available);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 6209);  // pack 1's default is not a reading

  datalayer.battery2.status.soh_available = false;  // nobody has one now
  update_aggregate_values();
  EXPECT_FALSE(datalayer.aggregate.soh_available);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9900);  // safe default still reaches the inverter
}

TEST_F(BatteryAggregateTest, SinglePackSohAvailabilityPassesThrough) {
  datalayer.battery.status.soh_available = false;
  datalayer.battery.status.soh_pptt = 9900;
  scale_all();
  update_aggregate_values();
  EXPECT_FALSE(datalayer.aggregate.soh_available);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 9900);

  datalayer.battery.status.soh_available = true;
  datalayer.battery.status.soh_pptt = 7560;
  update_aggregate_values();
  EXPECT_TRUE(datalayer.aggregate.soh_available);
  EXPECT_EQ(datalayer.aggregate.soh_pptt, 7560);
}

// Every pack starts at 0 V until its integration has decoded one. The inverter must never see
// that 0: until pack 1 has a reading it keeps getting the 370.0 V the packs used to start on.
TEST_F(BatteryAggregateTest, UndecodedVoltageSendsPlaceholder) {
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 0);  // the pack's own power-on value
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 3700);

  datalayer.battery.status.voltage_dV = 3525;  // decoded: passed through as is
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 3525);
}

// Some inverters (Solax) fault on a startup voltage outside what the pack can reach, and an LFP
// pack can top out below 370.0 V. LFP gets 330.0 V, even while the integration still has the
// deliberately wide start-up window the BYD Atto 3 uses until it has counted its cells.
TEST_F(BatteryAggregateTest, UndecodedLfpVoltageSends330V) {
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.min_design_voltage_dV = 2000;
  datalayer.battery.info.max_design_voltage_dV = 6500;
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 3300);

  // A pack whose range ends at 350.0 V, as on the smaller BYD LFP packs
  datalayer.battery.info.min_design_voltage_dV = 2500;
  datalayer.battery.info.max_design_voltage_dV = 3500;
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 3300);
}

// An LV LFP pack cannot take 330.0 V either: it falls back to the middle of its window too
TEST_F(BatteryAggregateTest, UndecodedLvLfpVoltageSendsMiddleOfDesignWindow) {
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.min_design_voltage_dV = 400;
  datalayer.battery.info.max_design_voltage_dV = 580;
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 490);
}

// An LV installation must not be told 370.0 V: it gets the middle of its design window instead
TEST_F(BatteryAggregateTest, UndecodedLvVoltageSendsMiddleOfDesignWindow) {
  datalayer.battery.info.min_design_voltage_dV = 400;
  datalayer.battery.info.max_design_voltage_dV = 580;
  scale_all();
  update_aggregate_values();
  EXPECT_EQ(datalayer.aggregate.voltage_dV, 490);
}

}  // namespace
