#include <gtest/gtest.h>

#include <vector>

#include "../../Software/src/battery/BYD-ATTO-3-BATTERY.h"
#include "../../Software/src/battery/NISSAN-LEAF-BATTERY.h"
#include "../../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"
#include "../../Software/src/devboard/hal/hal.h"

#include "Arduino.h"

namespace {

// Builds a frame on the given ID from up to 8 raw bytes.
CAN_frame leaf_frame(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = id;
  uint8_t i = 0;
  for (uint8_t b : bytes) {
    if (i >= 8) {
      break;
    }
    frame.data.u8[i++] = b;
  }
  return frame;
}

CAN_frame leaf_7bb_frame(std::initializer_list<uint8_t> bytes) {
  return leaf_frame(0x7BB, bytes);
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
  battery->transmit_can(50000);  // Channel is idle, so the request goes out immediately
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

// A healthy pack answers with the 3-byte header and nothing else, padded with FF. That is a
// successful read of zero codes, not a failed read.
TEST(NissanLeafDtcTests, ShouldReportNoDtcsStored) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x03, 0x59, 0x02, 0x4E, 0xFF, 0xFF, 0xFF, 0xFF}));

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
}

// The LBC acknowledges ClearDiagnosticInformation with a single-frame 54. Until that arrives the
// previously read list has to stay put, because an unconfirmed erase proves nothing.
TEST(NissanLeafDtcTests, ShouldClearStoredDtcsOnlyOnAcknowledgement) {
  reset_dtc_state();
  set_millis64(50000);
  auto battery = new NissanLeafBattery();
  battery->setup();

  datalayer.battery.dtc.dtc_count = 1;
  datalayer.battery.dtc.dtc_codes[0] = 0x33D700;
  datalayer.battery.dtc.dtc_last_read_millis = 50000;

  battery->reset_DTC();
  battery->transmit_can(50000);  // Sends 14 FF FF FF, but must not wipe anything yet

  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 1);

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x01, 0x54, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));

  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  // Back to "not read yet": the erase says nothing about what the LBC reports next.
  EXPECT_EQ(datalayer.battery.dtc.dtc_last_read_millis, 0u);
}

// An erase the LBC never acknowledges must leave the stored list alone rather than falsely
// reporting success.
TEST(NissanLeafDtcTests, ShouldKeepStoredDtcsWhenEraseIsNotAcknowledged) {
  reset_dtc_state();
  set_millis64(50000);
  auto battery = new NissanLeafBattery();
  battery->setup();

  datalayer.battery.dtc.dtc_count = 1;
  datalayer.battery.dtc.dtc_codes[0] = 0x33D700;
  datalayer.battery.dtc.dtc_last_read_millis = 50000;

  battery->reset_DTC();
  battery->transmit_can(50000);

  set_millis64(50000 + 2500);
  battery->transmit_can(50000 + 2500);

  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_last_read_millis, 50000u);
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

  // Each unanswered attempt is retried; only after the retries are exhausted is it a failure.
  unsigned long t = 50000;
  for (int attempt = 0; attempt < 4; attempt++) {
    t += 2500;
    set_millis64(t);
    battery->transmit_can(t);  // times out this attempt, re-arms if retries remain
    battery->transmit_can(t);  // sends the retry
  }

  EXPECT_TRUE(datalayer.battery.dtc.dtc_read_failed);
}

// Regression for the collision seen on real hardware: pressing Read DTC while a group poll transfer
// was still in flight put 19 02 0E on the bus 1 ms after a flow control frame, and the LBC dropped
// it without answering. The request must instead wait for the channel to go quiet.
TEST(NissanLeafDtcTests, ShouldNotSendRequestWhileGroupTransferIsInFlight) {
  reset_dtc_state();
  set_millis64(60000);
  auto battery = new NissanLeafBattery();
  battery->setup();

  // LBC is mid-transfer: first frame of a group 0x90 reply has just arrived.
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x10, 0x0A, 0x61, 0x90, 0x47, 0x41, 0x51, 0x31}));

  battery->read_DTC();
  battery->transmit_can(60000);  // Channel busy, so nothing should go out yet

  // A DTC reply arriving now would mean the request had been sent. Feed one and check it is ignored.
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);

  // Once the channel has been quiet for longer than the idle threshold, the request goes out.
  set_millis64(60000 + 200);
  battery->transmit_can(60000 + 200);
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));

  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xD00000u);
}

