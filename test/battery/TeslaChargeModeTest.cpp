#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "../../Software/src/battery/TESLA-BATTERY.h"
#include "../../Software/src/charger/CanCharger.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "Arduino.h"

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

const CAN_frame* last_frame_with_id(uint32_t id) {
  const auto& frames = get_transmitted_frames();
  auto match = std::find_if(frames.rbegin(), frames.rend(), [id](const CAN_frame& frame) { return frame.ID == id; });
  return match == frames.rend() ? nullptr : &*match;
}

uint8_t tesla_checksum(const CAN_frame& frame, uint8_t checksum_byte = 7) {
  uint8_t checksum = static_cast<uint8_t>((frame.ID & 0xFF) + ((frame.ID >> 8) & 0x0F));
  for (uint8_t i = 0; i < frame.DLC; ++i) {
    if (i != checksum_byte) {
      checksum = static_cast<uint8_t>(checksum + frame.data.u8[i]);
    }
  }
  return checksum;
}

CAN_frame charge_port_056() {
  CAN_frame frame = {};
  frame.ID = 0x056;
  frame.DLC = 8;
  frame.data.u8[7] = 0x56;
  return frame;
}

CAN_frame closed_contactors_212() {
  CAN_frame frame = {};
  frame.ID = 0x212;
  frame.DLC = 8;
  frame.data.u8[1] = 0x04;
  return frame;
}

CAN_frame charge_line_264(uint8_t dlc = 6) {
  CAN_frame frame = {};
  frame.ID = 0x264;
  frame.DLC = dlc;
  const uint8_t payload[6] = {0xB7, 0x0D, 0x1E, 0x0E, 0x78, 0x00};
  std::copy(payload, payload + 6, frame.data.u8);
  return frame;
}

CAN_frame stopped_charge_line_264() {
  CAN_frame frame = {};
  frame.ID = 0x264;
  frame.DLC = 6;
  return frame;
}

CAN_frame charge_port_latch_disengaging_25d() {
  CAN_frame frame = {};
  frame.ID = 0x25D;
  frame.DLC = 8;
  const uint8_t payload[8] = {0x6C, 0x81, 0x23, 0xAC, 0x04, 0x00, 0x00, 0x00};
  std::copy(payload, payload + 8, frame.data.u8);
  return frame;
}

CAN_frame charge_port_unplugged_21d() {
  CAN_frame frame = {};
  frame.ID = 0x21D;
  frame.DLC = 8;
  frame.data.u8[0] = 0x04;  // CP_proximity = 1 (connector removed)
  return frame;
}

CAN_frame charge_port_inserted_21d() {
  CAN_frame frame = {};
  frame.ID = 0x21D;
  frame.DLC = 8;
  frame.data.u8[0] = 0x0C;  // CP_proximity = 3 (connector inserted)
  return frame;
}

CAN_frame charge_handle_pressed_21d() {
  CAN_frame frame = {};
  frame.ID = 0x21D;
  frame.DLC = 8;
  frame.data.u8[0] = 0x2A;  // CP_proximity = 2 (physical handle button)
  return frame;
}

void call_five_phases(TeslaBattery& battery, unsigned long now) {
  for (int i = 0; i < 5; ++i) {
    battery.transmit_can(now);
  }
}

}  // namespace

class TeslaChargeModeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    user_selected_battery_type = BatteryType::TeslaModel3Y;
    user_selected_charger_type = ChargerType::TeslaModel3YPcs;
    user_selected_tesla_digital_HVIL = false;
  }
};

TEST(TeslaChargeModeConfig, RequiresTeslaPcsChargerSelection) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_charger_type = ChargerType::None;
  user_selected_tesla_digital_HVIL = false;

  TeslaBattery battery;
  battery.setup();

  EXPECT_FALSE(battery.supports_charge_mode());
  battery.start_charge_mode();
  EXPECT_FALSE(battery.is_charge_mode_active());
}

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

