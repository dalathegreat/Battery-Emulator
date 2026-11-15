#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "../../Software/src/battery/KIA-E-GMP-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../emul/Arduino.h"

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

constexpr uint8_t kContactorOpen = 0xFE;
constexpr uint8_t kContactorClosed = 0x01;
constexpr uint16_t kCellCount = 192;

void feed_cell_voltages(KiaEGmpBattery& battery, uint16_t voltage_mV, int changed_cell = -1,
                        uint16_t changed_voltage_mV = 0) {
  for (uint16_t start = 0; start < kCellCount; start += 13) {
    CAN_frame frame = {};
    frame.FD = true;
    frame.ext_ID = false;
    frame.DLC = 32;
    frame.ID = 0x215;
    frame.data.u8[3] = 0x01;
    frame.data.u8[4] = static_cast<uint8_t>(start + 1);

    for (uint8_t i = 0; i < 13; ++i) {
      const uint16_t cell = start + i;
      uint16_t cell_voltage = voltage_mV;
      if (cell == changed_cell) {
        cell_voltage = changed_voltage_mV;
      } else if (cell >= kCellCount) {
        cell_voltage = 0;
      }
      frame.data.u8[5 + i * 2] = static_cast<uint8_t>(cell_voltage);
      frame.data.u8[6 + i * 2] = static_cast<uint8_t>(cell_voltage >> 8);
    }
    battery.handle_incoming_can_frame(frame);
  }
}

void feed_contactor_status(KiaEGmpBattery& battery, uint8_t status) {
  CAN_frame frame = {};
  frame.ID = 0x21A;
  frame.DLC = 5;
  frame.data.u8[4] = status;
  battery.handle_incoming_can_frame(frame);
}

bool has_template_frame() {
  for (const CAN_frame& frame : get_transmitted_frames()) {
    if (frame.ID != 0x7E4) {
      return true;
    }
  }
  return false;
}

class KiaEGmpContactorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(1000);
    clear_transmitted_frames();
    init_events();
    reset_all_events();

    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.inverter_allows_contactor_closing = true;
    datalayer.system.status.battery_allows_contactor_closing = true;
    datalayer.battery.info.number_of_cells = kCellCount;
    for (uint16_t i = 0; i < MAX_AMOUNT_CELLS; ++i) {
      datalayer.battery.status.cell_voltages_mV[i] = 0;
    }
    user_selected_use_estimated_SOC = false;

    battery.setup();
    feed_cell_voltages(battery, 4000);
    feed_contactor_status(battery, kContactorOpen);
    battery.update_values();
  }

  KiaEGmpBattery battery;
};

TEST_F(KiaEGmpContactorTest, ManualOpenSuppressesTemplatesUntilExplicitClose) {
  ASSERT_TRUE(datalayer.system.status.battery_allows_contactor_closing);
  battery.request_open_contactors();
  clear_transmitted_frames();
  battery.transmit_can(1000);
  EXPECT_FALSE(has_template_frame());

  battery.request_close_contactors();
  battery.update_values();
  battery.transmit_can(1100);
  EXPECT_TRUE(has_template_frame());
}

TEST_F(KiaEGmpContactorTest, OutOfRangeCellBlocksTemplatesAndSafeRecoveryResumesThem) {
  feed_cell_voltages(battery, 4000, 0, 2950);
  battery.update_values();
  ASSERT_FALSE(datalayer.system.status.battery_allows_contactor_closing);

  clear_transmitted_frames();
  battery.transmit_can(1000);
  EXPECT_FALSE(has_template_frame());

  set_millis64(1100);
  feed_cell_voltages(battery, 4000, 0, 3000);
  battery.update_values();
  ASSERT_TRUE(datalayer.system.status.battery_allows_contactor_closing);

  battery.transmit_can(1100);
  EXPECT_TRUE(has_template_frame());
}

TEST_F(KiaEGmpContactorTest, StaleCellDataBlocksTemplates) {
  set_millis64(1000 + 5001);
  battery.update_values();
  ASSERT_FALSE(datalayer.system.status.battery_allows_contactor_closing);

  clear_transmitted_frames();
  battery.transmit_can(6001);
  EXPECT_FALSE(has_template_frame());
}

TEST_F(KiaEGmpContactorTest, UnexpectedOpenRaisesEventAndClosedStateClearsIt) {
  battery.update_values();
  EXPECT_EQ(get_event_pointer(EVENT_CONTACTOR_OPEN)->state, EVENT_STATE_ACTIVE);

  feed_contactor_status(battery, kContactorClosed);
  battery.update_values();
  EXPECT_EQ(get_event_pointer(EVENT_CONTACTOR_OPEN)->state, EVENT_STATE_INACTIVE);
}

TEST_F(KiaEGmpContactorTest, OpenEventIsNotRaisedWhenOpeningIsExpected) {
  feed_cell_voltages(battery, 4000, 0, 2900);
  battery.update_values();
  EXPECT_EQ(get_event_pointer(EVENT_CONTACTOR_OPEN)->state, EVENT_STATE_INACTIVE);

  feed_cell_voltages(battery, 4000);
  battery.update_values();
  ASSERT_EQ(get_event_pointer(EVENT_CONTACTOR_OPEN)->state, EVENT_STATE_ACTIVE);
  battery.request_open_contactors();
  EXPECT_EQ(get_event_pointer(EVENT_CONTACTOR_OPEN)->state, EVENT_STATE_INACTIVE);
}

TEST_F(KiaEGmpContactorTest, UnknownContactorStatusBlocksTemplateTraffic) {
  feed_contactor_status(battery, 0x00);
  clear_transmitted_frames();
  battery.transmit_can(1000);
  EXPECT_FALSE(has_template_frame());
}

TEST_F(KiaEGmpContactorTest, AutomaticSoftResetIsQueuedOnlyOnceWhileOpen) {
  battery.transmit_can(1000);
  auto reset_requests = [] {
    size_t count = 0;
    for (const CAN_frame& frame : get_transmitted_frames()) {
      if (frame.ID == 0x7E4 && frame.data.u8[1] == 0x11 && frame.data.u8[2] == 0x03) {
        ++count;
      }
    }
    return count;
  };
  ASSERT_EQ(reset_requests(), 1u);

  for (unsigned long now = 1100; now <= 2200; now += 100) {
    set_millis64(now);
    battery.transmit_can(now);
  }
  EXPECT_EQ(reset_requests(), 1u);
}

}  // namespace
