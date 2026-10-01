#include <gtest/gtest.h>

#include "../../Software/src/battery/VOLVO-SPA-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

namespace {

// Builds a 0x635 UDS reply frame from up to 8 raw bytes.
CAN_frame volvo_635_frame(std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x635;
  uint8_t i = 0;
  for (uint8_t b : bytes) {
    if (i >= 8) {
      break;
    }
    frame.data.u8[i++] = b;
  }
  return frame;
}

// The datalayer is a global shared by every test, so put the DTC block back to its power-on state.
void reset_dtc_state() {
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};
}

// Drives a battery up to the point where it is waiting for a DTC reply on 0x635. update_values() is
// what consumes the web request and puts 02 19 03 on the wire.
VolvoSpaBattery* battery_awaiting_dtc_reply() {
  reset_dtc_state();
  set_millis64(50000);  // Non-zero, so a completed read is distinguishable from "never read"
  auto battery = new VolvoSpaBattery();
  battery->setup();
  battery->read_DTC();
  battery->update_values();
  return battery;
}

}  // namespace

TEST(VolvoSPADtcTests, ShouldParseSingleFrameReply) {
  auto battery = battery_awaiting_dtc_reply();

  //Single frame DTC reply with one active code, PAA06
  battery->handle_incoming_can_frame(volvo_635_frame({0x07, 0x59, 0x02, 0x03, 0x0A, 0x95, 0x00, 0x4E}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0x0A9500u);  // PAA06
}

/*Example of DTC read - Real CAN data from vehicle and scan tool
TX0 735 [8] 02 19 03 00 00 00 00 00
RX1 635 [8] 10 56 59 03 0C EE 00 20
TX0 735 [8] 30 00 05 00 00 00 00 00
RX1 635 [8] 21 0C EE 00 21 0D 15 00
RX1 635 [8] 22 21 0E 0F 00 21 C1 10
RX1 635 [8] 23 00 20 C1 10 00 21 C2
RX1 635 [8] 24 92 00 20 C2 92 00 21
RX1 635 [8] 25 C2 99 00 20 C2 99 00
RX1 635 [8] 26 21 0A 95 00 21 0E EA
RX1 635 [8] 27 00 20 0E EA 00 21 0D
RX1 635 [8] 28 9C 00 20 0D 9C 00 21
RX1 635 [8] 29 C1 00 00 20 C1 00 00
RX1 635 [8] 2A 21 0D 9A 00 20 0D 9A
RX1 635 [8] 2B 00 21 10 68 00 20 10
RX1 635 [8] 2C 68 00 21 00 00 00 00
*/

TEST(VolvoSPADtcTests, ShouldParseMultiFrameReply) {
  auto battery = battery_awaiting_dtc_reply();

  //0x56 = how many bytes are in the reply, 0x0C = how many DTCs are in the reply
  battery->handle_incoming_can_frame(volvo_635_frame({0x10, 0x56, 0x59, 0x03, 0x0C, 0xEE, 0x00, 0x20}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x21, 0x0C, 0xEE, 0x00, 0x21, 0x0D, 0x15, 0x00}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x22, 0x21, 0x0E, 0x0F, 0x00, 0x21, 0xC1, 0x10}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x23, 0x00, 0x20, 0xC1, 0x10, 0x00, 0x21, 0xC2}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x24, 0x92, 0x00, 0x20, 0xC2, 0x92, 0x00, 0x21}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x25, 0xC2, 0x99, 0x00, 0x20, 0xC2, 0x99, 0x00}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x26, 0x21, 0x0A, 0x95, 0x00, 0x21, 0x0E, 0xEA}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x27, 0x00, 0x20, 0x0E, 0xEA, 0x00, 0x21, 0x0D}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x28, 0x9C, 0x00, 0x20, 0x0D, 0x9C, 0x00, 0x21}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x29, 0xC1, 0x00, 0x00, 0x20, 0xC1, 0x00, 0x00}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x2A, 0x21, 0x0D, 0x9A, 0x00, 0x20, 0x0D, 0x9A}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x2B, 0x00, 0x21, 0x10, 0x68, 0x00, 0x20, 0x10}));
  battery->handle_incoming_can_frame(volvo_635_frame({0x2C, 0x68, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0x0CEE00u);  // PO0CEE
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[0], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[1], 0x0CEE00u);  // PO0CEE
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[1], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[2], 0x0D1500u);  // P0E0F
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[2], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[3], 0x0E0F00u);  // U1100
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[3], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[4], 0xC11000u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[4], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[5], 0xC11000u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[5], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[6], 0xC29200u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[6], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[7], 0xC29200u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[7], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[8], 0xC29900u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[8], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[9], 0xC29900u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[9], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[10], 0x0A9500u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[10], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[11], 0x0EEA00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[11], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[12], 0x0EEA00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[12], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[13], 0x0D9C00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[13], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[14], 0x0D9C00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[14], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[15], 0xC10000u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[15], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[16], 0xC10000u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[16], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[17], 0x0D9A00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[17], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[18], 0x0D9A00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[18], 0x21);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[19], 0x106800u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[19], 0x20);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[20], 0x106800u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[20], 0x21);
}