TEST_F(TeslaChargeModeTest, EmitsMeasuredStartupAndSuccessfulChargeProfile) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.handle_incoming_can_frame(closed_contactors_212());
  ASSERT_TRUE(battery.supports_charge_mode());

  battery.start_charge_mode();
  ASSERT_TRUE(battery.is_charge_mode_active());

  clear_transmitted_frames();
  battery.transmit_can(1010);  // phase 0: 10 ms frames
  battery.transmit_can(1010);  // phase 1: 50 ms frames

  const CAN_frame* frame053 = last_frame_with_id(0x053);
  ASSERT_NE(frame053, nullptr);
  EXPECT_EQ(frame053->data.u8[0], 0x54);
  EXPECT_EQ(frame053->data.u8[3], 0xC3);

  const CAN_frame* frame055 = last_frame_with_id(0x055);
  ASSERT_NE(frame055, nullptr);
  EXPECT_EQ(frame055->data.u8[0], 0x00);
  EXPECT_EQ(frame055->data.u8[1], 0x00);
  EXPECT_EQ(frame055->data.u8[7], tesla_checksum(*frame055));

  const CAN_frame* frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[1] & 0xF0, 0x80);
  EXPECT_EQ(frame118->data.u8[2], 0x2D);
  EXPECT_EQ(frame118->data.u8[5], 0x08);
  EXPECT_EQ(frame118->data.u8[7], 0x00);
  EXPECT_EQ(frame118->data.u8[0], tesla_checksum(*frame118, 0));

  const CAN_frame* frame221 = last_frame_with_id(0x221);
  ASSERT_NE(frame221, nullptr);
  EXPECT_EQ(frame221->data.u8[0] & 0xF0, 0x00);
  EXPECT_EQ(frame221->data.u8[7], tesla_checksum(*frame221));

  // Advance through one more phase-0 cycle so the alternating 0x053 sender is
  // due again when the profile reaches steady state.
  for (int i = 0; i < 4; ++i) {
    battery.transmit_can(1150);
  }
  clear_transmitted_frames();
  call_five_phases(battery, 4140);

  frame053 = last_frame_with_id(0x053);
  ASSERT_NE(frame053, nullptr);
  EXPECT_EQ(frame053->data.u8[0], 0xD4);
  EXPECT_EQ(frame053->data.u8[3], 0xCB);

  frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[2], 0xE9);
  EXPECT_EQ(frame118->data.u8[5], 0x48);
  EXPECT_EQ(frame118->data.u8[7], 0x00);
  EXPECT_EQ(frame118->data.u8[0], tesla_checksum(*frame118, 0));

  frame221 = last_frame_with_id(0x221);
  ASSERT_NE(frame221, nullptr);
  EXPECT_EQ(frame221->data.u8[0] & 0xF0, 0x20);
  EXPECT_EQ(frame221->data.u8[7], tesla_checksum(*frame221));

  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x05);

  const CAN_frame* frame339 = last_frame_with_id(0x339);
  ASSERT_NE(frame339, nullptr);
  const uint8_t expected339[8] = {0x41, 0x44, 0xF8, 0x00, 0x00, 0x03, 0x80, 0x00};
  EXPECT_EQ(frame339->DLC, 8);
  EXPECT_TRUE(std::equal(expected339, expected339 + 8, frame339->data.u8));

  const CAN_frame* frame334 = last_frame_with_id(0x334);
  ASSERT_NE(frame334, nullptr);
  const uint8_t expectedCharge334Prefix[6] = {0x3F, 0x7F, 0x14, 0x02, 0xF0, 0x23};
  EXPECT_TRUE(std::equal(expectedCharge334Prefix, expectedCharge334Prefix + 6, frame334->data.u8));
  EXPECT_EQ((frame334->data.u8[1] >> 6) & 0x03, 0x01);
  EXPECT_EQ(frame334->data.u8[7], tesla_checksum(*frame334));

  const CAN_frame* frame102 = last_frame_with_id(0x102);
  ASSERT_NE(frame102, nullptr);
  const uint8_t expectedCharge102[8] = {0x22, 0xB3, 0x48, 0x04, 0x00, 0x00, 0xA0, 0x09};
  EXPECT_TRUE(std::equal(expectedCharge102, expectedCharge102 + 8, frame102->data.u8));

  const CAN_frame* frame103 = last_frame_with_id(0x103);
  ASSERT_NE(frame103, nullptr);
  const uint8_t expectedCharge103[8] = {0x22, 0xB3, 0x88, 0x44, 0x00, 0x00, 0x20, 0x32};
  EXPECT_TRUE(std::equal(expectedCharge103, expectedCharge103 + 8, frame103->data.u8));

  const CAN_frame* frame3b3 = last_frame_with_id(0x3B3);
  ASSERT_NE(frame3b3, nullptr);
  const uint8_t expectedCharge3b3[8] = {0x90, 0x80, 0x05, 0x22, 0x80, 0x00, 0x98, 0x25};
  EXPECT_TRUE(std::equal(expectedCharge3b3, expectedCharge3b3 + 8, frame3b3->data.u8));

  const CAN_frame* frame3c2 = last_frame_with_id(0x3C2);
  ASSERT_NE(frame3c2, nullptr);
  EXPECT_TRUE(frame3c2->data.u8[0] == 0x10 || frame3c2->data.u8[0] == 0x01);

  const CAN_frame* frame3a1 = last_frame_with_id(0x3A1);
  ASSERT_NE(frame3a1, nullptr);
  if (frame3a1->data.u8[0] == 0x88) {
    EXPECT_EQ(frame3a1->data.u8[6] & 0x0F, 0x02);
    EXPECT_EQ(frame3a1->data.u8[6] >> 4 & 0x01, 0x00);
  } else {
    EXPECT_EQ(frame3a1->data.u8[0], 0x03);
    EXPECT_EQ(frame3a1->data.u8[6] & 0x0F, 0x00);
    EXPECT_EQ(frame3a1->data.u8[6] >> 4 & 0x01, 0x01);
  }
}