// A request must not go out in the window between an earlier request being sent and its first
// response frame arriving. The channel looks quiet there, but the LBC is still working on the
// previous transaction and will drop whatever lands next.
TEST(NissanLeafDtcTests, ShouldNotSendRequestWhileEarlierRequestIsUnanswered) {
  reset_dtc_state();
  set_millis64(70000);
  auto battery = new NissanLeafBattery();
  battery->setup();

  // Periodic polling only runs once 0x5BC has marked the battery as alive.
  battery->handle_incoming_can_frame(leaf_frame(0x5BC, {0x43, 0xC0, 0xB4, 0x8C, 0xC8, 0x02, 0x5F, 0xFF}));

  // Polling is held off for the first few 10 s cycles after startup, so tick past that until a
  // group request actually goes out. The last tick leaves it outstanding with no reply yet.
  unsigned long t = 70000;
  for (int tick = 0; tick < 5; tick++) {
    battery->transmit_can(t);
    t += 10001;
  }
  battery->transmit_can(t);  // This one puts a group request on the bus

  set_millis64(t + 500);
  battery->read_DTC();
  battery->transmit_can(t + 500);  // Channel is quiet, but that request is still unanswered

  // If the DTC request had gone out, this reply would be accepted.
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
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

// Reproduces a reply larger than our storage: the LBC announced 599 bytes (149 codes) when asked
// with an over-wide status mask. The first 32 codes must be kept, and every remaining frame must
// still be acknowledged, because falling silent mid-transfer leaves the LBC waiting on a flow
// control that never arrives and blocks the next request.
TEST(NissanLeafDtcTests, ShouldDrainReplyLargerThanStorage) {
  auto battery = battery_awaiting_dtc_reply();

  const uint16_t announced = 599;
  battery->handle_incoming_can_frame(leaf_frame(
      0x7BB, {(uint8_t)(0x10 | (announced >> 8)), (uint8_t)(announced & 0xFF), 0x59, 0x02, 0x4E, 0x0A, 0x1F, 0x00}));

  // 6 payload bytes arrived in the first frame; feed consecutive frames until all 599 are sent.
  uint16_t sent = 6;
  uint8_t seq = 1;
  bool checked_midway = false;
  while (sent < announced) {
    CAN_frame cf = leaf_frame(0x7BB, {(uint8_t)(0x20 | (seq & 0x0F))});
    for (uint8_t i = 1; i < 8; i++) {
      cf.data.u8[i] = (sent < announced) ? 0x40 : 0xFF;
      sent++;
    }
    battery->handle_incoming_can_frame(cf);
    seq++;

    // Once our storage is full there is still far more to come. The readout must stay open and keep
    // acknowledging, not declare itself finished the moment the buffer fills.
    if (!checked_midway && sent >= 3 + 4 * DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT) {
      EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0) << "readout ended early instead of draining";
      checked_midway = true;
    }
  }
  EXPECT_TRUE(checked_midway);

  // Completed rather than timed out, and filled to capacity without overrunning it.
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0x0A1F00u);  // P0A1F, first code in the real capture

  // The full count is kept even though only the first 32 are stored, so the page can say the list
  // is truncated. 599 bytes less the 3 byte header is 149 codes.
  EXPECT_EQ(datalayer.battery.dtc.dtc_reported_count, 149);

  NissanLeafHtmlRenderer renderer(&datalayer.battery, &datalayer_extended.nissanleaf);
  EXPECT_NE(renderer.get_status_html().str().find("32 codes shown of 149 reported"), std::string::npos);
}

