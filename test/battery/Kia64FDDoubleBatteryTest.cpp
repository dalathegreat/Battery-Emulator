#include <gtest/gtest.h>

#include "../../Software/src/battery/KIA-64FD-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

namespace {

// One 0x215 cell-voltage frame: mux 0x01, batch starting at cell 1, one little-endian mV value.
CAN_frame cell_frame(uint16_t mv) {
  CAN_frame frame = {};
  frame.ID = 0x215;
  frame.DLC = 8;
  frame.data.u8[3] = 0x01;
  frame.data.u8[4] = 0x01;
  frame.data.u8[5] = mv & 0xFF;
  frame.data.u8[6] = mv >> 8;
  return frame;
}

void reset_cells() {
  datalayer.battery.status = DATALAYER_BATTERY_STATUS_TYPE{};
  datalayer.battery2.status = DATALAYER_BATTERY_STATUS_TYPE{};
}

}  // namespace

TEST(Kia64FDDoubleBatteryTest, SecondBatteryCellVoltagesLandInItsOwnDatalayer) {
  reset_cells();
  Kia64FDBattery second(&datalayer.battery2, CAN_Interface::CANFD_NATIVE);

  second.handle_incoming_can_frame(cell_frame(3692));
  second.update_values();

  EXPECT_EQ(datalayer.battery2.status.cell_voltages_mV[0], 3692);
  EXPECT_EQ(datalayer.battery2.status.cell_max_voltage_mV, 3692);
  EXPECT_EQ(datalayer.battery2.status.cell_min_voltage_mV, 3692);

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 0);
}

TEST(Kia64FDDoubleBatteryTest, FirstBatteryStillWritesThePrimaryDatalayer) {
  reset_cells();
  Kia64FDBattery first(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  first.handle_incoming_can_frame(cell_frame(3701));
  first.update_values();

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 3701);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 3701);
  EXPECT_EQ(datalayer.battery2.status.cell_voltages_mV[0], 0);
}
