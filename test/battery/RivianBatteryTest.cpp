#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"  // user_selected_use_estimated_SOC
#include "../../Software/src/battery/RIVIAN-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// Pin the current-limit -> power-limit conversion in the Rivian driver.

namespace {

// 0x100:
// charge limit (12-bit, (u8[3]&0x0F)<<8 | u8[2]<<4 | u8[1]>>4, raw/20 = A) @1
// discharge limit (12-bit, (u8[5]&0x0F)<<8 | u8[4]<<4 | u8[3]>>4) @3
CAN_frame rivian_100(uint16_t charge_raw) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x100;
  frame.data.u8[1] = (charge_raw & 0x00F) << 4;
  frame.data.u8[2] = (charge_raw >> 4) & 0xFF;
  frame.data.u8[3] = (charge_raw >> 12) & 0x0F;
  return frame;
}

// 0x120:
// pre_contactor_voltage (13-bit, (u8[7]&0x1F)<<8 | u8[6], dV) @6
CAN_frame rivian_120(uint16_t voltage_dV) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x120;
  frame.data.u8[6] = voltage_dV & 0xFF;
  frame.data.u8[7] = (voltage_dV >> 8) & 0x1F;
  return frame;
}

// Feeds voltage + charge limit through the real decode path and publishes to datalayer. The power
// calc only runs in the non-estimated-SOC branch, so force that global off (raw = amps * 20).
void decode_limits(RivianBattery* battery, uint16_t voltage_dV, uint16_t charge_amps) {
  user_selected_use_estimated_SOC = false;
  battery->handle_incoming_can_frame(rivian_120(voltage_dV));
  battery->handle_incoming_can_frame(rivian_100(charge_amps * 20));
  battery->update_values();
}

}  // namespace

// A deci-volt must not be truncated. 100 A @ 359.7 V: 3597 dV * 100 A / 10 = 35970 W
TEST(RivianBatteryTests, DeciVoltIsNotTruncated) {
  auto battery = new RivianBattery();
  decode_limits(battery, 3597, 100);

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 35970u);

  delete battery;
}

// An exact-tenths result is unchanged by the fix: 50 A @ 360.0 V = 18000 W
TEST(RivianBatteryTests, RoundVoltagePreserved) {
  auto battery = new RivianBattery();
  decode_limits(battery, 3600, 50);

  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 18000u);

  delete battery;
}