// When everything fits, the page must not clutter the line with a redundant "of N reported".
TEST(NissanLeafDtcTests, ShouldNotClaimTruncationWhenEverythingFits) {
  auto battery = battery_awaiting_dtc_reply();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x07, 0x59, 0x02, 0x4E, 0xD0, 0x00, 0x00, 0x4E}));

  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_reported_count, 1);

  NissanLeafHtmlRenderer renderer(&datalayer.battery, &datalayer_extended.nissanleaf);
  EXPECT_EQ(renderer.get_status_html().str().find("reported"), std::string::npos);
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

// ---------------------------------------------------------------------------
// BMS shut-down sequence
// ---------------------------------------------------------------------------

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

// Nissan nibble checksum over 0x1F2: nibbles of bytes 0..6 plus the high nibble
// of byte 7, plus 2, anded with 0xF. Recomputed here rather than reusing the
// driver's helper, so a broken implementation cannot validate itself.
uint8_t expected_1f2_checksum(const CAN_frame& frame) {
  uint8_t sum = 0;
  for (uint8_t j = 0; j < 7; j++) {
    sum += (frame.data.u8[j] >> 4) + (frame.data.u8[j] & 0x0F);
  }
  sum += frame.data.u8[7] >> 4;
  return (sum + 2) & 0x0F;
}

// Most recently transmitted frame on the given ID, or nullptr if there was none.
const CAN_frame* last_transmitted(uint32_t id) {
  const std::vector<CAN_frame>& frames = get_transmitted_frames();
  for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
    if (it->ID == id) {
      return &*it;
    }
  }
  return nullptr;
}

// Runs the transmit loop for the given number of milliseconds. When feed_rx is
// true the battery is also kept "talking" on the bus, as it is until the LBC
// acts on the GoToSleep command.
void run_leaf_ms(NissanLeafBattery* leaf, unsigned long ms, bool feed_rx) {
  CAN_frame alive = leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0});
  for (unsigned long i = 0; i < ms; i++) {
    set_millis64(millis() + 1);
    if (feed_rx && (millis() % 10 == 0)) {
      leaf->handle_incoming_can_frame(alive);
    }
    leaf->transmit_can(millis());
  }
}

}  // namespace

/* Walks the whole shut-down sequence and checks the signal values the LBC sees at each
   step, that the frames stay checksum/CRC valid while the driver rewrites them, that
   transmission really stops before power removal is allowed, and that normal contents
   come back afterwards. Signal positions per the GEN4 spec and the LEAF DBC:
   CHG_STA_RQ = 0x1F2 byte 2 bits 5-6, BTONFN = 0x1D4 byte 4 bit 2,
   RLYP = 0x1D4 byte 5 bit 6, VCM_WakeUpSleepCommand = 0x50B byte 3 bits 6-7. */
