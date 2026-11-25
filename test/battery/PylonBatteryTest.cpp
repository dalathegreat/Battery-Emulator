#include <gtest/gtest.h>

#include "../../Software/src/battery/PYLON-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// Pin the current-limit -> power-limit conversion in the Pylon/Dyness driver.

namespace {

// Little-endian 16-bit word into bytes[index], bytes[index+1].
void put_le16(CAN_frame& frame, uint8_t index, uint16_t value) {
  frame.data.u8[index] = value & 0xFF;
  frame.data.u8[index + 1] = (value >> 8) & 0xFF;
}

// 0x4210:
// voltage_dV @0
// current word (dA + 30000) @2
// temp word (dC + 1000) @4
// SOC @6
// SOH @7
CAN_frame pylon_4210(uint16_t voltage_dV, int16_t current_dA, uint8_t soc, uint8_t soh) {
  CAN_frame frame = {};
  frame.ext_ID = true;
  frame.DLC = 8;
  frame.ID = 0x4210;
  put_le16(frame, 0, voltage_dV);
  put_le16(frame, 2, static_cast<uint16_t>(current_dA + 30000));
  put_le16(frame, 4, static_cast<uint16_t>(1000));  // 0.0 degC
  frame.data.u8[6] = soc;
  frame.data.u8[7] = soh;
  return frame;
}

// 0x4220:
// charge cutoff @0
// discharge cutoff @2
// max charge current (dA + 30000) @4
// max discharge current (dA + 30000) @6
CAN_frame pylon_4220(int16_t max_charge_current_dA, int16_t max_discharge_current_dA) {
  CAN_frame frame = {};
  frame.ext_ID = true;
  frame.DLC = 8;
  frame.ID = 0x4220;
  put_le16(frame, 0, 4088);  // charge cutoff voltage, unused by these assertions
  put_le16(frame, 2, 2968);  // discharge cutoff voltage, unused
  put_le16(frame, 4, static_cast<uint16_t>(max_charge_current_dA + 30000));
  put_le16(frame, 6, static_cast<uint16_t>(max_discharge_current_dA + 30000));
  return frame;
}

// Feeds voltage + the given current limits through the real decode path and publishes to datalayer.
void decode_limits(PylonBattery* battery, uint16_t voltage_dV, int16_t max_charge_current_dA,
                   int16_t max_discharge_current_dA) {
  battery->handle_incoming_can_frame(pylon_4210(voltage_dV, 0, 50, 100));
  battery->handle_incoming_can_frame(pylon_4220(max_charge_current_dA, max_discharge_current_dA));
  battery->update_values();
}

}  // namespace

// 0.1 A limit on a 360 V pack: 1 dA * 3600 dV / 100 = 36 W
TEST(PylonBatteryTests, SubOneAmpLimitDoesNotTruncateToZero) {
  auto battery = new PylonBattery();
  decode_limits(battery, 3600, 1, 1);

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 36u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 36u);

  delete battery;
}

// A fractional-amp mid-range limit must keep the 0.5 A: 155 dA * 3600 dV / 100 = 5580 W
TEST(PylonBatteryTests, FractionalAmpLimitIsNotDiscarded) {
  auto battery = new PylonBattery();
  decode_limits(battery, 3600, 155, 155);

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5580u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5580u);

  delete battery;
}

namespace {

// 0x7320 (system equipment info):
// total cell count @0 (LE16)
// modules in series @2
// cells per module @3
// voltage level [V] @4 (LE16)
// capacity [Ah] @6 (LE16)
CAN_frame pylon_7320(uint16_t cells, uint8_t modules, uint8_t cells_per_module, uint16_t voltage_level_V,
                     uint16_t capacity_Ah) {
  CAN_frame frame = {};
  frame.ext_ID = true;
  frame.DLC = 8;
  frame.ID = 0x7320;
  put_le16(frame, 0, cells);
  frame.data.u8[2] = modules;
  frame.data.u8[3] = cells_per_module;
  put_le16(frame, 4, voltage_level_V);
  put_le16(frame, 6, capacity_Ah);
  return frame;
}

// 0x18FF9A6E (Dyness): byte0 = 0x03, remaining capacity @3 (LE16, *10 Wh), total capacity @5 (LE16, *10 Wh)
CAN_frame dyness_9a6e(uint16_t remaining_10Wh, uint16_t total_10Wh) {
  CAN_frame frame = {};
  frame.ext_ID = true;
  frame.DLC = 8;
  frame.ID = 0x18FF9A6E;
  frame.data.u8[0] = 0x03;
  frame.data.u8[3] = remaining_10Wh & 0xFF;
  frame.data.u8[4] = (remaining_10Wh >> 8) & 0xFF;
  frame.data.u8[5] = total_10Wh & 0xFF;
  frame.data.u8[6] = (total_10Wh >> 8) & 0xFF;
  return frame;
}

}  // namespace

// Battery without 0x18FF9A6E: nominal capacity from 0x7320 (130 Ah * 358 V = 46540 Wh).
// The 358 V voltage level does not fit in 8 bits and must not be truncated.
TEST(PylonBatteryTests, CapacityFallsBackToNominalFrom7320) {
  auto battery = new PylonBattery();
  battery->handle_incoming_can_frame(pylon_4210(3661, 0, 50, 100));
  battery->handle_incoming_can_frame(pylon_7320(112, 7, 16, 358, 130));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 46540u);

  delete battery;
}

// Battery that sends both: real capacity from 0x18FF9A6E has priority over the 0x7320 nominal value,
// regardless of the order the frames arrive in.
TEST(PylonBatteryTests, DynessCapacityHasPriorityOver7320) {
  auto battery = new PylonBattery();
  battery->handle_incoming_can_frame(pylon_7320(16, 1, 16, 51, 100));
  battery->handle_incoming_can_frame(dyness_9a6e(256, 512));
  battery->handle_incoming_can_frame(pylon_7320(16, 1, 16, 51, 100));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 5120u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 2560u);

  delete battery;
}

// Dyness capacity above 65535 Wh must not overflow (7000 * 10 Wh = 70000 Wh).
TEST(PylonBatteryTests, DynessCapacityAbove65kWhDoesNotOverflow) {
  auto battery = new PylonBattery();
  battery->handle_incoming_can_frame(dyness_9a6e(7000, 7000));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 70000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 70000u);

  delete battery;
}
