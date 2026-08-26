#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/battery/RELION-LV-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

// Regression coverage for RelionBattery::estimateSOC() after its LFP table moved out to the
// shared cell_soc_estimator module. Exercises the class through its public CAN/update_values()
// interface rather than the private estimateSOC()/estimateSOCfromCellvoltage(), so this pins down
// the class's actual observable behavior, not just the shared module underneath it.

namespace {

CAN_frame relion_cell_voltage_frame(uint16_t max_cell_mV, uint16_t min_cell_mV) {
  CAN_frame frame = {};
  frame.ID = 0x02038100;
  frame.DLC = 8;
  frame.data.u8[0] = max_cell_mV >> 8;
  frame.data.u8[1] = max_cell_mV & 0xFF;
  frame.data.u8[4] = min_cell_mV >> 8;
  frame.data.u8[5] = min_cell_mV & 0xFF;
  return frame;
}

// user_selected_* are globals shared by every test in the binary; reset them so this file's
// tests don't leak state into (or inherit it from) unrelated battery tests.
RelionBattery* battery_with_cell_voltages(uint16_t max_cell_mV, uint16_t min_cell_mV) {
  user_selected_max_cell_voltage_mV = 0;
  user_selected_min_cell_voltage_mV = 0;

  auto battery = new RelionBattery();
  battery->setup();
  battery->handle_incoming_can_frame(relion_cell_voltage_frame(max_cell_mV, min_cell_mV));
  user_selected_use_estimated_SOC = true;
  return battery;
}

}  // namespace

TEST(RelionBatteryTest, OvervoltageReportsFullChargeRegardlessOfMinCell) {
  auto battery = battery_with_cell_voltages(3650, 2900);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 10000);
}

// min=2800mV is <= the table's floor (2803mV), so estimateSOC()'s early-return guard reports a
// hard 0.00% here - not the curve's own 1.00% clamp floor (see CellSocEstimatorTest), which only
// shows up when soc_from_cell_voltage() is called without this guard in front of it.
TEST(RelionBatteryTest, UndervoltageReportsEmptyRatherThanTheCurvesOnePercentFloor) {
  auto battery = battery_with_cell_voltages(3300, 2800);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 0);
}

TEST(RelionBatteryTest, AboveZoneCrossoverTrustsTheHighestCell) {
  // max=3410mV is an exact LFP table point (idx8, 92.00%) and above the 3380mV crossover.
  auto battery = battery_with_cell_voltages(3410, 3300);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 9200);
}

TEST(RelionBatteryTest, BelowZoneCrossoverTrustsTheLowestCell) {
  // max=3300mV is below the 3380mV crossover, so the min cell (3370mV, exact table point, 90.00%)
  // decides the reported SOC instead.
  auto battery = battery_with_cell_voltages(3300, 3370);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 9000);
}
