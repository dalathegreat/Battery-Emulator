#include <gtest/gtest.h>

#include "../../Software/src/battery/FORD-MACH-E-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

namespace {

    // Builds a 0x7EC reply frame from up to 8 raw bytes.
CAN_frame ford_7ec_frame(std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x7EC;
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

// Drives a battery up to the point where it is waiting for a DTC reply on 0x7EC. update_values() is
// what consumes the web request and puts 19 02 0E on the wire.
FordMachEBattery* battery_awaiting_dtc_reply() {
  reset_dtc_state();
  set_millis64(50000);  // Non-zero, so a completed read is distinguishable from "never read"
  auto battery = new FordMachEBattery();
  battery->setup();
  battery->read_DTC();
  battery->update_values();
  return battery;
}

}  // namespace

/*Example of DTC read
  //(21.90) RX0 7E4 [8] 03 19 02 8F 00 00 00 00
  //(21.92) RX0 7EC [8] 10 2F 59 02 FF C1 9B 00
  //(21.92) RX0 7E4 [8] 30 00 00 00 00 00 00 00
  //(21.92) RX0 7EC [8] 21 AF C1 00 00 2F C2 93
  //(21.93) RX0 7EC [8] 22 00 AF C2 98 00 AF 1A
  */

// Real BMS capture holding four codes: U019B, U0100, U0293 and U0298. The announced ISO-TP length of
// 0x02F (47 bytes) is telling us that there are more frames than this, but this is all the data we have
TEST(FordMachEDtcTests, ShouldParseMultiFrameReply) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(ford_7ec_frame({0x10, 0x2F, 0x59, 0x02, 0xFF, 0xC1, 0x9B, 0x00}));
  battery->handle_incoming_can_frame(ford_7ec_frame({0x21, 0xAF, 0xC1, 0x00, 0x00, 0x2F, 0xC2, 0x93}));
  battery->handle_incoming_can_frame(ford_7ec_frame({0x22, 0x00, 0xAF, 0xC2, 0x98, 0x00, 0xAF, 0x1A}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 4);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xC19B00u);  // U019B
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[1], 0xC10000u);  // U0100
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[2], 0xC29300u);  // U0293
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[3], 0xC29800u);  // U0298
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(datalayer.battery.dtc.dtc_status[i], 0x4E);
  }
}