TEST_F(TeslaChargeModeTest, ReplaysExact3A1CounterChecksumCycle) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.handle_incoming_can_frame(closed_contactors_212());
  battery.start_charge_mode();

  const uint8_t expectedByte6[16] = {0x02, 0x10, 0x22, 0x30, 0x42, 0x50, 0x62, 0x70,
                                     0x82, 0x90, 0xA2, 0xB0, 0xC2, 0xD0, 0xE2, 0xF0};
  const uint8_t expectedByte7[16] = {0xBA, 0xE2, 0xDA, 0x02, 0xFA, 0x22, 0x1A, 0x42,
                                     0x3A, 0x62, 0x5A, 0x82, 0x7A, 0xA2, 0x9A, 0xC2};

  uint8_t previousCounter = 0xFF;
  for (uint8_t i = 0; i < 16; ++i) {
    clear_transmitted_frames();
    call_five_phases(battery, 1050 + i * 50);
    const CAN_frame* frame3a1 = last_frame_with_id(0x3A1);
    ASSERT_NE(frame3a1, nullptr);
    const uint8_t counter = frame3a1->data.u8[6] >> 4;
    EXPECT_EQ(frame3a1->data.u8[6], expectedByte6[counter]);
    EXPECT_EQ(frame3a1->data.u8[7], expectedByte7[counter]);
    EXPECT_EQ(frame3a1->data.u8[0], (counter & 1) == 0 ? 0x88 : 0x03);
    if (previousCounter != 0xFF) {
      EXPECT_EQ(counter, static_cast<uint8_t>((previousCounter + 1) % 16));
    }
    previousCounter = counter;
  }
}

