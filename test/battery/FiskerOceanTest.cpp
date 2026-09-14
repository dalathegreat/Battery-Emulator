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

TEST(FiskerOceanTests, TransmitsOnlyConfirmedFramesByDefault) {
  clear_transmitted_frames();
  FiskerOceanBattery battery;
  battery.setup();
  auto& fisker = datalayer_extended.fiskerOcean;
  fisker.wake_transmit_active = true;
  fisker.wake_093_counter = 0;
  fisker.wake_333_counter = 0;
  fisker.ready_candidate_enable_mask = 0;
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  battery.transmit_can(20);
  ASSERT_NE(find_frame(0x093), nullptr);
  EXPECT_EQ(find_frame(0x093)->data.u8[0], 0x05);
  EXPECT_EQ(find_frame(0x214), nullptr);
  EXPECT_EQ(find_frame(0x333), nullptr);
  EXPECT_EQ(find_frame(0x358), nullptr);
  EXPECT_EQ(find_frame(0x511), nullptr);

  clear_transmitted_frames();
  battery.transmit_can(50);
  ASSERT_NE(find_frame(0x333), nullptr);
  EXPECT_EQ(find_frame(0x333)->data.u8[0], 0xB8);
}

TEST(FiskerOceanTests, TransmitsSelectedOptionalReadyFrames) {
  clear_transmitted_frames();
  FiskerOceanBattery battery;
  battery.setup();
  auto& fisker = datalayer_extended.fiskerOcean;
  fisker.wake_transmit_active = true;
  fisker.ready_candidate_enable_mask = (1U << 3) | (1U << 10) | (1U << 14);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  battery.transmit_can(100);
  ASSERT_NE(find_frame(0x214), nullptr);
  ASSERT_NE(find_frame(0x358), nullptr);
  ASSERT_NE(find_frame(0x511), nullptr);
  EXPECT_EQ(find_frame(0x214)->data.u8[0], 0xB0);
  EXPECT_EQ(find_frame(0x358)->data.u8[0], 0x31);
  EXPECT_EQ(find_frame(0x511)->data.u8[2], 0x01);
  EXPECT_TRUE(find_frame(0x358)->FD);
  EXPECT_TRUE(find_frame(0x511)->FD);
  fisker.ready_candidate_enable_mask = 0;
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
