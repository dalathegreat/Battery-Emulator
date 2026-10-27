#include <gtest/gtest.h>

#include "../../Software/src/battery/TESLA-LEGACY-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

namespace {

// 0x6F2 cell-voltage frame: mux byte + four 14-bit brick voltages (0.30517578125 mV/LSB).
CAN_frame cell_frame(uint8_t mux, uint16_t mv0, uint16_t mv1, uint16_t mv2, uint16_t mv3) {
  auto raw = [](uint16_t mv) {
    return (uint16_t)(mv / 0.30517578125 + 0.5);
  };
  uint16_t r0 = raw(mv0), r1 = raw(mv1), r2 = raw(mv2), r3 = raw(mv3);
  CAN_frame f = {};
  f.ID = 0x6F2;
  f.DLC = 8;
  f.data.u8[0] = mux;
  f.data.u8[1] = r0 & 0xFF;
  f.data.u8[2] = ((r0 >> 8) & 0x3F) | ((r1 & 0x03) << 6);
  f.data.u8[3] = (r1 >> 2) & 0xFF;
  f.data.u8[4] = ((r1 >> 10) & 0x0F) | ((r2 & 0x0F) << 4);
  f.data.u8[5] = (r2 >> 4) & 0xFF;
  f.data.u8[6] = ((r2 >> 12) & 0x03) | ((r3 & 0x3F) << 2);
  f.data.u8[7] = (r3 >> 6) & 0xFF;
  return f;
}

// Feeds two full mux cycles for a pack of `bricks` series cells at 3.70 V each.
void feed_pack(TeslaLegacyBattery& b, uint16_t bricks) {
  for (auto& v : datalayer.battery.status.cell_voltages_mV) {
    v = 0;
  }
  const uint8_t frames = (bricks + 3) / 4;
  for (int cycle = 0; cycle < 2; cycle++) {
    for (uint8_t mux = 0; mux < frames; mux++) {
      uint16_t mv[4];
      for (int i = 0; i < 4; i++) {
        mv[i] = (mux * 4 + i < bricks) ? 3700 : 0;
      }
      b.handle_incoming_can_frame(cell_frame(mux, mv[0], mv[1], mv[2], mv[3]));
    }
  }
  b.handle_incoming_can_frame(cell_frame(0, 3700, 3700, 3700, 3700));  // next cycle's mux 0 closes the 2nd
}

}  // namespace

TEST(TeslaLegacySeriesLimits, StartsWithWideDefaults) {
  TeslaLegacyBattery b;
  b.setup();
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 3000);
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 5000);
}

TEST(TeslaLegacySeriesLimits, FullPack96S) {
  TeslaLegacyBattery b;
  b.setup();
  feed_pack(b, 96);
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 3072);  // 96 x 3.200 V
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4032);  // 96 x 4.200 V
}

TEST(TeslaLegacySeriesLimits, EarlyPack84S) {
  TeslaLegacyBattery b;
  b.setup();
  feed_pack(b, 84);
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 84);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2688);
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 3528);
}

TEST(TeslaLegacySeriesLimits, Pack83SNotMultipleOfFour) {
  TeslaLegacyBattery b;
  b.setup();
  feed_pack(b, 83);  // last 0x6F2 frame carries 3 bricks + an empty slot
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 83);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2656);  // 83 x 3.200 V
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 3486);  // 83 x 4.200 V
}