TEST(NissanLeafShutdownSequenceTests, ShouldFollowSpecifiedSequenceBeforePowerRemoval) {
  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));

  // Baseline: normal operation, relays commanded on and the BMS told to stay awake.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 300, true);
  const CAN_frame* frame_1d4 = last_transmitted(0x1D4);
  const CAN_frame* frame_1f2 = last_transmitted(0x1F2);
  const CAN_frame* frame_50b = last_transmitted(0x50B);
  ASSERT_NE(frame_1d4, nullptr);
  ASSERT_NE(frame_1f2, nullptr);
  ASSERT_NE(frame_50b, nullptr);
  EXPECT_EQ(frame_1d4->data.u8[4] & 0x04, 0x04);  // BTONFN=1
  EXPECT_EQ(frame_1d4->data.u8[5] & 0x40, 0x40);  // RLYP=1
  EXPECT_EQ(frame_1f2->data.u8[2] & 0x60, 0x00);  // CHG_STA_RQ=00b, the value this build boots with
  EXPECT_EQ(frame_1f2->data.u8[7] & 0x0F, expected_1f2_checksum(*frame_1f2));
  EXPECT_EQ(frame_50b->data.u8[3] & 0xC0, 0xC0);  // WakeUp

  // The reset state machine pauses the battery and asks for the sequence.
  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  leaf.request_bms_shutdown_sequence();
  EXPECT_FALSE(leaf.bms_shutdown_sequence_completed());

  /* The charge stop request and both relay-off signals are applied together, then held for
     10 ms before the ignition drops and the sleep command follows. 0x1D4 and 0x1F2 go out
     every 10 ms; 0x50B every 100 ms, so it is only checked once the sequence has reached the
     step that changes it. */

  // Step 1: charge stop request and both relays off, in the same frame batch.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 5, true);
  frame_1f2 = last_transmitted(0x1F2);
  frame_1d4 = last_transmitted(0x1D4);
  ASSERT_NE(frame_1f2, nullptr);
  ASSERT_NE(frame_1d4, nullptr);
  EXPECT_EQ(frame_1f2->data.u8[2] & 0x60, 0x60);  // CHG_STA_RQ=11b
  EXPECT_EQ(frame_1f2->data.u8[7] & 0x0F, expected_1f2_checksum(*frame_1f2));
  EXPECT_EQ(frame_1d4->data.u8[4] & 0x04, 0x00);  // BTONFN=0b
  EXPECT_EQ(frame_1d4->data.u8[5] & 0x40, 0x00);  // RLYP=0b, no wait between the two
  CAN_frame crc_check = *frame_1d4;
  EXPECT_EQ(frame_1d4->data.u8[7], leaf.calculate_crc(crc_check));

  // Step 2: GoToSleep, still transmitted while the LBC is on the bus.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 200, true);
  frame_50b = last_transmitted(0x50B);
  ASSERT_NE(frame_50b, nullptr);
  EXPECT_EQ(frame_50b->data.u8[3] & 0xC0, 0x00);
  // The relay-off values are not undone by the sleep step.
  frame_1d4 = last_transmitted(0x1D4);
  ASSERT_NE(frame_1d4, nullptr);
  EXPECT_EQ(frame_1d4->data.u8[4] & 0x04, 0x00);
  EXPECT_EQ(frame_1d4->data.u8[5] & 0x40, 0x00);
  EXPECT_FALSE(leaf.bms_shutdown_sequence_completed());

  // Step 5: once the LBC has been quiet for more than a second, we stop too.
  run_leaf_ms(&leaf, 1200, false);
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 500, false);
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_FALSE(leaf.bms_shutdown_sequence_completed());  // still inside the 1 min wait

  // Step 6: after the wait the power may be cut, and the bus stayed silent throughout.
  run_leaf_ms(&leaf, 61500, false);
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_TRUE(leaf.bms_shutdown_sequence_completed());

  // Once the reset is over the normal message contents come back by themselves.
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 300, true);
  frame_1d4 = last_transmitted(0x1D4);
  frame_1f2 = last_transmitted(0x1F2);
  frame_50b = last_transmitted(0x50B);
  ASSERT_NE(frame_1d4, nullptr);
  ASSERT_NE(frame_1f2, nullptr);
  ASSERT_NE(frame_50b, nullptr);
  EXPECT_EQ(frame_1d4->data.u8[4] & 0x04, 0x04);
  EXPECT_EQ(frame_1d4->data.u8[5], 0x46);
  EXPECT_EQ(frame_1f2->data.u8[2] & 0x60, 0x20);  // alternated to 01b by the reset, and not left at 11b
  EXPECT_EQ(frame_1f2->data.u8[7] & 0x0F, expected_1f2_checksum(*frame_1f2));
  EXPECT_EQ(frame_50b->data.u8[3], 0xC0);
  crc_check = *frame_1d4;
  EXPECT_EQ(frame_1d4->data.u8[7], leaf.calculate_crc(crc_check));
}