TEST_F(TeslaChargeModeTest, AdvertisesClosuresConfirmedFromFirst334Frame) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();

  clear_transmitted_frames();
  call_five_phases(battery, 1500);
  const CAN_frame* frame334 = last_frame_with_id(0x334);
  ASSERT_NE(frame334, nullptr);
  EXPECT_EQ((frame334->data.u8[1] >> 6) & 0x03, 0x01);
  EXPECT_EQ(frame334->data.u8[7], tesla_checksum(*frame334));

  clear_transmitted_frames();
  call_five_phases(battery, 2000);
  frame334 = last_frame_with_id(0x334);
  ASSERT_NE(frame334, nullptr);
  EXPECT_EQ((frame334->data.u8[1] >> 6) & 0x03, 0x01);
  EXPECT_EQ(frame334->data.u8[7], tesla_checksum(*frame334));
}

TEST_F(TeslaChargeModeTest, SendsMeasured052OnlyWhileChargeModeIsActive) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.start_charge_mode();

  clear_transmitted_frames();
  call_five_phases(battery, 1100);

  const CAN_frame* frame052 = last_frame_with_id(0x052);
  ASSERT_NE(frame052, nullptr);
  const uint8_t expected052[8] = {0x85, 0x9B, 0xE4, 0x27, 0x65, 0x28, 0x30, 0x00};
  EXPECT_EQ(frame052->DLC, 8);
  EXPECT_TRUE(std::equal(expected052, expected052 + 8, frame052->data.u8));

  const CAN_frame* frame339 = last_frame_with_id(0x339);
  ASSERT_NE(frame339, nullptr);
  const uint8_t expected339[8] = {0x41, 0x44, 0xF8, 0x00, 0x00, 0x03, 0x80, 0x00};
  EXPECT_EQ(frame339->DLC, 8);
  EXPECT_TRUE(std::equal(expected339, expected339 + 8, frame339->data.u8));

  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();
  EXPECT_TRUE(battery.is_charge_mode_active());
  clear_transmitted_frames();
  call_five_phases(battery, 15999);
  EXPECT_NE(last_frame_with_id(0x052), nullptr);
  EXPECT_NE(last_frame_with_id(0x339), nullptr);

  clear_transmitted_frames();
  call_five_phases(battery, 16000);
  EXPECT_TRUE(battery.is_charge_mode_active());
}

TEST_F(TeslaChargeModeTest, PulsesChargePortHatchRequestWhenChargeModeStarts) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.start_charge_mode();

  clear_transmitted_frames();
  call_five_phases(battery, 1100);
  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x05);

  clear_transmitted_frames();
  call_five_phases(battery, 1250);
  frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x04);

  clear_transmitted_frames();
  call_five_phases(battery, 1510);
  frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x05);

  clear_transmitted_frames();
  call_five_phases(battery, 7600);
  frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x04);
}

TEST_F(TeslaChargeModeTest, DoesNotTreatLatchMovementWithoutHandlePressAsPrepareToUnplugProgress) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();

  set_millis64(2000);
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 2000);

  EXPECT_TRUE(battery.is_charge_mode_active());
  EXPECT_NE(last_frame_with_id(0x339), nullptr);
}

