#include <gtest/gtest.h>

#include "../../Software/src/battery/NISSAN-LEAF-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

namespace {

// Builds a 0x7BB reply frame from up to 8 raw bytes.
CAN_frame leaf_7bb_frame(std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = 0x7BB;
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

// Drives a battery up to the point where it is waiting for a DTC reply on 0x7BB. update_values() is
// what consumes the web request and puts 19 02 0E on the wire.
NissanLeafBattery* battery_awaiting_dtc_reply() {
  reset_dtc_state();
  set_millis64(50000);  // Non-zero, so a completed read is distinguishable from "never read"
  auto battery = new NissanLeafBattery();
  battery->setup();
  battery->read_DTC();
  battery->update_values();
  return battery;
}

}  // namespace

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

// A single stored code fits in one ISO-TP frame: 59 02 <mask> then one 4-byte record.
TEST(NissanLeafDtcTests, ShouldParseSingleFrameReply) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xD00000u);  // Renders as U1000
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[0], 0x4E);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
}

// Real LBC capture holding four codes: U1000, P33D7, P33D9 and P33DD. The announced ISO-TP length of
// 0x013 (19 bytes) is what stops the trailing FF padding being parsed as a fifth code.
TEST(NissanLeafDtcTests, ShouldParseMultiFrameReply) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x10, 0x13, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00}));
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x21, 0x4E, 0x33, 0xD7, 0x00, 0x4E, 0x33, 0xD9}));
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x22, 0x00, 0x4E, 0x33, 0xDD, 0x00, 0x4E, 0xFF}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 4);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xD00000u);  // U1000
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[1], 0x33D700u);  // P33D7
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[2], 0x33D900u);  // P33D9
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[3], 0x33DD00u);  // P33DD
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(datalayer.battery.dtc.dtc_status[i], 0x4E);
  }
}

// A healthy pack answers with the 3-byte header and nothing else. That is a successful read of zero
// codes, not a failed read.
TEST(NissanLeafDtcTests, ShouldReportNoDtcsStored) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x03, 0x59, 0x02, 0x4E, 0x00, 0x00, 0x00, 0x00}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
}

// 7F 19 xx means the LBC refused the request outright.
TEST(NissanLeafDtcTests, ShouldFlagFailureOnNegativeResponse) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x03, 0x7F, 0x19, 0x12, 0x00, 0x00, 0x00, 0x00}));

  EXPECT_TRUE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
}

// If nothing ever answers, the read has to give up so the page stops showing it as pending.
TEST(NissanLeafDtcTests, ShouldTimeOutWhenLbcNeverReplies) {
  auto battery = battery_awaiting_dtc_reply();

  set_millis64(50000 + 2500);
  battery->update_values();

  EXPECT_TRUE(datalayer.battery.dtc.dtc_read_failed);
}

// The periodic group polling answers on 0x7BB too, and its first frame carries 0x02 in the byte the
// group handler reads as a group number. The 0x61 service byte is what keeps the two apart, so a
// group reply arriving mid-readout must not be swallowed by the DTC reassembler.
TEST(NissanLeafDtcTests, ShouldNotConsumeGroupReplyAsDtc) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x10, 0x35, 0x61, 0x01, 0xFF, 0xFF, 0xFC, 0x18}));

  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);

  // The genuine DTC reply that follows still parses.
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));

  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xD00000u);
}

// nissan_leaf_dtc.json is keyed by the 5-character short form, so that is what has to end up in the
// data-dtc-code attribute the JavaScript loader matches on.
TEST(NissanLeafDtcTests, ShouldRenderShortNissanCodeAsLookupKey) {
  reset_dtc_state();
  set_millis64(50000);

  datalayer.battery.dtc.dtc_count = 2;
  datalayer.battery.dtc.dtc_codes[0] = 0x33D700;  // P33D7, failure type byte 00
  datalayer.battery.dtc.dtc_status[0] = 0x4E;
  datalayer.battery.dtc.dtc_codes[1] = 0xD0002F;  // U1000, failure type byte 2F
  datalayer.battery.dtc.dtc_status[1] = 0x09;
  datalayer.battery.dtc.dtc_last_read_millis = 50000;

  NissanLeafHtmlRenderer renderer(&datalayer.battery, &datalayer_extended.nissanleaf);
  std::string html = renderer.get_status_html().str();

  EXPECT_NE(html.find("data-dtc-code='P33D7'"), std::string::npos);
  EXPECT_NE(html.find("data-dtc-code='U1000'"), std::string::npos);
  // A set failure type byte is shown to the user, but is never part of the lookup key.
  EXPECT_NE(html.find("U1000-2F"), std::string::npos);
  EXPECT_EQ(html.find("data-dtc-code='U1000-2F'"), std::string::npos);
  EXPECT_NE(html.find("nissan_leaf_dtc.json"), std::string::npos);
}

// The three read states each have to be distinguishable on the page.
TEST(NissanLeafDtcTests, ShouldRenderReadStateWhenNoTableIsShown) {
  NissanLeafHtmlRenderer renderer(&datalayer.battery, &datalayer_extended.nissanleaf);

  reset_dtc_state();  // Never read
  EXPECT_NE(renderer.get_status_html().str().find("Not read yet"), std::string::npos);

  reset_dtc_state();
  datalayer.battery.dtc.dtc_last_read_millis = 50000;
  datalayer.battery.dtc.dtc_read_failed = true;
  EXPECT_NE(renderer.get_status_html().str().find("failed or timed out"), std::string::npos);

  reset_dtc_state();
  datalayer.battery.dtc.dtc_last_read_millis = 50000;
  EXPECT_NE(renderer.get_status_html().str().find("No DTCs present"), std::string::npos);
}