/* A battery that does not implement the hook must be left exactly as before: the base
   class reports no sequence and reports completion immediately, so the reset state
   machine never enters the extra state for it. */
TEST(NissanLeafShutdownSequenceTests, ShouldNotAffectBatteriesWithoutTheHook) {
  BydAttoBattery other;
  EXPECT_FALSE(other.supports_bms_shutdown_sequence());
  EXPECT_TRUE(other.bms_shutdown_sequence_completed());
}

// ---------------------------------------------------------------------------
// BMS ignition line during the shut-down sequence
// ---------------------------------------------------------------------------

// Defined in comm_contactorcontrol.cpp; not in its header, as nothing outside the
// reset state machine drives these lines in firmware.
void bms_power_on();
void bms_power_off();

namespace {

// The test suite builds for a board without a separate ignition line, so stand in a HAL
// that has one. GPIO numbers match the Waveshare board, which is the first board wired
// this way.
class IgnitionHal : public Esp32Hal {
 public:
  const char* name() { return "Ignition line test HAL"; }
  gpio_num_t BMS_POWER() { return GPIO_NUM_6; }
  gpio_num_t BMS_IGNIT() { return GPIO_NUM_7; }
  std::vector<gpio_num_t> reset_hold_pins() { return {GPIO_NUM_6, GPIO_NUM_7}; }
  std::vector<comm_interface> available_interfaces() { return {comm_interface::CanNative}; }
};

}  // namespace

/* The spec puts IGN OFF ahead of the charge stop request, so the ignition line has to be
   down before the sequence transmits anything, and has to come back up with the power. */
/* NDS 293A0NDS25 5.1.2 step 3 stops the pack controller by turning IGN off and then sending
   the sleep command, with the earlier relay-off messages still transmitted while IGN is on.
   So the line must stay up through the first three steps and drop at the fourth. */
TEST(NissanLeafShutdownSequenceTests, ShouldDropIgnitionLineWithTheSleepCommand) {
  Esp32Hal* saved_hal = esp32hal;
  IgnitionHal ignition_hal;
  esp32hal = &ignition_hal;

  const bool saved_periodic = periodic_bms_reset;
  const bool saved_remote = remote_bms_reset;
  remote_bms_reset = true;  // One of the two settings that puts the BMS lines under control

  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));
  run_leaf_ms(&leaf, 200, true);

  clear_pin_writes();
  clear_transmitted_frames();

  // Requesting the sequence leaves both lines alone for now.
  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  leaf.request_bms_shutdown_sequence();
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), -1);  // ignition not touched yet

  // While the charge stop request and the relay-off signals are being sent, ignition stays up.
  run_leaf_ms(&leaf, 5, true);
  const CAN_frame* frame_1d4 = last_transmitted(0x1D4);
  ASSERT_NE(frame_1d4, nullptr);
  EXPECT_EQ(frame_1d4->data.u8[4] & 0x04, 0x00);  // BTONFN=0b already sent
  EXPECT_EQ(frame_1d4->data.u8[5] & 0x40, 0x00);  // RLYP=0b already sent
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), -1);       // and IGN is still untouched

  // Reaching the sleep step drops it, and the sleep command goes out in the same step.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 10, true);
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), LOW);
  EXPECT_NE(get_pin_level(GPIO_NUM_6), LOW);  // BAT line still up
  const CAN_frame* frame_50b = last_transmitted(0x50B);
  ASSERT_NE(frame_50b, nullptr);
  EXPECT_EQ(frame_50b->data.u8[3] & 0xC0, 0x00);  // GoToSleep

  // Restoring power brings the ignition line back up, or the next reset would find it
  // already low and the LBC would never see the transition.
  bms_power_on();
  EXPECT_EQ(get_pin_level(GPIO_NUM_6), HIGH);
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), HIGH);

  // A power cut with no sequence in front of it still takes IGN down first.
  bms_power_off();
  EXPECT_EQ(get_pin_level(GPIO_NUM_6), LOW);
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), LOW);

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  periodic_bms_reset = saved_periodic;
  remote_bms_reset = saved_remote;
  esp32hal = saved_hal;
}

