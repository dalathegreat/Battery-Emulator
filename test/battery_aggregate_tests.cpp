#include <gtest/gtest.h>

#include <initializer_list>

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
    emulator_pause_request_ON = false;  // A global other suites may leave set
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

  // Packs on the link at 370.0 V, each allowing the power given both ways, with no current cap in the way
  void packs_allowing(std::initializer_list<uint32_t> limits_W) {
    DATALAYER_BATTERY_TYPE* packs[3] = {&datalayer.battery, &datalayer.battery2, &datalayer.battery3};
    if (limits_W.size() > 1) {
      add_second_pack();
      battery2_detected = true;
    }
    if (limits_W.size() > 2) {
      add_third_pack();
      battery3_detected = true;
    }
    uint8_t i = 0;
    for (uint32_t limit_W : limits_W) {
      packs[i]->status.voltage_dV = 3700;
      packs[i]->status.max_discharge_power_W = limit_W;
      packs[i++]->status.max_charge_power_W = limit_W;
    }
    datalayer.battery_settings.max_user_set_charge_dA = 3000;
    datalayer.battery_settings.max_user_set_discharge_dA = 3000;
    update_aggregate_values();
  }

  // The inverter charging at total_W, split between the packs by the percentages given
  static void inverter_charges(uint32_t total_W, std::initializer_list<uint32_t> share_pct) {
    DATALAYER_BATTERY_TYPE* packs[3] = {&datalayer.battery, &datalayer.battery2, &datalayer.battery3};
    int32_t left_W = (int32_t)total_W;
    uint8_t i = 0;
    for (uint32_t pct : share_pct) {
      const bool last = (i + 1u == share_pct.size());
      packs[i]->status.active_power_W = last ? left_W : (int32_t)(total_W * pct / 100);
      left_W -= packs[i++]->status.active_power_W;
    }
  }

  // One pass of the limit chain with the filters left out: what the inverter is told
  static uint32_t told_W() {
    combine_pack_limits();
    update_aggregate_limits();
    return datalayer.aggregate.max_charge_power_W;
  }

  // The passes the inverter is given to follow a change, with the packs staying where they are
  static uint32_t settled_W() {
    for (uint8_t pass = 1; pass < PARALLEL_SETTLE_PASSES; pass++) {
      told_W();
    }
    return told_W();
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
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 6000u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 8000u);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 162);     // 6000 W at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 216);  // 8000 W at 370.0 V

  // Pack 2 faults and the safety layer zeroes it. Pack 1 is untouched and still happy.
  datalayer.battery2.status.max_charge_power_W = 0;
  datalayer.battery2.status.max_discharge_power_W = 0;
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 0);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 10000u);  // pack 1's own card is honest
}

// A pack held out by the voltage check is still talking, and its 0 W is real - but it is not a
// limit on anything the inverter can charge or discharge. Letting it through would stop the
// installation on behalf of a battery that is not connected to it, same reasoning as SOC above.
TEST_F(BatteryAggregateTest, NotYetJoinedPackDoesNotCapLimits) {
  add_second_pack();
  battery2_detected = true;
  datalayer.system.status.battery2_allowed_contactor_closing = false;
  datalayer.battery.status.voltage_dV = 3700;

  datalayer.battery.status.max_charge_power_W = 10000;
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery2.status.max_charge_power_W = 0;  // detached, reads 0 while held out
  datalayer.battery2.status.max_discharge_power_W = 0;

  update_aggregate_values();
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 10000u);  // pack 1 alone, not dragged to 0
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 10000u);

  // Once it joins, its real limits count again
  datalayer.system.status.battery2_allowed_contactor_closing = true;
  datalayer.battery2.status.max_charge_power_W = 6000;
  datalayer.battery2.status.max_discharge_power_W = 8000;
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 6000u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 8000u);
}

// Two alike packs each take half of what the inverter pushes, so holding the installation to the
// lowest pack limit left both at half of theirs. While the inverter charges at the limit, it climbs
// by the tightest pack's headroom, one step each time the inverter has had time to follow, and
// never past what the two allow together.
TEST_F(BatteryAggregateTest, ParallelChargeLimitRaisesInStepsUntilThePacksTakeTheirLimits) {
  packs_allowing({5000, 5000});
  EXPECT_EQ(told_W(), 5000u);  // Not charging yet: the lowest pack limit, as always

  inverter_charges(5000, {50, 50});  // Each pack takes 2500 W of its 5000 W
  for (uint8_t pass = 1; pass < PARALLEL_SETTLE_PASSES; pass++) {
    EXPECT_EQ(told_W(), 5000u);  // The session has to settle first
  }
  EXPECT_EQ(told_W(), 7500u);
  inverter_charges(7500, {50, 50});  // Followed at once, but the next step still waits its turn
  for (uint8_t pass = 1; pass < PARALLEL_SETTLE_PASSES; pass++) {
    EXPECT_EQ(told_W(), 7500u);
  }
  EXPECT_EQ(told_W(), 8750u);
  inverter_charges(8750, {50, 50});
  EXPECT_EQ(settled_W(), 9375u);

  for (int step = 0; step < 20; step++) {
    inverter_charges(datalayer.aggregate.max_charge_power_W, {50, 50});
    EXPECT_LE(datalayer.battery.status.active_power_W, 5000);
    EXPECT_LE(datalayer.battery2.status.active_power_W, 5000);
    settled_W();
  }
  EXPECT_GE(datalayer.aggregate.max_charge_power_W, 9900u);
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 10000u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 5000u);  // Discharge stays at the lowest pack limit
}

