#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "../../Software/src/battery/UdsCanBattery.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// TX frame capture injected by the emulated CAN layer (see emul/can.cpp).
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

// ---------------------------------------------------------------------------
// Test double: a minimal UdsCanBattery subclass that records everything the
// superclass hands to its virtual hooks and exposes the protected UDS API so
// tests can drive it directly.
// ---------------------------------------------------------------------------
class TestUdsBattery : public UdsCanBattery {
 public:
  using UdsCanBattery::handle_incoming_uds_can_frame;
  using UdsCanBattery::pause_uds;
  using UdsCanBattery::send_sequence_message;
  using UdsCanBattery::set_pid_scan_list;
  using UdsCanBattery::setup_uds;
  using UdsCanBattery::start_sequence;
  using UdsCanBattery::transmit_uds_can;

  struct PidCall {
    uint16_t pid;
    uint32_t value;
    std::vector<uint8_t> data;
    uint16_t length;
  };
  struct SeqCall {
    uint16_t state;
    uint8_t sid;
    std::vector<uint8_t> data;
  };

  TestUdsBattery() : UdsCanBattery() {}

  // --- Battery interface --------------------------------------------------
  void setup() override { setup_uds(0x79B, 0x7BB); }  // Requests to 0x79B, replies from 0x7BB
  void update_values() override {}
  void handle_incoming_can_frame(CAN_frame rx_frame) override { handle_incoming_uds_can_frame(rx_frame); }
  void transmit_can(unsigned long currentMillis) override { transmit_uds_can(currentMillis); }

  // --- UDS virtual hooks --------------------------------------------------
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override {
    pid_calls.push_back({pid, value, std::vector<uint8_t>(data, data + length), length});
    uint16_t detour = detour_pid;  // 0 = continue the scan list in order
    detour_pid = 0;                // One-shot: the scan list resumes afterwards.
    return detour;
  }

  void on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) override {
    seq_calls.push_back(
        {state, sid, data != nullptr ? std::vector<uint8_t>(data, data + len) : std::vector<uint8_t>()});
  }

  void on_uds_sequence_timeout(uint16_t state) override { seq_timeouts.push_back(state); }

  String get_uds_info_html() override { return String("<h4>TEST-INFO</h4>"); }

  // --- Test hooks ---------------------------------------------------------
  uint16_t detour_pid = 0;

  std::vector<PidCall> pid_calls;
  std::vector<SeqCall> seq_calls;
  std::vector<uint16_t> seq_timeouts;
};

// ---------------------------------------------------------------------------
// Frame builders
// ---------------------------------------------------------------------------

CAN_frame make_frame(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  uint8_t i = 0;
  for (uint8_t b : bytes) {
    if (i >= 8) {
      break;
    }
    frame.data.u8[i++] = b;
  }
  return frame;
}

// Builds a complete single-frame ISO-TP message (PCI byte + payload, 0x55 padding).
CAN_frame uds_single_frame(uint32_t id, std::initializer_list<uint8_t> uds_message) {
  std::vector<uint8_t> bytes;
  bytes.push_back(static_cast<uint8_t>(uds_message.size()));  // SF PCI byte
  for (uint8_t b : uds_message) {
    bytes.push_back(b);
  }
  while (bytes.size() < 8) {
    bytes.push_back(0x55);
  }
  return make_frame(id, {bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]});
}

// The PID carried by a transmitted 0x22 request frame (bytes 2-3 after the PCI/SID bytes).
uint16_t requested_pid(const CAN_frame& frame) {
  return static_cast<uint16_t>((frame.data.u8[2] << 8) | frame.data.u8[3]);
}

const CAN_frame& last_frame(const std::vector<CAN_frame>& frames) {
  return frames.back();
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class UdsCanBatteryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    clear_transmitted_frames();
    datalayer = DataLayer();
    set_millis64(1000);  // Non-zero start so "never read" (0) stays distinguishable.
    battery = new TestUdsBattery();
    battery->setup();
  }

  void TearDown() override { delete battery; }

  // Advance the emulated clock to `ms` and run one UDS transmit tick.
  void tick(unsigned long ms) {
    set_millis64(ms);
    battery->transmit_uds_can(ms);
  }

  // Feeds a complete single-frame UDS response as it would arrive from the BMS.
  void feed_response(std::initializer_list<uint8_t> uds_message, uint32_t id = 0x7BB) {
    battery->handle_incoming_can_frame(uds_single_frame(id, uds_message));
  }

  TestUdsBattery* battery = nullptr;
};