/* Neither line is driven unless one of the two BMS reset settings is on, so a user who
   has both switched off keeps the pins free for anything else. */
TEST(NissanLeafShutdownSequenceTests, ShouldLeaveIgnitionLineAloneWhenResetsAreDisabled) {
  Esp32Hal* saved_hal = esp32hal;
  IgnitionHal ignition_hal;
  esp32hal = &ignition_hal;

  const bool saved_periodic = periodic_bms_reset;
  const bool saved_remote = remote_bms_reset;
  periodic_bms_reset = false;
  remote_bms_reset = false;

  clear_pin_writes();
  bms_ignit_off();
  bms_ignit_on();
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), -1);  // never written

  periodic_bms_reset = saved_periodic;
  remote_bms_reset = saved_remote;
  esp32hal = saved_hal;
}

/* If the BMS leaves the bus part way through the sequence there is nobody left to receive
   the rest of it, so transmission stops there instead of marching through the remaining
   steps. Covers the case that matters most in practice: a reset started on a BMS that has
   already stopped talking. */
TEST(NissanLeafShutdownSequenceTests, ShouldStopTransmittingWhenBmsGoesQuietMidSequence) {
  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));
  run_leaf_ms(&leaf, 200, true);

  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  leaf.request_bms_shutdown_sequence();

  // BMS drops off the bus 10 ms in, before the sequence reaches its later steps.
  run_leaf_ms(&leaf, 10, true);
  run_leaf_ms(&leaf, 1000, false);

  // Still transmitting: the BMS has not been quiet long enough to call it stopped.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 50, false);
  EXPECT_FALSE(get_transmitted_frames().empty());

  // Past the silence threshold it gives up, well before the GoToSleep timeout would fire.
  run_leaf_ms(&leaf, 100, false);
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 500, false);
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_FALSE(leaf.bms_shutdown_sequence_completed());  // still inside the wait before BAT OFF

  run_leaf_ms(&leaf, 61500, false);
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_TRUE(leaf.bms_shutdown_sequence_completed());

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
}

/* The ignition line has to be down before the sleep command reaches the wire, not merely
   in the same step: NDS 293A0NDS25 5.1.2 orders IGN off (3.1) ahead of the command (3.2).
   The two land in the same millisecond, so this pins the ordering inside that tick. */
TEST(NissanLeafShutdownSequenceTests, ShouldDropIgnitionBeforeAnySleepCommandReachesTheBus) {
  Esp32Hal* saved_hal = esp32hal;
  IgnitionHal ignition_hal;
  esp32hal = &ignition_hal;

  const bool saved_periodic = periodic_bms_reset;
  const bool saved_remote = remote_bms_reset;
  remote_bms_reset = true;

  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));
  run_leaf_ms(&leaf, 200, true);

  clear_pin_writes();
  clear_transmitted_frames();
  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  leaf.request_bms_shutdown_sequence();

  int sleep_frames_sent_with_ignition_up = 0;
  CAN_frame alive = leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0});
  for (unsigned long i = 0; i < 200; i++) {
    set_millis64(millis() + 1);
    if (millis() % 10 == 0) {
      leaf.handle_incoming_can_frame(alive);
    }
    const size_t before = get_transmitted_frames().size();
    leaf.transmit_can(millis());
    for (size_t k = before; k < get_transmitted_frames().size(); k++) {
      const CAN_frame& frame = get_transmitted_frames()[k];
      if (frame.ID == 0x50B && (frame.data.u8[3] & 0xC0) == 0x00 && get_pin_level(GPIO_NUM_7) != LOW) {
        sleep_frames_sent_with_ignition_up++;
      }
    }
  }
  EXPECT_EQ(sleep_frames_sent_with_ignition_up, 0);
  EXPECT_EQ(get_pin_level(GPIO_NUM_7), LOW);

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
  periodic_bms_reset = saved_periodic;
  remote_bms_reset = saved_remote;
  esp32hal = saved_hal;
}

