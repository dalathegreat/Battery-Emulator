#include <gtest/gtest.h>

#include "../../Software/src/battery/NISSAN-LEAF-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include <vector>

#include "../../Software/src/datalayer/datalayer_extended.h"
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

// Group replies are only decoded once the driver is polling for itself, which needs a live
// battery and one pass of the 10 s tick to clear the initial "someone else may be on the bus" hold.
NissanLeafBattery* battery_polling() {
  set_millis64(50000);
  auto battery = new NissanLeafBattery();
  battery->setup();
  // 0x5BC marks the battery alive. Byte 0 gives a plausible GID count so the empty-battery
  // safety path stays out of the way; byte 4 is left at zero so no broadcast SOH is reported.
  battery->handle_incoming_can_frame(leaf_frame(0x5BC, {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
  battery->transmit_can(50000);
  return battery;
}

// Feeds a complete group 0x02 reply carrying the 96 given cell values. The LBC pads the block past
// the last cell, and the driver keys the end of the transfer off that padding.
void feed_cell_voltage_reply(NissanLeafBattery* battery, const uint16_t (&cells)[96]) {
  std::vector<uint8_t> payload = {0x61, 0x02};
  for (uint16_t cell : cells) {
    payload.push_back((uint8_t)(cell >> 8));
    payload.push_back((uint8_t)(cell & 0xFF));
  }
  payload.insert(payload.end(), 8, 0xFF);  // trailing padding, no cell data

  std::initializer_list<uint8_t> unused = {};
  (void)unused;

  CAN_frame first = leaf_7bb_frame({});
  first.data.u8[0] = (uint8_t)(0x10 | ((payload.size() >> 8) & 0x0F));
  first.data.u8[1] = (uint8_t)(payload.size() & 0xFF);
  for (size_t i = 0; i < 6; i++) {
    first.data.u8[2 + i] = payload[i];
  }
  battery->handle_incoming_can_frame(first);

  size_t offset = 6;
  uint8_t sequence = 1;
  while (offset < payload.size()) {
    CAN_frame consecutive = leaf_7bb_frame({});
    consecutive.data.u8[0] = (uint8_t)(0x20 | (sequence & 0x0F));
    for (size_t i = 0; i < 7; i++) {
      consecutive.data.u8[1 + i] = (offset + i < payload.size()) ? payload[offset + i] : 0xFF;
    }
    battery->handle_incoming_can_frame(consecutive);
    offset += 7;
    sequence++;
  }
}

}  // namespace

// The health block is the only group reply longer than 255 bytes, so its first frame announces
// itself as 1L LL. An equality test against 0x10 would never latch the group at all.
TEST(NissanLeafHealthTests, ShouldDecodeHealthBlockFromLongFirstFrame) {
  auto battery = battery_polling();

  // 11 4B 61 61 | Hx 0x2AF8 = 110.00 % | SOH 0x2710 = 100.00 %
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x11, 0x4B, 0x61, 0x61, 0x2A, 0xF8, 0x27, 0x10}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.nissanleaf.battery_HX_pptt, 11000u);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000u);
  EXPECT_TRUE(datalayer.battery.status.soh_available);
}

// Hx above 100 % is a normal reading on a healthy pack and must survive intact.
TEST(NissanLeafHealthTests, ShouldNotClampHxAboveOneHundredPercent) {
  auto battery = battery_polling();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x11, 0x4B, 0x61, 0x61, 0x30, 0xD4, 0x25, 0x8A}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.nissanleaf.battery_HX_pptt, 12500u);  // 0x30D4
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9610u);               // 0x258A
}

// Nothing has been read from the pack yet, so no state of health is invented.
TEST(NissanLeafHealthTests, ShouldReportStateOfHealthUnknownBeforeAnyReading) {
  auto battery = new NissanLeafBattery();
  battery->setup();
  battery->update_values();

  EXPECT_FALSE(datalayer.battery.status.soh_available);
}

// The broadcast value in 0x5BC stands in at whole-percent resolution until the health block
// answers, and is then superseded by it.
TEST(NissanLeafHealthTests, ShouldPreferPolledStateOfHealthOverBroadcast) {
  auto battery = battery_polling();

  battery->handle_incoming_can_frame(leaf_frame(0x5BC, {0x50, 0x00, 0x00, 0x00, 0xBE, 0x00, 0x00, 0x00}));
  battery->update_values();
  EXPECT_TRUE(datalayer.battery.status.soh_available);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9500u);  // 0xBE >> 1 = 95 %

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x11, 0x4B, 0x61, 0x61, 0x2A, 0xF8, 0x25, 0x2C}));
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9516u);  // 0x252C = 95.16 %
}

// A rejected request is a single frame, not group data. Letting it through would leave the
// previously latched group decoding it, which for cell voltages means writing garbage into
// the array.
TEST(NissanLeafHealthTests, ShouldIgnoreNegativeResponse) {
  auto battery = battery_polling();

  battery->handle_incoming_can_frame(leaf_7bb_frame({0x11, 0x4B, 0x61, 0x61, 0x2A, 0xF8, 0x27, 0x10}));
  battery->update_values();
  ASSERT_EQ(datalayer_extended.nissanleaf.battery_HX_pptt, 11000u);

  // requestOutOfRange for service 0x21
  battery->handle_incoming_can_frame(leaf_7bb_frame({0x03, 0x7F, 0x21, 0x31, 0x00, 0x00, 0x00, 0x00}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.nissanleaf.battery_HX_pptt, 11000u);
}

// 0xFFFF is the LBC saying it has no reading for that cell, which is not a measurement of zero
// and must not be treated as 65.535 V either.
TEST(NissanLeafCellTests, ShouldTreatSentinelCellsAsUnknown) {
  auto battery = battery_polling();

  uint16_t cells[96];
  for (uint16_t& cell : cells) {
    cell = 3800;
  }
  cells[0] = 0xFFFF;  // no reading
  cells[5] = 3700;    // genuine minimum
  cells[9] = 3900;    // genuine maximum

  feed_cell_voltage_reply(battery, cells);
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 0u);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[1], 3800u);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3700u);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 3900u);
}

// A pack answering with nothing but sentinels, as a bench BMS with no HV stack does, leaves the
// min/max alone rather than publishing a 0 mV minimum.
TEST(NissanLeafCellTests, ShouldKeepPreviousMinMaxWhenNoCellReports) {
  auto battery = battery_polling();

  uint16_t good[96];
  for (uint16_t& cell : good) {
    cell = 3800;
  }
  good[0] = 3600;
  feed_cell_voltage_reply(battery, good);

  uint16_t none[96];
  for (uint16_t& cell : none) {
    cell = 0xFFFF;
  }
  feed_cell_voltage_reply(battery, none);
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3600u);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 3800u);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 0u);
}

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