// ---------------------------------------------------------------------------
// PID scan: request generation
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, PidScanRequestsPidsInOrderAndWraps) {
  const uint16_t pids[] = {0x9001, 0x9002, 0x9003};
  battery->set_pid_scan_list(pids, 3);

  // First request: single-frame ISO-TP [0x03, 0x22, hi, lo] to 0x79B.
  tick(1000);
  // Check the transmitted frame is what we expect: [79B] 0x03 0x22 0x90 0x01
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).ID, 0x79Bu);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[0], 0x03);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x22);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);

  // Reply with the value for 0x9001 and check the scan moves on.
  feed_response({0x62, 0x90, 0x01, 0x12, 0x34});
  ASSERT_EQ(battery->pid_calls.size(), 1u);

  tick(1100);
  // Check the next request is for 0x9002.
  ASSERT_EQ(get_transmitted_frames().size(), 2u);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);

  feed_response({0x62, 0x90, 0x02, 0x00, 0x64});
  tick(1200);
  // Check the next request is for 0x9003.
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9003);

  feed_response({0x62, 0x90, 0x03, 0x01, 0x00});
  tick(1300);
  // Check the list wraps around to the beginning.
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);
}

TEST_F(UdsCanBatteryTest, PidScanIsIdleWithoutAList) {
  tick(1000);
  tick(1100);
  // No list was set, so no requests should have been sent.
  EXPECT_TRUE(get_transmitted_frames().empty());
  // and the battery shouldn't be busy
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(UdsCanBatteryTest, SetupUdsRestartsTheScanAtTheFirstPid) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);
  feed_response({0x62, 0x90, 0x01, 0xAA});  // Index now points at 0x9002.
  tick(1100);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);

  battery->setup_uds(0x79B, 0x7BB);  // e.g. re-initialised on boot.

  // The 0x9002 request that was in flight still has to time out (2 ticks)
  // before the reset scan position is used.
  tick(1200);  // 2->1, still busy
  tick(1300);  // timeout -> retry from index 0
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);
}

// ---------------------------------------------------------------------------
// PID scan: response decoding
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, PidValueIsParsedBigEndian) {
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  tick(1000);
  feed_response({0x62, 0x90, 0x01, 0x12, 0x34, 0x56});

  ASSERT_EQ(battery->pid_calls.size(), 1u);
  EXPECT_EQ(battery->pid_calls[0].pid, 0x9001);
  // The value should be big endian
  EXPECT_EQ(battery->pid_calls[0].value, 0x123456u);
  ASSERT_EQ(battery->pid_calls[0].length, 3u);
  EXPECT_EQ(battery->pid_calls[0].data, (std::vector<uint8_t>{0x12, 0x34, 0x56}));
}

// A value longer than four bytes is truncated to the first four (big-endian).
TEST_F(UdsCanBatteryTest, PidValueLongerThanFourBytesIsTruncated) {
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  tick(1000);
  // Multi-frame ISO-TP response carrying an 8-byte UDS message (SID + DID + 5 value bytes).
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x10, 0x08, 0x62, 0x90, 0x01, 0x01, 0x02, 0x03}));
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x21, 0x04, 0x05, 0x55, 0x55, 0x55, 0x55, 0x55}));

  ASSERT_EQ(battery->pid_calls.size(), 1u);
  EXPECT_EQ(battery->pid_calls[0].pid, 0x9001);
  // The value should be big endian, truncated to 4 bytes.
  EXPECT_EQ(battery->pid_calls[0].value, 0x01020304u);
  EXPECT_EQ(battery->pid_calls[0].length, 5u);
}

// A handle_pid() return value is requested once, out of sequence, before the scan resumes.
TEST_F(UdsCanBatteryTest, DetourPidIsRequestedBeforeScanResumes) {
  const uint16_t pids[] = {0x9001, 0x9002, 0x9003};
  battery->set_pid_scan_list(pids, 3);
  battery->detour_pid = 0x9007;  // Sample the fast-changing PID whenever 0x9001 is answered.

  tick(1000);
  feed_response({0x62, 0x90, 0x01, 0x00, 0x01});
  tick(1100);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9007);

  feed_response({0x62, 0x90, 0x07, 0x00, 0x02});
  tick(1200);
  // Check the scan resumed where it left off.
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);

  feed_response({0x62, 0x90, 0x02, 0x00, 0x03});
  tick(1300);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9003);
}