/* The core loop samples currentMillis once per iteration and hands that same value to every
   transmitter, while the reset trigger and the CAN receive path both read millis() live. So
   transmit_can() can legitimately be called with a currentMillis a millisecond or two BEHIND
   the phase start or the last receive timestamp. Unsigned subtraction wraps there, which
   collapsed the first step to nothing and could have ended the CAN part of the sequence
   before it began. */
TEST(NissanLeafShutdownSequenceTests, ShouldNotCollapseStepsWhenTimestampsRunAhead) {
  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  CAN_frame alive = leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0});
  leaf.handle_incoming_can_frame(alive);
  run_leaf_ms(&leaf, 200, true);

  // The reset path stamps the phase with a live millis(), 2 ms ahead of the sample the
  // transmit loop is still holding.
  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  set_millis64(millis() + 2);
  leaf.request_bms_shutdown_sequence();

  /* Run the loop the way the firmware does: every transmitter gets the currentMillis
     sampled at the top of the iteration, while receives keep stamping live millis(). */
  const unsigned long start = millis();
  long sleep_sent_at = -1;
  for (int i = 0; i < 300; i++) {
    set_millis64(millis() + 1);
    if (millis() % 10 == 0) {
      leaf.handle_incoming_can_frame(alive);
    }
    clear_transmitted_frames();
    leaf.transmit_can(millis() - 2);
    const CAN_frame* frame_50b = last_transmitted(0x50B);
    if (sleep_sent_at < 0 && frame_50b != nullptr && (frame_50b->data.u8[3] & 0xC0) == 0x00) {
      sleep_sent_at = (long)(millis() - start);
    }
  }

  /* The sleep step must wait out the dwell rather than firing on the first call. The relay-off
     signals go out immediately now, so the sleep command is what marks the transition. */
  ASSERT_GE(sleep_sent_at, 0);
  EXPECT_GE(sleep_sent_at, 8);

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
}

/* The lifetime charge counters and the identity strings drop out of the poll rotation once
   they answer, on the grounds that they cannot change while the pack is powered. A BMS reset
   power cycles the pack, so they have to go back into the rotation when it finishes — while
   the previously read values stay on the page until fresh ones arrive. */
namespace {

// Requests go out on 0x79B with the group in byte 2.
bool group_was_requested(uint8_t group) {
  for (const CAN_frame& frame : get_transmitted_frames()) {
    if (frame.ID == 0x79B && frame.data.u8[2] == group) {
      return true;
    }
  }
  return false;
}

// First frame of the 0x62 reply: two AC charges, three quick charges.
CAN_frame charge_counter_reply(uint16_t ac, uint16_t qc) {
  return leaf_frame(0x7BB, {0x10, 0x76, 0x61, 0x62, (uint8_t)(ac >> 8), (uint8_t)(ac & 0xFF), (uint8_t)(qc >> 8),
                            (uint8_t)(qc & 0xFF)});
}

}  // namespace

