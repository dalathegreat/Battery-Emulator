#include <gtest/gtest.h>

#include "../../Software/src/battery/TESLA-LEGACY-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

// The legacy Model S/X driver is the only place a CAC value is read today.
// Mux 0 of 0x7E2 is the minimum, in 0.1 Ah. Maximum and average are not decoded.
// The stored reading is private, so the observable is the SOH update_values() publishes:
// raw * 10000 / 2316, with 2316 standing for a new pack of 231.6 Ah.

namespace {

CAN_frame CacMin(uint8_t mux, uint16_t raw_tenth_ah) {
  CAN_frame frame = {};
  frame.ID = 0x7E2;
  frame.DLC = 8;
  frame.data.u8[0] = mux;
  // Low 4 bits sit in the high nibble of byte 2. High 8 bits are byte 3.
  frame.data.u8[2] = (uint8_t)((raw_tenth_ah & 0x0F) << 4);
  frame.data.u8[3] = (uint8_t)((raw_tenth_ah >> 4) & 0xFF);
  return frame;
}

uint16_t SohFromRaw(uint16_t raw_tenth_ah) {
  return (uint16_t)((raw_tenth_ah * 10000u) / 2316u);
}

}  // namespace

class TeslaLegacyBatteryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    pack_ = new TeslaLegacyBattery();
    pack_->setup();
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;
  }

  void PublishCac(uint8_t mux, uint16_t raw_tenth_ah) {
    pack_->handle_incoming_can_frame(CacMin(mux, raw_tenth_ah));
    pack_->update_values();
  }

  TeslaLegacyBattery* pack_ = nullptr;
};

// BMS_CAC_min starts at 23160000, which is the new-pack reading. A missing frame
// therefore looks like 100.00% SOH rather than an unknown pack.
TEST_F(TeslaLegacyBatteryTest, MissingCacReadsAsANewPack) {
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
}

// Raw 2316 is 231.6 Ah. Byte 2 is 0xC0 and byte 3 is 0x90, so both halves of the field are in use.
TEST_F(TeslaLegacyBatteryTest, NewPackCacIsOneHundredPercent) {
  PublishCac(0, 2316);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
  EXPECT_EQ(SohFromRaw(2316), 10000);
}

// 200.0 Ah is not a round percentage of 231.6. 20000000 / 2316 truncates to 8635.
TEST_F(TeslaLegacyBatteryTest, TwoHundredAmpHoursTruncatesStateOfHealth) {
  PublishCac(0, 2000);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
  EXPECT_EQ(SohFromRaw(2000), 8635);
}

// Raw 171 uses the low nibble (byte 2 = 0xB0, byte 3 = 0x0A). Dropping that nibble still
// passes the 200.0 Ah case, whose low nibble is zero.
TEST_F(TeslaLegacyBatteryTest, LowNibbleIsPartOfTheCac) {
  PublishCac(0, 171);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 738);
  EXPECT_EQ(SohFromRaw(171), 738);
}

// A later max or average will be some other mux. Today any mux but 0 leaves the minimum alone.
TEST_F(TeslaLegacyBatteryTest, OtherMuxesLeaveTheMinimumAlone) {
  PublishCac(0, 2000);
  ASSERT_EQ(datalayer.battery.status.soh_pptt, 8635);

  PublishCac(1, 2316);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
}