TEST_F(TeslaChargeModeTest, PreparesForPhysicalHandleReleaseThenHandsOffWithoutShutdown) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();

  set_millis64(4135);
  battery.handle_incoming_can_frame(charge_port_056());
  clear_transmitted_frames();
  call_five_phases(battery, 4140);
  EXPECT_EQ(last_frame_with_id(0x056), nullptr);

  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();
  EXPECT_TRUE(battery.is_charge_mode_active());
  clear_transmitted_frames();
  call_five_phases(battery, 4640);

  const CAN_frame* frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[1] & 0xF0, 0x80);
  EXPECT_EQ(frame118->data.u8[2], 0xE9);
  // Ext. Module keeps DI_proximity asserted until after physical removal. This
  // authorization must survive the zero-current wait and release request.
  EXPECT_EQ(frame118->data.u8[5], 0x48);
  // The successful Ext. Module unlatch trace uses the otherwise identical
  // release profile with byte 7 set. The checksum must be regenerated after
  // that state transition.
  EXPECT_EQ(frame118->data.u8[7], 0x80);
  EXPECT_EQ(frame118->data.u8[0], tesla_checksum(*frame118, 0));

  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0] & 0x04, 0x04);
  EXPECT_EQ(frame333->data.u8[0] & 0x01, 0x00);

  ASSERT_TRUE(battery.is_charge_mode_active());
  frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[2], 0xE9);
  EXPECT_EQ(frame118->data.u8[5], 0x48);
  EXPECT_EQ(frame118->data.u8[7], 0x80);
  EXPECT_EQ(frame118->data.u8[0], tesla_checksum(*frame118, 0));
  frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  const uint8_t expected333[5] = {0x04, 0x30, 0x84, 0x07, 0x02};
  EXPECT_TRUE(std::equal(expected333, expected333 + 5, frame333->data.u8));

  const uint8_t expected207[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x28, 0x00};
  const uint8_t expected241[7] = {0x50, 0x50, 0x0C, 0x14, 0x14, 0x53, 0x00};
  const uint8_t expected247[8] = {0x32, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00};
  const uint8_t expected284[8] = {0x10, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00};
  const uint8_t expected500[2] = {0x01, 0x01};
  const uint8_t expected55a[8] = {0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00};
  ASSERT_NE(last_frame_with_id(0x207), nullptr);
  EXPECT_TRUE(std::equal(expected207, expected207 + 8, last_frame_with_id(0x207)->data.u8));
  ASSERT_NE(last_frame_with_id(0x241), nullptr);
  EXPECT_TRUE(std::equal(expected241, expected241 + 7, last_frame_with_id(0x241)->data.u8));
  ASSERT_NE(last_frame_with_id(0x247), nullptr);
  EXPECT_TRUE(std::equal(expected247, expected247 + 8, last_frame_with_id(0x247)->data.u8));
  ASSERT_NE(last_frame_with_id(0x284), nullptr);
  EXPECT_EQ(last_frame_with_id(0x284)->DLC, 8);
  EXPECT_TRUE(std::equal(expected284, expected284 + 8, last_frame_with_id(0x284)->data.u8));
  ASSERT_NE(last_frame_with_id(0x500), nullptr);
  EXPECT_TRUE(std::equal(expected500, expected500 + 2, last_frame_with_id(0x500)->data.u8));
  ASSERT_NE(last_frame_with_id(0x55A), nullptr);
  EXPECT_TRUE(std::equal(expected55a, expected55a + 8, last_frame_with_id(0x55A)->data.u8));

  const CAN_frame* frame293 = last_frame_with_id(0x293);
  ASSERT_NE(frame293, nullptr);
  const uint8_t expected293Prefix[6] = {0x96, 0x08, 0x00, 0x00, 0x21, 0x10};
  EXPECT_TRUE(std::equal(expected293Prefix, expected293Prefix + 6, frame293->data.u8));
  EXPECT_EQ(frame293->data.u8[7], tesla_checksum(*frame293));

  const CAN_frame* frame313 = last_frame_with_id(0x313);
  ASSERT_NE(frame313, nullptr);
  const uint8_t expected313Prefix[6] = {0x02, 0x00, 0xC8, 0x07, 0x00, 0x00};
  EXPECT_TRUE(std::equal(expected313Prefix, expected313Prefix + 6, frame313->data.u8));
  EXPECT_EQ(frame313->data.u8[7], tesla_checksum(*frame313));

  const CAN_frame* frame2e8 = last_frame_with_id(0x2E8);
  ASSERT_NE(frame2e8, nullptr);
  const uint8_t expected2e8Prefix[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x80};
  EXPECT_TRUE(std::equal(expected2e8Prefix, expected2e8Prefix + 6, frame2e8->data.u8));
  EXPECT_EQ(frame2e8->data.u8[7], tesla_checksum(*frame2e8));

  const CAN_frame* frame339 = last_frame_with_id(0x339);
  ASSERT_NE(frame339, nullptr);
  const uint8_t expected339[8] = {0x41, 0x44, 0xF8, 0x00, 0x00, 0x03, 0x80, 0x00};
  EXPECT_TRUE(std::equal(expected339, expected339 + 8, frame339->data.u8));

  // Keep both the exact Ext. Module 0x333 profile and VCSEC authorization alive
  // until the physical handle button makes the charge port report movement.
  set_millis64(5650);
  battery.handle_incoming_can_frame(charge_handle_pressed_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  set_millis64(35650);
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 35650);

  EXPECT_TRUE(battery.is_charge_mode_active());
  frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x04);
  const CAN_frame* released241 = last_frame_with_id(0x241);
  ASSERT_NE(released241, nullptr);
  const uint8_t expectedReleased241[7] = {0x3C, 0x3C, 0x16, 0x0F, 0x8F, 0x55, 0x00};
  EXPECT_TRUE(std::equal(expectedReleased241, expectedReleased241 + 7, released241->data.u8));

  // Latch movement alone is not enough: remain online until the connector is
  // physically removed.
  set_millis64(35660);
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 35660);

  EXPECT_FALSE(battery.is_charge_mode_active());
  clear_transmitted_frames();
  call_five_phases(battery, 35670);
  frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[1] & 0xF0, 0x60);
  EXPECT_EQ(frame118->data.u8[2], 0x2A);
  EXPECT_EQ(frame118->data.u8[5], 0x08);
  EXPECT_EQ(frame118->data.u8[7], 0x00);
  EXPECT_EQ(frame118->data.u8[0], tesla_checksum(*frame118, 0));
}