// A negative response skips the PID without bothering the handler and moves on.
TEST_F(UdsCanBatteryTest, NegativePidResponseSkipsToNextPid) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);
  feed_response({0x7F, 0x22, 0x31});  // RequestOutOfRange

  // Check the failed request was swallowed.
  EXPECT_TRUE(battery->pid_calls.empty());
  tick(1100);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);
}

// ResponsePending (0x78) must not finish the transaction: the real response that
// follows is still consumed by the same PID request.
TEST_F(UdsCanBatteryTest, ResponsePendingKeepsWaitingForTheRealResponse) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);
  feed_response({0x7F, 0x22, 0x78});  // RCR-RP

  EXPECT_TRUE(battery->pid_calls.empty());
  // No new request is sent on the next tick, ie. the transaction is still in flight
  tick(1100);
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  // The real response completes the original request.
  feed_response({0x62, 0x90, 0x01, 0xAB, 0xCD});
  ASSERT_EQ(battery->pid_calls.size(), 1u);
  EXPECT_EQ(battery->pid_calls[0].pid, 0x9001);
  EXPECT_EQ(battery->pid_calls[0].value, 0xABCDu);
}

// A malformed positive response (missing the DID) keeps the transaction waiting.
TEST_F(UdsCanBatteryTest, MalformedPidResponseIsNotConsumed) {
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  tick(1000);
  feed_response({0x62, 0x90});  // SID + one byte: no full DID.

  EXPECT_TRUE(battery->pid_calls.empty());
  tick(1100);
  // Transaction still in flight: no new request yet.
  EXPECT_EQ(get_transmitted_frames().size(), 1u);
}

// A stale response for an earlier PID (e.g. delivered late, after the scan has
// moved on) must not be consumed as the answer to the PID currently in flight.
TEST_F(UdsCanBatteryTest, LateResponseForAnotherPidIsNotConsumed) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);  // Request 0x9001.
  // Let 0x9001 time out 10 times so the scan advances to 0x9002 unanswered.
  for (int k = 1; k <= 21; k++) {
    tick(1000 + 100 * k);
  }
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);  // 0x9002 is now pending.
  EXPECT_TRUE(battery->pid_calls.empty());

  // A late response for 0x9001 arrives: it must NOT complete the 0x9002
  // transaction nor notify the handler.
  feed_response({0x62, 0x90, 0x01, 0xAA, 0xBB});
  EXPECT_TRUE(battery->pid_calls.empty());

  // The real response for 0x9002 is still accepted.
  feed_response({0x62, 0x90, 0x02, 0xCC, 0xDD});
  ASSERT_EQ(battery->pid_calls.size(), 1u);
  EXPECT_EQ(battery->pid_calls[0].pid, 0x9002);
  EXPECT_EQ(battery->pid_calls[0].value, 0xCCDDu);
}

// ---------------------------------------------------------------------------
// PID scan: timeouts and retries
// ---------------------------------------------------------------------------

// A request that never gets answered is retried (timeout = 2 ticks = 200 ms);
// after the retry budget (10) is exhausted the scan advances to the next PID.
TEST_F(UdsCanBatteryTest, PidTimeoutRetriesThenMovesOn) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);  // Initial request for 0x9001.
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);

  // Each 200 ms cycle: one busy tick (timeout 2->1), then a timeout tick (1->0)
  // that retries. 10 timeouts total -> 9 retries of 0x9001, then the scan moves
  // to 0x9002. The first timeout is at t=1200, the 10th at t=3000.
  for (int k = 1; k <= 21; k++) {
    tick(1000 + 100 * k);
  }

  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9002);
  EXPECT_TRUE(battery->pid_calls.empty());
}

