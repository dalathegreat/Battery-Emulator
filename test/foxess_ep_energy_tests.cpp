#include <gtest/gtest.h>
#include <map>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/FOXESS-EP-CAN.h"
#include "Arduino.h"

extern void clear_transmitted_frames();
extern const std::vector<CAN_frame>& get_transmitted_frames();

namespace {
class FoxessEpEnergyTest : public testing::Test {
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
    fox.setup();
  }
  void TearDown() override {
    set_millis64(0);
    clear_transmitted_frames();
  }
  std::map<uint32_t, CAN_frame> frames() {
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
    for (const auto& frame : get_transmitted_frames())
      result[frame.ID] = frame;
    EXPECT_EQ(result.count(0x1875), 1);
    EXPECT_EQ(result.count(0x1878), 1);
    EXPECT_EQ(result.count(0x1879), 1);
    EXPECT_EQ(result.count(0x187A), 1);
    return result;
  }
  uint32_t le32(const CAN_frame& frame, unsigned offset) {
    return uint32_t(frame.data.u8[offset]) | (uint32_t(frame.data.u8[offset + 1]) << 8) |
           (uint32_t(frame.data.u8[offset + 2]) << 16) | (uint32_t(frame.data.u8[offset + 3]) << 24);
  }
  uint16_t cycles(const CAN_frame& frame) { return frame.data.u8[6] | (uint16_t(frame.data.u8[7]) << 8); }
};

TEST_F(FoxessEpEnergyTest, MapsGenericWhAndDAhWithProvenScalingDirectionAndEndian) {
  datalayer.battery.status.total_charged_battery_Wh = 123456789;
  datalayer.battery.status.total_discharged_battery_Wh = 234567890;
  datalayer.battery.status.total_charged_battery_dAh = 0x12345678;
  datalayer.battery.status.total_discharged_battery_dAh = 0xFEDCBA98;
  auto data = frames();
  EXPECT_EQ(le32(data.at(0x1878), 4), 358024679U);
  EXPECT_EQ(le32(data.at(0x1879), 0), 0x12345678U);
  EXPECT_EQ(le32(data.at(0x1879), 4), 0xFEDCBA98U);
  EXPECT_EQ(le32(data.at(0x187A), 0), 1234567U);
  EXPECT_EQ(le32(data.at(0x187A), 4), 2345678U);
  EXPECT_EQ(cycles(data.at(0x1875)), 17901);
}

TEST_F(FoxessEpEnergyTest, RepeatedFoxUpdatesDoNotAccumulateLocally) {
  datalayer.battery.status.total_charged_battery_Wh = 1000;
  datalayer.battery.status.total_discharged_battery_Wh = 2000;
  datalayer.battery.status.total_charged_battery_dAh = 30;
  datalayer.battery.status.total_discharged_battery_dAh = 40;
  auto before = frames();
  set_millis64(3600000);
  auto after = frames();
  for (uint32_t id : {0x1878, 0x1879, 0x187A}) {
    for (unsigned i = 0; i < 8; ++i)
      EXPECT_EQ(before.at(id).data.u8[i], after.at(id).data.u8[i]);
  }
  EXPECT_EQ(cycles(before.at(0x1875)), cycles(after.at(0x1875)));
}

TEST_F(FoxessEpEnergyTest, CounterBoundariesDoNotWrapOnWire) {
  datalayer.battery.status.total_charged_battery_Wh = INT32_MAX;
  datalayer.battery.status.total_discharged_battery_Wh = INT32_MAX;
  datalayer.battery.status.total_charged_battery_dAh = UINT32_MAX;
  datalayer.battery.status.total_discharged_battery_dAh = UINT32_MAX;
  auto data = frames();
  EXPECT_EQ(le32(data.at(0x1878), 4), 4294967294U);
  EXPECT_EQ(le32(data.at(0x1879), 0), UINT32_MAX);
  EXPECT_EQ(le32(data.at(0x1879), 4), UINT32_MAX);
  EXPECT_EQ(le32(data.at(0x187A), 0), 21474836U);
  EXPECT_EQ(le32(data.at(0x187A), 4), 21474836U);
  EXPECT_EQ(cycles(data.at(0x1875)), UINT16_MAX);
}

TEST_F(FoxessEpEnergyTest, InvalidNegativeNativeWhIsNotAdvertisedAsHugeUnsignedEnergy) {
  datalayer.battery.status.total_charged_battery_Wh = -1;
  datalayer.battery.status.total_discharged_battery_Wh = 199;
  auto data = frames();
  EXPECT_EQ(le32(data.at(0x1878), 4), 199U);
  EXPECT_EQ(le32(data.at(0x187A), 0), 0U);
  EXPECT_EQ(le32(data.at(0x187A), 4), 1U);
}

TEST_F(FoxessEpEnergyTest, CycleDenominatorAndReadinessRemainUnchanged) {
  datalayer.battery.status.total_charged_battery_Wh = 39999;
  EXPECT_EQ(cycles(frames().at(0x1875)), 1);
  datalayer.battery.status.total_discharged_battery_Wh = 1;
  EXPECT_EQ(cycles(frames().at(0x1875)), 2);
  datalayer.system.status.contactors_engaged = 0;  // Readiness gate does not require engagement for cycles.
  EXPECT_EQ(cycles(frames().at(0x1875)), 2);
  datalayer.system.status.system_status = FAULT;
  EXPECT_EQ(cycles(frames().at(0x1875)), 0);
  datalayer.system.status.system_status = ACTIVE;
  datalayer.battery.info.total_capacity_Wh = 0;
  EXPECT_EQ(cycles(frames().at(0x1875)), 0);
}
}  // namespace