TEST_F(TeslaChargeModeTest, KeepsPrepareToUnplugProfileAliveWithoutFeedbackTimeout) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();

  set_millis64(120000);
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 120000);
  EXPECT_TRUE(battery.is_charge_mode_active());
  EXPECT_NE(last_frame_with_id(0x339), nullptr);
  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0] & 0x04, 0x04);
}

TEST_F(TeslaChargeModeTest, WaitsForInverterPermissionBeforeOnlineHandoff) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = false;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();

  set_millis64(2000);
  battery.handle_incoming_can_frame(charge_handle_pressed_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 2000);
  EXPECT_TRUE(battery.is_charge_mode_active());
  EXPECT_NE(last_frame_with_id(0x339), nullptr);

  datalayer.system.status.inverter_allows_contactor_closing = true;
  set_millis64(2100);
  battery.handle_incoming_can_frame(stopped_charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 2100);
  EXPECT_FALSE(battery.is_charge_mode_active());
}

TEST_F(TeslaChargeModeTest, DoesNotHandoffWhileChargeLineIsLive) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_line_264());
  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();
  battery.handle_incoming_can_frame(charge_handle_pressed_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());

  set_millis64(16000);
  battery.handle_incoming_can_frame(charge_line_264());
  clear_transmitted_frames();
  call_five_phases(battery, 16000);
  ASSERT_TRUE(battery.is_charge_mode_active());
  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0] & 0x04, 0x04);
}