// ---------------------------------------------------------------------------
// Sequences: lifecycle
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, StartSequenceDispatchesTheStartState) {
  EXPECT_FALSE(battery->uds_is_busy());

  EXPECT_TRUE(battery->start_sequence(0x1001));
  EXPECT_FALSE(battery->start_sequence(0x1001));  // Already queued: refused.

  tick(1000);
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].state, 0x1001);
  EXPECT_EQ(battery->seq_calls[0].sid, 0u);  // No response yet
  EXPECT_TRUE(battery->seq_calls[0].data.empty());
}

// A queued (not yet started) sequence counts as busy: start_sequence() also
// refuses new sequences while one is queued, so the gate and the reporter must
// agree.
TEST_F(UdsCanBatteryTest, QueuedSequenceReportsBusy) {
  EXPECT_FALSE(battery->uds_is_busy());
  EXPECT_TRUE(battery->start_sequence(0x1001));
  EXPECT_TRUE(battery->uds_is_busy());  // Queued but not started yet.

  tick(1000);  // Dispatched; the subclass sends nothing further.
  EXPECT_FALSE(battery->uds_is_busy());
}

// While the ISO-TP layer is reassembling a (stray) multi-frame response, the
// bus is held even though no UDS transaction is in flight: transmit_uds_can()
// bails on isotp_is_busy(), so the busy report must match.
TEST_F(UdsCanBatteryTest, IsoTpReassemblyReportsBusy) {
  EXPECT_FALSE(battery->uds_is_busy());

  // Stray multi-frame response with nothing in flight: ISO-TP starts
  // reassembly (RX busy), holding the bus.
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x10, 0x0B, 0x62, 0x90, 0x01, 0xAA, 0xBB, 0xCC}));
  EXPECT_TRUE(battery->uds_is_busy());

  // Completing the message clears the busy state.
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x21, 0xDD, 0xEE, 0x55, 0x55, 0x55, 0x55, 0x55}));
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(UdsCanBatteryTest, SequenceStepResponseIsDispatchedToTheSubclass) {
  uint8_t payload[] = {0x01};
  EXPECT_TRUE(battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, /*timeout=*/2, /*retries=*/2));
  EXPECT_TRUE(battery->uds_is_busy());

  // The request went out as a single-frame 0x11 (ECUReset) request:
  // [0x02, 0x11, 0x01] (2 payload bytes: SID + data).
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[0], 0x02);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x11);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[2], 0x01);

  // Positive response 0x51.
  feed_response({0x51, 0x01});
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].state, 0x1002);
  EXPECT_EQ(battery->seq_calls[0].sid, 0x51);
  EXPECT_EQ(battery->seq_calls[0].data, (std::vector<uint8_t>{0x51, 0x01}));
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(UdsCanBatteryTest, SequenceNegativeResponseIsDispatched) {
  uint8_t payload[] = {0x01};
  battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, 2, 2);

  feed_response({0x7F, 0x11, 0x22});  // ConditionsNotCorrect
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].state, 0x1002);
  EXPECT_EQ(battery->seq_calls[0].sid, 0x7F);
}

TEST_F(UdsCanBatteryTest, SequenceResponsePendingExtendsTheWait) {
  uint8_t payload[] = {0x01};
  battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, 2, 2);

  feed_response({0x7F, 0x11, 0x78});  // RCR-RP: not dispatched, timeout extended.
  EXPECT_TRUE(battery->seq_calls.empty());

  feed_response({0x51, 0x01});
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].sid, 0x51);
  EXPECT_TRUE(battery->seq_timeouts.empty());
}

// Stray/late frames must not kill an in-flight sequence step.
TEST_F(UdsCanBatteryTest, UnmatchedFrameDoesNotKillInFlightStep) {
  uint8_t payload[] = {0x01};
  battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, 2, 2);

  feed_response({0x62, 0x90, 0x01, 0x00});  // A PID response for something else.
  EXPECT_TRUE(battery->seq_calls.empty());

  // The step is still awaiting its response: no new request and no timeout yet.
  tick(1000);
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  // The genuine response is still accepted.
  feed_response({0x51, 0x01});
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].sid, 0x51);
}