// The split follows the packs' internal resistance, so the pack with the lowest limit is not always
// the first to reach it. Here pack 2 allows more but takes 80% of the current: it ends up at its
// limit, and neither pack is ever pushed past its own on the way.
TEST_F(BatteryAggregateTest, ParallelChargeLimitFollowsThePackClosestToItsLimit) {
  packs_allowing({4000, 6000});
  told_W();
  for (int step = 0; step < 30; step++) {
    inverter_charges(datalayer.aggregate.max_charge_power_W, {20, 80});
    EXPECT_LE(datalayer.battery.status.active_power_W, 4000);
    EXPECT_LE(datalayer.battery2.status.active_power_W, 6000);
    settled_W();
  }
  // 7500 W puts pack 2 at its 6000 W, with pack 1 at 1500 W of its 4000 W
  EXPECT_GE(datalayer.aggregate.max_charge_power_W, 7400u);
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 7500u);
}

TEST_F(BatteryAggregateTest, ParallelChargeLimitWorksAcrossThreePacks) {
  packs_allowing({5000, 6000, 7000});
  told_W();
  for (int step = 0; step < 30; step++) {
    inverter_charges(datalayer.aggregate.max_charge_power_W, {40, 35, 25});
    EXPECT_LE(datalayer.battery.status.active_power_W, 5000);
    EXPECT_LE(datalayer.battery2.status.active_power_W, 6000);
    EXPECT_LE(datalayer.battery3.status.active_power_W, 7000);
    settled_W();
  }
  // 12500 W puts pack 1 at its 5000 W
  EXPECT_GE(datalayer.aggregate.max_charge_power_W, 12400u);
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 12500u);
}

// The split only says something about the limit while the inverter actually uses it. Short of
// sun, or held below by a current cap, the taper or the low pass filter, nothing is learned and
// nothing is raised - and nothing winds up behind the cap either.
TEST_F(BatteryAggregateTest, ParallelChargeLimitWaitsForTheInstallationToDrawIt) {
  packs_allowing({5000, 5000});
  told_W();
  for (int step = 0; step < 5; step++) {
    inverter_charges(3000, {50, 50});  // Not enough sun to reach the limit
    EXPECT_EQ(settled_W(), 5000u);
  }

  datalayer.battery_settings.max_user_set_charge_dA = 150;  // 5550 W at 370.0 V
  inverter_charges(5000, {50, 50});
  EXPECT_EQ(settled_W(), 5550u);  // Raised to 7500 W, held to 5550 W by the cap
  for (int step = 0; step < 5; step++) {
    inverter_charges(5550, {50, 50});
    EXPECT_EQ(settled_W(), 5550u);
  }
  EXPECT_EQ(datalayer.aggregate.parallel_charge_limit_W, 7500u);
}

// A pack that goes over its limit - the split moved, or its limit dropped - cuts the limit at once
// to the total at which it is back on its limit. Worked out from what flows, the cut comes out the
// same while the inverter has not followed yet, rather than deeper on every pass.
TEST_F(BatteryAggregateTest, ParallelChargeLimitCutsAtOnceWhenAPackGoesOver) {
  packs_allowing({5000, 5000});
  told_W();
  inverter_charges(5000, {50, 50});
  EXPECT_EQ(settled_W(), 7500u);
  inverter_charges(7500, {50, 50});
  EXPECT_EQ(settled_W(), 8750u);

  inverter_charges(8750, {60, 40});  // Pack 1 now takes 5250 W, over its 5000 W
  EXPECT_EQ(told_W(), 8333u);        // At once: 8750 W x 5000 W / 5250 W
  EXPECT_EQ(told_W(), 8333u);        // The inverter has not followed yet

  inverter_charges(8333, {60, 40});  // It has: pack 1 at 4999 W
  EXPECT_EQ(settled_W(), 8334u);
}

