#include <gtest/gtest.h>
#include <map>
#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/FOXESS-EP-CAN.h"
#include "Arduino.h"

extern void clear_transmitted_frames();
extern const std::vector<CAN_frame>& get_transmitted_frames();

namespace {
class FoxessEpProtocolTest : public testing::Test {
 protected:
  FoxessEpCanInverter fox;
  uint32_t now = 0;
  void SetUp() override {
    set_millis64(0);
    datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    datalayer.battery.status.real_bms_status = BMS_ACTIVE;
    datalayer.battery.status.voltage_dV = 4000;
    datalayer.battery.status.reported_current_dA = 100;
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.battery_allows_contactor_closing = true;
    datalayer.system.status.contactors_engaged = 1;
    datalayer.battery.info.total_capacity_Wh = 10000;
    datalayer.battery_settings.soc_scaling_active = false;
    datalayer.battery.status.real_soc = 5000;
    datalayer.battery.status.soh_pptt = 10000;
    fox.setup();
  }
  void TearDown() override {
    set_millis64(0);
    clear_transmitted_frames();
  }
  std::map<uint32_t, CAN_frame> frames() {
    update_aggregate_values();
    update_aggregate_limits();
    fox.update_values();
    CAN_frame request = {};
    request.ID = 0x1871;
    request.DLC = 8;
    request.data.u8[0] = 1;
    fox.map_can_frame_to_variable(request);
    clear_transmitted_frames();
    for (unsigned i = 0; i < 6; ++i) {
      now += 10;
      fox.transmit_can(now);
    }
    std::map<uint32_t, CAN_frame> result;
    for (const auto& frame : get_transmitted_frames()) {
      result[frame.ID] = frame;
    }
    EXPECT_EQ(result.count(0x1875), 1);
    EXPECT_EQ(result.count(0x1878), 1);
    EXPECT_EQ(result.count(0x1879), 1);
    EXPECT_EQ(result.count(0x187A), 1);
    EXPECT_EQ(result.count(0x187B), 1);
    EXPECT_EQ(result.count(0x1903), 1);
    return result;
  }
  uint32_t le32(const CAN_frame& frame, unsigned offset) {
    return uint32_t(frame.data.u8[offset]) | (uint32_t(frame.data.u8[offset + 1]) << 8) |
           (uint32_t(frame.data.u8[offset + 2]) << 16) | (uint32_t(frame.data.u8[offset + 3]) << 24);
  }
  uint16_t cycles(const CAN_frame& frame) { return frame.data.u8[6] | (uint16_t(frame.data.u8[7]) << 8); }
  uint16_t le16(const CAN_frame& frame, unsigned offset) {
    return frame.data.u8[offset] | (uint16_t(frame.data.u8[offset + 1]) << 8);
  }
};

TEST_F(FoxessEpProtocolTest, DirectionEncodingKeepsExistingActivityDeadbandAndCurrentSign) {
  for (int16_t current : {-11, -10, -9, -6, -5, 0, 5, 6, 9, 10, 11}) {
    SCOPED_TRACE(current);
    datalayer.battery.status.reported_current_dA = current;
    auto data = frames();
    const uint8_t direction = current >= 10 ? 1 : (current <= -10 ? 2 : 0);
    EXPECT_EQ(data.at(0x187B).data.u8[1], direction);
    EXPECT_EQ(static_cast<int16_t>(le16(data.at(0x1873), 2)), -current);
  }
  datalayer.system.status.contactors_engaged = 0;
  datalayer.battery.status.reported_current_dA = 100;
  auto data = frames();
  EXPECT_EQ(data.at(0x187B).data.u8[1], 0);  // Ready but no active power path.
  EXPECT_EQ(le16(data.at(0x1873), 2), 0);
}

TEST_F(FoxessEpProtocolTest, SocScaledEnergyTruncatesUsingWholePercentAndNominal10Wh) {
  datalayer.battery.info.total_capacity_Wh = 12297;
  for (uint16_t soc : {0, 99, 100, 2579, 5000, 9999, 10000, 11000}) {
    SCOPED_TRACE(soc);
    datalayer.battery.status.real_soc = soc;
    auto data = frames();
    const uint32_t percent = soc / 100U > 100U ? 100U : soc / 100U;
    const uint32_t nominal10 = le16(data.at(0x1873), 6);
    EXPECT_EQ(nominal10, 1229U);
    EXPECT_EQ(le16(data.at(0x1903), 6), nominal10 * percent / 1000U);
    for (unsigned i = 0; i < 6; ++i) {
      EXPECT_EQ(data.at(0x1903).data.u8[i], 0);
    }
  }
}

TEST_F(FoxessEpProtocolTest, SocScaledEnergyKeepsNominalSohConversionAndBounds) {
  datalayer.battery_settings.soc_scaling_active = true;
  datalayer.battery_settings.min_percentage = 0;
  datalayer.battery_settings.max_percentage = 10000;
  datalayer.battery.status.real_soc = 2550;
  datalayer.battery.status.soh_pptt = 8000;
  datalayer.battery.info.total_capacity_Wh = 10000;
  auto data = frames();
  EXPECT_EQ(le16(data.at(0x1873), 6), 1250);
  EXPECT_EQ(le16(data.at(0x1903), 6), 31);  // 1250 * 25 / 1000, truncated.
  datalayer.battery_settings.soc_scaling_active = false;
  datalayer.battery.info.total_capacity_Wh = 1000000;
  datalayer.battery.status.real_soc = 10000;
  data = frames();
  EXPECT_EQ(le16(data.at(0x1873), 6), 65535);
  EXPECT_EQ(le16(data.at(0x1903), 6), 6553);
}

}  // namespace