// A step that never gets answered is retried up to max_retries, then the
// timeout hook fires.
TEST_F(UdsCanBatteryTest, SequenceStepRetriesThenTimesOut) {
  uint8_t payload[] = {0x01};
  EXPECT_TRUE(battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, /*timeout=*/2, /*retries=*/2));
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  tick(1000);  // 2->1, busy.
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  tick(1100);  // 1->0, retry 1.
  EXPECT_EQ(get_transmitted_frames().size(), 2u);

  tick(1200);  // busy
  tick(1300);  // retry 2.
  EXPECT_EQ(get_transmitted_frames().size(), 3u);

  tick(1400);  // busy
  tick(1500);  // budget exhausted -> timeout hook.
  EXPECT_EQ(get_transmitted_frames().size(), 3u);
  ASSERT_EQ(battery->seq_timeouts.size(), 1u);
  EXPECT_EQ(battery->seq_timeouts[0], 0x1002);
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(UdsCanBatteryTest, SequenceRefusedWhileBusyOrPaused) {
  uint8_t payload[] = {0x01};
  battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, 2, 2);

  // A sequence step is already in flight: nothing new can be queued, whatever
  // its priority.
  EXPECT_FALSE(battery->send_sequence_message(0x1003, SID::ECUReset, payload, 1, 2, 2));
  EXPECT_FALSE(
      battery->send_sequence_message(0x1003, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::Custom));

  // Finish the in-flight step, then a pause blocks new steps at or below its
  // level while higher-priority sends still get through.
  feed_response({0x51, 0x01});
  battery->pause_uds(5, UdsCanBattery::UdsPriority::PidScan);
  EXPECT_FALSE(
      battery->send_sequence_message(0x1004, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::Sequence));
  EXPECT_FALSE(
      battery->send_sequence_message(0x1004, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::PidScan));
  EXPECT_TRUE(
      battery->send_sequence_message(0x1004, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::Custom));
}

TEST_F(UdsCanBatteryTest, SequencePayloadLongerThanBufferIsRefused) {
  uint8_t payload[17] = {0};
  EXPECT_FALSE(battery->send_sequence_message(0x1002, SID::WriteDataByIdentifier, payload, sizeof(payload)));
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_FALSE(battery->uds_is_busy());
}

// A zero timeout mustn't wedge the sequence.
TEST_F(UdsCanBatteryTest, ZeroTimeoutStepIsRefused) {
  uint8_t payload[] = {0x01};
  EXPECT_FALSE(battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, /*timeout=*/0));
  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_FALSE(battery->uds_is_busy());
}

// Timeouts longer than an int16 (e.g. 40000 ticks = 4000 s) must expire even
// though the tick counter wraps.
TEST_F(UdsCanBatteryTest, LongTimeoutStepStillTimesOut) {
  uint8_t payload[] = {0x01};
  EXPECT_TRUE(battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, /*timeout=*/40000, /*retries=*/0));
  EXPECT_TRUE(battery->uds_is_busy());

  // 40000 ticks of silence (100 ms each), then the timeout hook must fire.
  for (unsigned long k = 1; k <= 40001; k++) {
    tick(1000 + 100 * k);
  }
  ASSERT_EQ(battery->seq_timeouts.size(), 1u);
  EXPECT_EQ(battery->seq_timeouts[0], 0x1002);
  EXPECT_FALSE(battery->uds_is_busy());
}

// Multi-frame sequence requests (>7 bytes) are transmitted via FF/FC/CF.
TEST_F(UdsCanBatteryTest, MultiFrameSequenceRequestIsSent) {
  const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
  EXPECT_TRUE(battery->send_sequence_message(0x1002, SID::WriteDataByIdentifier, payload, sizeof(payload), 10, 0));

  // First frame announced 11 bytes total.
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[0], 0x10);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x0B);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[2], 0x2E);  // WriteDataByIdentifier

  // Flow control from the BMS lets the remaining bytes out as a consecutive frame.
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}));
  tick(1000);
  ASSERT_EQ(get_transmitted_frames().size(), 2u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[0], 0x21);  // CF, SN=1
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x06);  // First remaining payload byte

  // The assembled 0x2E request gets its positive response dispatched.
  feed_response({0x6E, 0x01});
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].sid, 0x6E);
}