TEST_F(TeslaChargeModeTest, PhysicalHandleButtonAutomaticallyPreparesAndHandsOffWithoutWebRequest) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_line_264());
  battery.handle_incoming_can_frame(charge_port_inserted_21d());

  // No stop_charge_mode() web request: the physical handle button must arm
  // Prepare to Unplug by itself and select the release profile.
  set_millis64(2000);
  battery.handle_incoming_can_frame(charge_handle_pressed_21d());
  clear_transmitted_frames();
  call_five_phases(battery, 2000);
  ASSERT_TRUE(battery.is_charge_mode_active());
  const CAN_frame* frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[7], 0x80);
  const CAN_frame* frame333 = last_frame_with_id(0x333);
  ASSERT_NE(frame333, nullptr);
  EXPECT_EQ(frame333->data.u8[0], 0x04);

  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());

  // A recent live sample still blocks an immediate transition.
  set_millis64(2999);
  clear_transmitted_frames();
  call_five_phases(battery, 2999);
  EXPECT_TRUE(battery.is_charge_mode_active());

  // Once the post-unplug freshness window expires, return directly to the
  // normal inverter profile without a web-page action.
  set_millis64(4001);
  clear_transmitted_frames();
  call_five_phases(battery, 4001);
  EXPECT_FALSE(battery.is_charge_mode_active());
}

TEST_F(TeslaChargeModeTest, RecoversWhenTransientHandleFrameIsMissedAfterKnownInsertion) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_line_264());
  battery.handle_incoming_can_frame(charge_port_inserted_21d());

  // Reproduce the live failure: proximity=2 was not sampled. The next cyclic
  // frames report removed, then latch disengaged, then removed again.
  set_millis64(2000);
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());

  set_millis64(4001);
  clear_transmitted_frames();
  call_five_phases(battery, 4001);
  EXPECT_FALSE(battery.is_charge_mode_active());
}

TEST_F(TeslaChargeModeTest, EmptyChargePortDoesNotCancelModeWithoutKnownInsertion) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  EXPECT_FALSE(battery.can_prepare_to_unplug());

  // A stale web page or direct request must not select the release profile
  // before the connector has actually been detected.
  battery.stop_charge_mode();
  clear_transmitted_frames();
  call_five_phases(battery, 1100);
  const CAN_frame* frame118 = last_frame_with_id(0x118);
  ASSERT_NE(frame118, nullptr);
  EXPECT_EQ(frame118->data.u8[7], 0x00);
  EXPECT_EQ(last_frame_with_id(0x207), nullptr);

  // The hatch opens while no connector is present. Proximity=1 and a latch
  // status must not be interpreted as a completed unplug for this session.
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  set_millis64(10000);
  clear_transmitted_frames();
  call_five_phases(battery, 10000);
  EXPECT_TRUE(battery.is_charge_mode_active());

  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  EXPECT_TRUE(battery.can_prepare_to_unplug());
}

TEST_F(TeslaChargeModeTest, HandsOffAfterUnplugWhenPcsChargeLineFrameBecomesStale) {
  user_selected_battery_type = BatteryType::TeslaModel3Y;
  user_selected_tesla_digital_HVIL = false;
  set_millis64(1000);

  TeslaBattery battery;
  battery.setup();
  datalayer.system.status.inverter_allows_contactor_closing = true;
  battery.start_charge_mode();
  battery.handle_incoming_can_frame(charge_line_264());
  battery.handle_incoming_can_frame(charge_port_inserted_21d());
  battery.stop_charge_mode();

  set_millis64(2000);
  battery.handle_incoming_can_frame(charge_handle_pressed_21d());
  battery.handle_incoming_can_frame(charge_port_latch_disengaging_25d());
  battery.handle_incoming_can_frame(charge_port_unplugged_21d());

  // A recent non-zero sample must keep the charge profile alive immediately
  // after physical unplug.
  set_millis64(2999);
  clear_transmitted_frames();
  call_five_phases(battery, 2999);
  EXPECT_TRUE(battery.is_charge_mode_active());

  // The real PCS may simply stop 0x264 after unplug. Once that sample is stale
  // for the full freshness timeout, the ordered physical-unplug evidence is
  // sufficient for the direct inverter handoff.
  set_millis64(4001);
  clear_transmitted_frames();
  call_five_phases(battery, 4001);
  EXPECT_FALSE(battery.is_charge_mode_active());
  EXPECT_FALSE(battery.is_charge_line_data_valid());
}