TEST(NissanLeafShutdownSequenceTests, ShouldRepollChargeCountersAfterABmsReset) {
  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));
  /* stop_battery_query starts set and is only cleared by the 10s block in transmit_can, so
     replies arriving before the driver has polled once are ignored by design. */
  run_leaf_ms(&leaf, 11000, true);

  // Answer the charge counter group once; it then drops out of the rotation.
  leaf.handle_incoming_can_frame(charge_counter_reply(2, 3));
  leaf.update_values();
  EXPECT_EQ(datalayer_extended.nissanleaf.ChargeCountL1L2, 2);
  EXPECT_EQ(datalayer_extended.nissanleaf.ChargeCountQC, 3);

  clear_transmitted_frames();
  run_leaf_ms(&leaf, 30000, true);
  EXPECT_FALSE(group_was_requested(0x62)) << "answered group should have left the rotation";

  // Run a reset through to completion.
  datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
  leaf.request_bms_shutdown_sequence();
  run_leaf_ms(&leaf, 200, true);
  datalayer.system.status.bms_reset_status = BMS_RESET_POWERED_OFF;
  run_leaf_ms(&leaf, 200, false);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  // The group is asked for again, and the page still shows the previous reading meanwhile.
  clear_transmitted_frames();
  run_leaf_ms(&leaf, 30000, true);
  EXPECT_TRUE(group_was_requested(0x62)) << "reset should put the group back in the rotation";
  leaf.update_values();
  EXPECT_EQ(datalayer_extended.nissanleaf.ChargeCountL1L2, 2) << "page must not blink back to zero";

  // A fresh reply replaces it.
  leaf.handle_incoming_can_frame(charge_counter_reply(4, 5));
  leaf.update_values();
  EXPECT_EQ(datalayer_extended.nissanleaf.ChargeCountL1L2, 4);
  EXPECT_EQ(datalayer_extended.nissanleaf.ChargeCountQC, 5);
}

/* CHG_STA_RQ is alternated between 00b and 01b on each BMS reset so both values can be tried
   against the same pack without reflashing. It boots at 00b and is not persisted. */
TEST(NissanLeafShutdownSequenceTests, ShouldAlternateChgStaRqOnEachReset) {
  set_millis64(1000);
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  NissanLeafBattery leaf;
  leaf.setup();
  leaf.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));

  auto transmitted_chg_sta_rq = [&]() {
    clear_transmitted_frames();
    run_leaf_ms(&leaf, 30, true);
    const CAN_frame* frame = last_transmitted(0x1F2);
    EXPECT_NE(frame, nullptr);
    // The checksum has to stay valid whichever value is in use.
    EXPECT_EQ(frame->data.u8[7] & 0x0F, expected_1f2_checksum(*frame));
    return (uint8_t)((frame->data.u8[2] & 0x60) >> 5);
  };

  auto run_a_reset = [&]() {
    datalayer.system.status.bms_reset_status = BMS_RESET_SHUTDOWN_SEQUENCE;
    leaf.request_bms_shutdown_sequence();
    run_leaf_ms(&leaf, 200, true);
    datalayer.system.status.bms_reset_status = BMS_RESET_POWERED_OFF;
    run_leaf_ms(&leaf, 100, false);
    datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
    run_leaf_ms(&leaf, 30, true);
  };

  EXPECT_EQ(transmitted_chg_sta_rq(), 0x00) << "boots at 00b";
  run_a_reset();
  EXPECT_EQ(transmitted_chg_sta_rq(), 0x01) << "first reset switches to 01b";
  run_a_reset();
  EXPECT_EQ(transmitted_chg_sta_rq(), 0x00) << "second reset switches back to 00b";
  run_a_reset();
  EXPECT_EQ(transmitted_chg_sta_rq(), 0x01) << "and keeps alternating";

  // A fresh driver starts from 00b again: the value is deliberately not persisted.
  NissanLeafBattery rebooted;
  rebooted.setup();
  rebooted.handle_incoming_can_frame(leaf_frame(0x5BC, {0, 0, 0, 0, 0, 0, 0, 0}));
  clear_transmitted_frames();
  run_leaf_ms(&rebooted, 30, true);
  const CAN_frame* after_boot = last_transmitted(0x1F2);
  ASSERT_NE(after_boot, nullptr);
  EXPECT_EQ(after_boot->data.u8[2] & 0x60, 0x00);

  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;
}