// A sequence started while a PID request is in flight is queued and only starts
// once the PID transaction has finished.
//
// NOTE: the header documents start_sequence() as returning false "if a sequence
// is already active or a PID scan is in flight", but the implementation only
// checks for a *queued* sequence and happily queues behind a PID request or an
// active sequence step. This test pins the current (queueing) behaviour; flag
// for the maintainer whether that matches the intended contract.
TEST_F(UdsCanBatteryTest, StartSequenceDuringPidScanIsQueuedNotRefused) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);                                    // PID 0x9001 is in flight.
  EXPECT_TRUE(battery->start_sequence(0x1001));  // Queued, not refused.

  tick(1100);  // PID transaction still in flight: sequence not started yet.
  EXPECT_TRUE(battery->seq_calls.empty());

  feed_response({0x62, 0x90, 0x01, 0x01});  // PID completes.
  tick(1200);                               // The queued sequence starts now.
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(battery->seq_calls[0].state, 0x1001);
}

// While a sequence is queued/active the PID scan is held back.
TEST_F(UdsCanBatteryTest, SequenceHoldsTheBusFromPidScan) {
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  battery->start_sequence(0x1001);
  tick(1000);
  // The sequence start consumed the tick; no PID request went out.
  ASSERT_EQ(battery->seq_calls.size(), 1u);
  EXPECT_EQ(get_transmitted_frames().size(), 0u);

  tick(1100);
  // Sequence finished (subclass sent nothing further) - PID scan proceeds.
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);
}

// ---------------------------------------------------------------------------
// Pauses
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, PauseBlocksPidScanUntilItExpires) {
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  battery->pause_uds(3, UdsCanBattery::UdsPriority::PidScan);
  EXPECT_TRUE(battery->uds_is_busy());

  tick(1000);  // 3->2
  tick(1100);  // 2->1
  EXPECT_TRUE(get_transmitted_frames().empty());

  tick(1200);  // 1->0: pause expired, scan resumes from the first PID.
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(requested_pid(last_frame(get_transmitted_frames())), 0x9001);
  // (The battery is busy again: the PID request that just went out is in flight.)
}

// A pause at Custom level blocks everything; at PidScan level it still lets
// Custom-priority messages through.
TEST_F(UdsCanBatteryTest, PauseLevelGatesPriorities) {
  uint8_t payload[] = {0x01};

  battery->pause_uds(5, UdsCanBattery::UdsPriority::Custom);
  EXPECT_FALSE(
      battery->send_sequence_message(0x1001, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::Custom));
  EXPECT_TRUE(get_transmitted_frames().empty());

  battery->pause_uds(5, UdsCanBattery::UdsPriority::PidScan);
  EXPECT_TRUE(
      battery->send_sequence_message(0x1001, SID::ECUReset, payload, 1, 2, 2, UdsCanBattery::UdsPriority::Custom));
  EXPECT_EQ(get_transmitted_frames().size(), 1u);
}

// If a pause arrives while a step is in flight and blocks its priority, the
// step gives up immediately instead of retrying into the pause.
TEST_F(UdsCanBatteryTest, InFlightStepGivesUpWhenPausedRetryWouldBeBlocked) {
  uint8_t payload[] = {0x01};
  battery->send_sequence_message(0x1002, SID::ECUReset, payload, 1, /*timeout=*/2, /*retries=*/2);
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  tick(1000);                                                   // 2->1
  battery->pause_uds(10, UdsCanBattery::UdsPriority::PidScan);  // Blocks Sequence priority.

  tick(1100);  // 1->0: would retry, but the retry is blocked -> give up.
  EXPECT_EQ(get_transmitted_frames().size(), 1u);
  ASSERT_EQ(battery->seq_timeouts.size(), 1u);
  EXPECT_EQ(battery->seq_timeouts[0], 0x1002);
  // (The battery still reports busy while the pause itself is active.)
}

// ---------------------------------------------------------------------------
// External diagnostic tool detection
// ---------------------------------------------------------------------------

// A sequence queued while a pause is active must not be silently dropped: it is
// held and starts once the pause expires (previously the first step send was
// refused and the request was lost forever).
TEST_F(UdsCanBatteryTest, SequenceStartedDuringPauseIsHeldUntilPauseExpires) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->pause_uds(3, UdsCanBattery::UdsPriority::Custom);  // Blocks everything.
  battery->read_DTC();                                        // Queued while paused.

  tick(1000);  // 3->2: held, nothing sent.
  tick(1100);  // 2->1: held.
  EXPECT_TRUE(get_transmitted_frames().empty());

  tick(1200);  // 1->0: pause expires, the queued read starts (19 02 09 goes out).
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x19);

  // And the read completes normally after the pause.
  feed_response({0x59, 0x02, 0xFF});
  EXPECT_FALSE(battery->uds_is_busy());
}