// A pack whose integration does not report its current looks like a pack taking nothing, which
// would read as all the headroom in the world. Nothing is raised until every joined pack shows a
// share, and a pack on its own on the link has nothing to share with.
TEST_F(BatteryAggregateTest, ParallelChargeLimitNeedsEveryJoinedPackCharging) {
  packs_allowing({5000, 5000});
  told_W();
  inverter_charges(5000, {100, 0});
  EXPECT_EQ(settled_W(), 5000u);

  datalayer.system.status.battery2_allowed_contactor_closing = false;
  inverter_charges(5000, {100});
  EXPECT_EQ(settled_W(), 5000u);
}

// A LEAF can flip its limit between two values every couple of seconds for half an hour before it
// settles. A raise counts only on the lowest limit a pack has reported lately, so the installation
// settles where that pack sits on its low limit instead of being raised on every high and pushed
// over on every low - and a limit that goes up and stays up counts once it has held.
TEST_F(BatteryAggregateTest, ParallelChargeLimitCountsOnTheLowestLimitLately) {
  packs_allowing({3000, 6000});
  told_W();
  for (int pass = 0; pass < 300; pass++) {
    datalayer.battery.status.max_charge_power_W = ((pass / 2) % 2) ? 4000 : 3000;  // Flips every 2 s
    inverter_charges(datalayer.aggregate.max_charge_power_W, {50, 50});            // Followed at once
    EXPECT_LE(datalayer.battery.status.active_power_W, 3000);
    told_W();
  }
  EXPECT_GE(datalayer.aggregate.max_charge_power_W, 5900u);  // Pack 1 close to its 3000 W
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 6000u);

  datalayer.battery.status.max_charge_power_W = 4000;  // Up, and staying up
  for (int pass = 0; pass < PARALLEL_LIMIT_HOLD_PASSES - 5; pass++) {
    inverter_charges(datalayer.aggregate.max_charge_power_W, {50, 50});
    told_W();
  }
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 6000u);  // Not yet counted on
  for (int pass = 0; pass < 120; pass++) {
    inverter_charges(datalayer.aggregate.max_charge_power_W, {50, 50});
    told_W();
  }
  EXPECT_GE(datalayer.aggregate.max_charge_power_W, 7800u);  // Pack 1 close to its 4000 W
  EXPECT_LE(datalayer.aggregate.max_charge_power_W, 8000u);
}

// Every charge session starts from the lowest pack limit again, and a pack that allows nothing
// stops the installation however far the limit had climbed.
TEST_F(BatteryAggregateTest, ParallelChargeLimitStartsOverWhenChargingStops) {
  packs_allowing({5000, 5000});
  told_W();
  inverter_charges(5000, {50, 50});
  EXPECT_EQ(settled_W(), 7500u);

  inverter_charges(0, {50, 50});  // Evening
  EXPECT_EQ(told_W(), 5000u);

  inverter_charges(5000, {50, 50});
  EXPECT_EQ(settled_W(), 7500u);
  datalayer.battery2.status.max_charge_power_W = 0;  // Full, or zeroed by the safety layer
  EXPECT_EQ(told_W(), 0u);
  inverter_charges(0, {50, 50});
  datalayer.battery2.status.max_charge_power_W = 5000;
  EXPECT_EQ(told_W(), 5000u);
}

// The user's current ceiling applies to the power as well as to the current derived from it.
TEST_F(BatteryAggregateTest, UserCurrentLimitCapsTheAggregate) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 10000;  // would be 270 dA
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery_settings.max_user_set_charge_dA = 100;
  datalayer.battery_settings.max_user_set_discharge_dA = 3000;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 100);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 3700u);  // 10.0 A at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 270);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 10000u);  // the BMS limit is the lower one
}

// A remote limit lands on the power as well as on the current. BYD-Modbus, VCU-CAN and FoxESS-EP
// hand their inverter the power, so a cap on the current alone never reached them - while the
// main page, reading the current, showed it as applied.
TEST_F(BatteryAggregateTest, RemoteLimitCapsThePowerToo) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 10000;
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery_settings.remote_settings_limit_charge = true;
  datalayer.battery_settings.max_remote_set_charge_dA = 50;
  datalayer.battery_settings.remote_settings_limit_discharge = true;
  datalayer.battery_settings.max_remote_set_discharge_dA = 0;  // Stop discharging altogether

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 1850u);  // 5.0 A at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 0);
}

// The setup it was reported on: two packs, the limit applies to the installation.
TEST_F(BatteryAggregateTest, RemoteLimitCapsTheInstallationWithTwoPacks) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery2.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 10000;
  datalayer.battery.status.max_discharge_power_W = 10000;
  datalayer.battery2.status.max_charge_power_W = 8000;
  datalayer.battery2.status.max_discharge_power_W = 9000;
  datalayer.battery_settings.remote_settings_limit_charge = true;
  datalayer.battery_settings.max_remote_set_charge_dA = 100;
  datalayer.battery_settings.remote_settings_limit_discharge = true;
  datalayer.battery_settings.max_remote_set_discharge_dA = 150;

  update_aggregate_values();
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 3700u);  // 10.0 A at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 100);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 5550u);  // 15.0 A at 370.0 V
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 150);
  // The packs keep their own numbers
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 10000u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 8000u);
}

