#include <gtest/gtest.h>

#include <algorithm>

#include "../../Software/src/battery/TESLA-BATTERY.h"
#include "Arduino.h"

namespace {

CAN_frame charge_line_264(uint8_t dlc = 6) {
  CAN_frame frame = {};
  frame.ID = 0x264;
  frame.DLC = dlc;
  const uint8_t payload[6] = {0xB7, 0x0D, 0x1E, 0x0E, 0x78, 0x00};
  std::copy(payload, payload + 6, frame.data.u8);
  return frame;
}

}  // namespace

TEST(TeslaChargeLine, DecodesCapturedPcsFrameIndependentlyOfChargeMode) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  ASSERT_TRUE(battery.supports_charge_line_measurements());
  ASSERT_FALSE(battery.is_charge_mode_active());

  battery.handle_incoming_can_frame(charge_line_264());

  EXPECT_TRUE(battery.is_charge_line_data_valid());
  EXPECT_NEAR(battery.get_charge_line_voltage_V(), 116.9f, 0.05f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_current_A(), 12.0f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_power_W(), 1400.0f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_current_limit_A(), 12.0f);
}

TEST(TeslaChargeLine, IgnoresShortFramesAndExpiresWithoutZeroingMeasurements) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.handle_incoming_can_frame(charge_line_264());
  ASSERT_TRUE(battery.is_charge_line_data_valid());

  // A short frame arriving later must not refresh the valid sample's timer.
  set_millis64(2500);
  CAN_frame short_frame = charge_line_264(5);
  short_frame.data.u8[0] = 0;
  battery.handle_incoming_can_frame(short_frame);
  EXPECT_TRUE(battery.is_charge_line_data_valid());
  EXPECT_NEAR(battery.get_charge_line_voltage_V(), 116.9f, 0.05f);

  set_millis64(3001);
  EXPECT_FALSE(battery.is_charge_line_data_valid());
  EXPECT_NEAR(battery.get_charge_line_voltage_V(), 116.9f, 0.05f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_current_A(), 12.0f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_power_W(), 1400.0f);
  EXPECT_FLOAT_EQ(battery.get_charge_line_current_limit_A(), 12.0f);
}

TEST(TeslaChargeLine, IsNotAdvertisedForTeslaModelSx) {
  user_selected_battery_type = BatteryType::TeslaModelSX;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  EXPECT_FALSE(battery.supports_charge_line_measurements());
}
