#include <gtest/gtest.h>

#include <vector>

#include "../../Software/src/battery/FISKER-OCEAN-BATTERY.h"
#include "../../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"
#include "../../Software/src/devboard/safety/safety.h"

#include "Arduino.h"

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

const CAN_frame* find_frame(uint32_t id) {
  for (const auto& frame : get_transmitted_frames()) {
    if (frame.ID == id) {
      return &frame;
    }
  }
  return nullptr;
}

TEST(FiskerOceanTests, UsesConfiguredCurrentLimits) {
  FiskerOceanBattery battery;
  battery.setup();
  datalayer.battery.settings.max_user_set_charge_dA = 350;
  datalayer.battery.settings.max_user_set_discharge_dA = 350;

  battery.update_values();

  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 350);
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 350);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 12950);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 12950);

  datalayer.battery.settings.max_user_set_charge_dA = 300;
  datalayer.battery.settings.max_user_set_discharge_dA = 300;
}

TEST(FiskerOceanTests, DecodesCrcProtectedPackCurrentAndVoltage) {
  FiskerOceanBattery battery;
  battery.setup();

  // Field capture while discharging: raw current 0x0174 / 20 = 18.6 A,
  // raw voltage 0x9218 / 100 = 374.00 V.
  CAN_frame discharge = {
      .FD = true, .ext_ID = false, .DLC = 8, .ID = 0x0E9, .data = {0x52, 0x0E, 0x32, 0x00, 0x01, 0x74, 0x92, 0x18}};
  battery.handle_incoming_can_frame(discharge);
  battery.update_values();

  EXPECT_EQ(datalayer.battery.status.current_dA, -186);
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3740);

  // Vehicle charging capture: signed raw current 0xFE2E = -466, or -23.3 A
  // in Fisker's convention. Battery Emulator represents charging as positive.
  CAN_frame charge = {
      .FD = true, .ext_ID = false, .DLC = 8, .ID = 0x0E9, .data = {0x0B, 0xF4, 0xFF, 0xFF, 0xFE, 0x2E, 0xAA, 0xA0}};
  battery.handle_incoming_can_frame(charge);
  battery.update_values();

  EXPECT_EQ(datalayer.battery.status.current_dA, 233);
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4368);
}

TEST(FiskerOceanTests, RejectsPackCurrentWhenCrcIsInvalid) {
  FiskerOceanBattery battery;
  battery.setup();
  datalayer.battery.status.current_dA = 123;
  const uint16_t error_count = datalayer.battery.status.CAN_error_counter;

  CAN_frame invalid = {
      .FD = true, .ext_ID = false, .DLC = 8, .ID = 0x0E9, .data = {0x00, 0x0E, 0x32, 0x00, 0x01, 0x74, 0x92, 0x18}};
  battery.handle_incoming_can_frame(invalid);

  EXPECT_EQ(datalayer.battery.status.current_dA, 123);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, error_count + 1);
}

TEST(FiskerOceanTests, TransmitsOnlyConfirmedWakeFrames) {
  clear_transmitted_frames();
  FiskerOceanBattery battery;
  battery.setup();
  auto& fisker = datalayer_extended.fiskerOcean;
  fisker.wake_transmit_active = true;
  fisker.wake_093_counter = 0;
  fisker.wake_333_counter = 0;
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  battery.transmit_can(20);
  ASSERT_NE(find_frame(0x093), nullptr);
  EXPECT_EQ(find_frame(0x093)->data.u8[0], 0x05);
  EXPECT_EQ(find_frame(0x333), nullptr);
  EXPECT_EQ(get_transmitted_frames().size(), 1);

  clear_transmitted_frames();
  battery.transmit_can(50);
  ASSERT_NE(find_frame(0x333), nullptr);
  EXPECT_EQ(find_frame(0x333)->data.u8[0], 0xB8);

  clear_transmitted_frames();
  battery.transmit_can(100);
  EXPECT_NE(find_frame(0x093), nullptr);
  EXPECT_NE(find_frame(0x333), nullptr);
  EXPECT_EQ(get_transmitted_frames().size(), 2);
}

TEST(FiskerOceanTests, ExposesSharedBmsPowerCycleCommand) {
  FiskerOceanBattery battery;
  battery.setup();

  remote_bms_reset = true;
  periodic_bms_reset = false;
  contactor_control_enabled = false;
  datalayer.system.info.equipment_stop_active = false;
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  datalayer.battery.status.current_dA = 0;
  set_millis64(1000);

  EXPECT_TRUE(battery.supports_reset_BMS());
  battery.reset_BMS();
  EXPECT_EQ(datalayer.system.status.bms_reset_status, BMS_RESET_WAITING_FOR_PAUSE);

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  setBatteryPause(false, false, EquipmentStop::UNCHANGED, false);
  remote_bms_reset = false;
}

TEST(FiskerOceanTests, SuppressesCanAndUdsTrafficDuringBmsPowerCycle) {
  clear_transmitted_frames();
  FiskerOceanBattery battery;
  battery.setup();
  datalayer_extended.fiskerOcean.wake_transmit_active = true;

  datalayer.system.status.bms_reset_status = BMS_RESET_POWERED_OFF;
  battery.transmit_can(1000);
  EXPECT_TRUE(get_transmitted_frames().empty());

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  battery.transmit_can(1100);

  bool sent_093 = false;
  bool sent_333 = false;
  for (const auto& frame : get_transmitted_frames()) {
    sent_093 |= frame.ID == 0x093;
    sent_333 |= frame.ID == 0x333;
  }
  EXPECT_TRUE(sent_093);
  EXPECT_TRUE(sent_333);
}