// While it lasts, a remote limit replaces the user one rather than combining with it, so it may
// sit above it. Once it expires the user limit is back.
TEST_F(BatteryAggregateTest, RemoteLimitReplacesTheUserLimitUntilItExpires) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 20000;
  datalayer.battery.status.max_discharge_power_W = 20000;
  datalayer.battery_settings.max_user_set_charge_dA = 100;
  datalayer.battery_settings.max_user_set_discharge_dA = 100;
  datalayer.battery_settings.remote_settings_limit_charge = true;
  datalayer.battery_settings.max_remote_set_charge_dA = 200;
  datalayer.battery_settings.remote_set_timestamp = 1000;
  datalayer.battery_settings.remote_set_timeout = 30000;

  update_remote_limit_expiry(2000);
  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 7400u);  // the remote 20.0 A
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 200);
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 3700u);  // no remote discharge limit: user's 10.0 A
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 100);

  update_remote_limit_expiry(31001);  // Just past the timeout
  update_aggregate_limits();

  EXPECT_FALSE(datalayer.battery_settings.remote_settings_limit_charge);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 3700u);  // back to the user's 10.0 A
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 100);
}

// Forced recovery charge gives an installation its BMS will not charge a small current anyway.
// Written into pack 1's current, as it used to be, it was rebuilt from the 0 W power on the way to
// the inverter and never arrived. It now reaches the power and the current alike.
TEST_F(BatteryAggregateTest, RecoveryChargeReachesTheInverter) {
  datalayer.battery.status.voltage_dV = 2600;
  datalayer.battery.status.max_charge_power_W = 0;  // The BMS will not charge it
  datalayer.battery.status.max_discharge_power_W = 0;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);    // 5.0 A
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 1300u);    // 5.0 A at 260.0 V
  EXPECT_EQ(datalayer.aggregate.max_discharge_current_dA, 0);  // Discharging is left alone
  EXPECT_EQ(datalayer.aggregate.max_discharge_power_W, 0u);
}

// The setup it is most likely to be needed on: one pack refuses, the other would charge.
TEST_F(BatteryAggregateTest, RecoveryChargeAppliesToTheInstallationWithTwoPacks) {
  add_second_pack();
  battery2_detected = true;
  datalayer.battery.status.voltage_dV = 2600;
  datalayer.battery2.status.voltage_dV = 2600;
  datalayer.battery.status.max_charge_power_W = 5000;
  datalayer.battery2.status.max_charge_power_W = 0;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;

  update_aggregate_values();
  combine_pack_limits();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 1300u);
  // The packs keep their own numbers
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 0u);
}

// It replaces what the packs allow in either direction: a BMS that would take more is still held
// to the recovery current.
TEST_F(BatteryAggregateTest, RecoveryChargeHoldsAWillingBmsToFiveAmps) {
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 10000;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 50);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 1850u);  // 5.0 A at 370.0 V
}

// The manual limit lowers it, as the wiki says, and so does an active remote limit below it. A
// remote limit above the manual one does not raise it past the manual one.
TEST_F(BatteryAggregateTest, RecoveryChargeFollowsTheLowestLimit) {
  datalayer.battery.status.voltage_dV = 2600;
  datalayer.battery.status.max_charge_power_W = 0;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;
  datalayer.battery_settings.max_user_set_charge_dA = 30;

  update_aggregate_values();
  update_aggregate_limits();
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 30);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 780u);  // 3.0 A at 260.0 V

  datalayer.battery_settings.remote_settings_limit_charge = true;
  datalayer.battery_settings.max_remote_set_charge_dA = 20;
  update_aggregate_limits();
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 20);

  datalayer.battery_settings.max_remote_set_charge_dA = 200;
  update_aggregate_limits();
  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 30);
}

// A pause or an equipment stop still stops the installation. The pause zeroes the packs' limits,
// and recovery must not put a current back on top of that.
TEST_F(BatteryAggregateTest, PauseStopsRecoveryCharge) {
  datalayer.battery.status.voltage_dV = 2600;
  datalayer.battery.status.max_charge_power_W = 0;  // As the pause leaves it
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;
  emulator_pause_request_ON = true;

  update_aggregate_values();
  update_aggregate_limits();

  EXPECT_EQ(datalayer.aggregate.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.aggregate.max_charge_power_W, 0u);
  emulator_pause_request_ON = false;
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
