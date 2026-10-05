#include <gtest/gtest.h>

#include "../../Software/src/battery/THUNDERSTRUCK-BMS.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// Pin the current-limit -> power-limit conversion in the Thunderstruck driver.

namespace {

// Big-endian 16-bit word into bytes[index], bytes[index+1].
void put_be16(CAN_frame& frame, uint8_t index, uint16_t value) {
  frame.data.u8[index] = (value >> 8) & 0xFF;
  frame.data.u8[index + 1] = value & 0xFF;
}

// 0x14ff25d0:
// DCLMin @0
// CCLMin @2
// DCLMax (1A/LSB) @4
// CCLMax (1A/LSB) @6
CAN_frame thunderstruck_limits(uint16_t dcl_max, uint16_t ccl_max) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x14ff25d0;
  put_be16(frame, 0, 0);  // DCLMin, unused
  put_be16(frame, 2, 0);  // CCLMin, unused
  put_be16(frame, 4, dcl_max);
  put_be16(frame, 6, ccl_max);
  return frame;
}

// 0x14ff21d0:
// packvoltage_dV @2
// pack_current_dA @4
CAN_frame thunderstruck_voltage(uint16_t voltage_dV) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x14ff21d0;
  put_be16(frame, 2, voltage_dV);
  put_be16(frame, 4, 0);  // pack current, unused
  return frame;
}

// Feeds voltage + the given current limits through the real decode path and publishes to datalayer.
void decode_limits(ThunderstruckBMS* battery, uint16_t voltage_dV, uint16_t dcl_max, uint16_t ccl_max) {
  battery->handle_incoming_can_frame(thunderstruck_voltage(voltage_dV));
  battery->handle_incoming_can_frame(thunderstruck_limits(dcl_max, ccl_max));
  battery->update_values();
}

}  // namespace

// A deci-volt must not be truncated. 100 A @ 359.7 V: 3597 dV * 100 A / 10 = 35970 W
TEST(ThunderstruckBmsTests, DeciVoltIsNotTruncated) {
  auto battery = new ThunderstruckBMS();
  decode_limits(battery, 3597, 100, 90);

  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 35970u);  // 100 A discharge
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 32373u);     // 90 A: 3597 * 90 / 10

  delete battery;
}

// An exact-tenths result is unchanged by the fix: 50 A @ 360.0 V = 18000 W
TEST(ThunderstruckBmsTests, RoundVoltagePreserved) {
  auto battery = new ThunderstruckBMS();
  decode_limits(battery, 3600, 50, 50);

  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 18000u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 18000u);

  delete battery;
}