// The 5 s external-tool backoff must not eat a DTC read request: the queued
// read is sent once the backoff expires.
TEST_F(UdsCanBatteryTest, DtcReadDuringExternalToolBackoffIsNotLost) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->handle_incoming_uds_can_frame(make_frame(0x7DF, {0x02, 0x3E, 0x00}));  // Tool on the bus.
  battery->read_DTC();

  tick(1000);
  EXPECT_TRUE(get_transmitted_frames().empty());

  // After the 50-tick backoff the queued read is sent.
  for (int k = 2; k <= 60; k++) {
    tick(1000 + 100 * k);
  }
  bool sent_dtc_request = false;
  for (const auto& f : get_transmitted_frames()) {
    if (f.data.u8[1] == 0x19) {
      sent_dtc_request = true;
    }
  }
  EXPECT_TRUE(sent_dtc_request);
}

TEST_F(UdsCanBatteryTest, ExternalToolTrafficBacksOff) {
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  // A request on the broadcast address means a tool is active on the bus.
  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x7DF, {0x02, 0x3E, 0x00})));

  tick(1000);
  EXPECT_TRUE(get_transmitted_frames().empty());  // 5 s backoff (50 ticks).
  EXPECT_TRUE(battery->uds_is_busy());

  // Requests addressed to our own request address also count as external.
  battery->handle_incoming_uds_can_frame(make_frame(0x79B, {0x02, 0x10, 0x01}));
  EXPECT_TRUE(battery->uds_is_busy());
}

// ---------------------------------------------------------------------------
// Response address filtering
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, FramesFromOtherAddressesAreNotConsumed) {
  // Fixed response address 0x7BB (fixture default).
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);
  tick(1000);

  EXPECT_FALSE(battery->handle_incoming_uds_can_frame(make_frame(0x7BC, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));
  EXPECT_TRUE(battery->pid_calls.empty());
  EXPECT_FALSE(battery->handle_incoming_uds_can_frame(make_frame(0x1EA, {0x00})));
  EXPECT_TRUE(battery->pid_calls.empty());
}

TEST_F(UdsCanBatteryTest, AutoDetectAcceptsOnlyTheUdsResponseRange) {
  battery->setup_uds(0x79B, 0);  // Auto-detect the response address.
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);
  // Below the range: not consumed.
  EXPECT_FALSE(battery->handle_incoming_uds_can_frame(make_frame(0x77F, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));
  EXPECT_TRUE(battery->pid_calls.empty());

  // Within the range (0x780-0x7EF): consumed.
  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x780, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));
  ASSERT_EQ(battery->pid_calls.size(), 1u);
  EXPECT_EQ(battery->pid_calls[0].pid, 0x9001);

  // Above the range: not consumed.
  tick(1100);
  EXPECT_FALSE(battery->handle_incoming_uds_can_frame(make_frame(0x7F0, {0x05, 0x62, 0x90, 0x02, 0xAA, 0xBB})));
  EXPECT_EQ(battery->pid_calls.size(), 1u);
}

// The response address is only pinned for the duration of a transaction: a
// completed transaction leaves no stale address behind.
TEST_F(UdsCanBatteryTest, CompletedTransactionDoesNotPinTheResponseAddress) {
  battery->setup_uds(0x79B, 0);
  const uint16_t pids[] = {0x9001, 0x9002};
  battery->set_pid_scan_list(pids, 2);

  tick(1000);
  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x780, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));

  // Next transaction may legitimately be answered from a different address.
  tick(1100);
  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x7EF, {0x05, 0x62, 0x90, 0x02, 0xBB, 0xCC})));
  ASSERT_EQ(battery->pid_calls.size(), 2u);
  EXPECT_EQ(battery->pid_calls[1].pid, 0x9002);
}

