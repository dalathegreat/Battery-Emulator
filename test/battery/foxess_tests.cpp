#include <gtest/gtest.h>

#include "../../Software/src/battery/FOXESS-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/devboard/hal/hal.h"
#include "../../Software/src/devboard/utils/events.h"

// Regression tests for the FoxESS driver fixes from #1664:
// 1. The driver never refreshed CAN_battery_still_alive, so
//    EVENT_CAN_BATTERY_MISSING fired 60 s after boot regardless of traffic.
// 2. update_values() re-applied the hardcoded HS-series voltage table every
//    cycle, overwriting the limits the BMS itself reports in 0x1872 - packs
//    outside the HS table (EP-series) tripped false BATTERY_OVERVOLTAGE.

class FoxessTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    init_hal();
    battery = new FoxessBattery();
  }

  void TearDown() override { delete battery; }

  FoxessBattery* battery = nullptr;
};

TEST_F(FoxessTest, RecognizedFrameRefreshesStillAliveCounter) {
  datalayer.battery.status.CAN_battery_still_alive = 0;

  // 0x1873 BMS_PackData - part of the BMS's periodic broadcast
  CAN_frame frame = {
      .ID = 0x1873,
      .data = {.u8 = {0x84, 0x0F, 0x00, 0x00, 0x50, 0x00, 0xE8, 0x03}},
  };
  battery->handle_incoming_can_frame(frame);

  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE)
      << "Periodic BMS traffic must refresh the aliveness counter";
}

TEST_F(FoxessTest, BmsReportedLimitsSurviveUpdateValues) {
  // 0x1875 BMS_Status: byte 3 = NUMBER_OF_PACKS = 6 (HS15.6 in the table)
  CAN_frame status_frame = {
      .ID = 0x1875,
      .data = {.u8 = {0x00, 0x00, 0x06, 0x06, 0x01, 0x00, 0x00, 0x00}},
  };
  battery->handle_incoming_can_frame(status_frame);

  // Before any 0x1872: the table is the (fallback) source
  battery->update_values();
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 3504);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2400);

  // 0x1872 BMS_Limits: max 398.0 V (3980 = 0x0F8C), min 296.8 V (2968 = 0x0B98)
  // - an EP-series pack outside the HS table
  CAN_frame limits_frame = {
      .ID = 0x1872,
      .data = {.u8 = {0x8C, 0x0F, 0x98, 0x0B, 0x64, 0x00, 0x64, 0x00}},
  };
  battery->handle_incoming_can_frame(limits_frame);

  battery->update_values();
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 3980)
      << "BMS-reported limits must not be overwritten by the HS-series table";
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2968);
  // The table remains the only source for the cell count
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
}
