#include <gtest/gtest.h>

#include "../../Software/src/battery/NISSAN-LEAF-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

TEST(NissanLeafTests, ShouldReportVoltage) {
  auto battery = new NissanLeafBattery();
  battery->setup();

  int expected_dV = 440;

  int divided = expected_dV / 5;

  CAN_frame frame = {.ID = 0x1DB, .data = {.u8 = {0, 0, (uint8_t)(divided >> 2), (uint8_t)((divided & 0xC0) << 6)}}};

  frame.data.u8[7] = battery->calculate_crc(frame);
  battery->handle_incoming_can_frame(frame);
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, expected_dV);
}