// A response arriving while nothing is in flight is consumed but does not pin
// the response address for a later transaction.
TEST_F(UdsCanBatteryTest, StrayResponseDoesNotPinTheAddress) {
  battery->setup_uds(0x79B, 0);
  const uint16_t pids[] = {0x9001};
  battery->set_pid_scan_list(pids, 1);

  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x780, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));
  EXPECT_TRUE(battery->pid_calls.empty());

  tick(1000);
  // The real response from a different valid address is still accepted.
  EXPECT_TRUE(battery->handle_incoming_uds_can_frame(make_frame(0x7EF, {0x05, 0x62, 0x90, 0x01, 0xAA, 0xBB})));
  ASSERT_EQ(battery->pid_calls.size(), 1u);
}

// ---------------------------------------------------------------------------
// DTC read / clear (superclass-internal sequences)
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, ReadDtcParsesSingleFrameResponse) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->read_DTC();
  tick(1000);

  // 19 02 09 (reportDTCByStatusMask) went out.
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x19);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[2], 0x02);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[3], 0x09);

  // 59 02 FF <U019B status FF>  -> one code: C1 9B 00.
  feed_response({0x59, 0x02, 0xFF, 0xC1, 0x9B, 0x00, 0xFF});

  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  ASSERT_EQ(datalayer.battery.dtc.dtc_count, 1);
  EXPECT_EQ(datalayer.battery.dtc.dtc_codes[0], 0xC19B00u);
  EXPECT_EQ(datalayer.battery.dtc.dtc_status[0], 0xFF);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(UdsCanBatteryTest, ReadDtcParsesMultiFrameResponse) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->read_DTC();
  tick(1000);

  // 19 bytes announced: 59 02 FF + 4 codes, each with status 0x4E.
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x10, 0x13, 0x59, 0x02, 0xFF, 0xD0, 0x00, 0x00}));
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x21, 0x4E, 0x33, 0xD7, 0x00, 0x4E, 0x33, 0xD9}));
  battery->handle_incoming_uds_can_frame(make_frame(0x7BB, {0x22, 0x00, 0x4E, 0x33, 0xDD, 0x00, 0x4E, 0xFF}));

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

TEST_F(UdsCanBatteryTest, ReadDtcNegativeResponseMarksReadFailed) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->read_DTC();
  tick(1000);

  feed_response({0x7F, 0x19, 0x12});
  EXPECT_TRUE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
}

TEST_F(UdsCanBatteryTest, ClearDtcSequenceCompletesOnAcknowledgment) {
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};

  battery->reset_DTC();
  tick(1000);

  // 14 FF FF FF (clearDiagnosticInformation) went out.
  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[1], 0x14);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[2], 0xFF);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[3], 0xFF);
  EXPECT_EQ(last_frame(get_transmitted_frames()).data.u8[4], 0xFF);
  EXPECT_TRUE(battery->uds_is_busy());

  feed_response({0x54, 0xFF});
  EXPECT_FALSE(battery->uds_is_busy());  // Sequence ended.
}

// ---------------------------------------------------------------------------
// HTML renderer
// ---------------------------------------------------------------------------

TEST_F(UdsCanBatteryTest, RendererShowsInfoAndDtcSection) {
  UdsBatteryHtmlRenderer renderer(*battery);
  EXPECT_TRUE(renderer.renders_own_battery_data());

  // Without a dtc pointer only the info HTML is emitted.
  std::string html = renderer.get_status_html().c_str();
  EXPECT_NE(html.find("TEST-INFO"), std::string::npos);
  EXPECT_EQ(html.find("Diagnostic Trouble Codes"), std::string::npos);

  // With a dtc pointer the standard DTC section appears.
  battery->dtc = &datalayer.battery.dtc;
  datalayer.battery.dtc = DATALAYER_BATTERY_DTC_TYPE{};
  html = renderer.get_status_html().c_str();
  EXPECT_NE(html.find("TEST-INFO"), std::string::npos);
  // Display text goes through TR(): assert via the runtime so the check holds
  // whatever catalog is loaded.
  EXPECT_NE(html.find(TR(TrKey::DRV_DIAGNOSTIC_TROUBLE_CODES).c_str()), std::string::npos);
  EXPECT_NE(html.find(TR(TrKey::DRV_NOT_READ_YET_USE_READ_DTC_BUTTON_BELOW_SCAN).c_str()), std::string::npos);
}

}  // namespace
