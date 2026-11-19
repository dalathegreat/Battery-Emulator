#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

#include "../../Software/src/battery/RENAULT-ZOE-GEN1-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// TX frame capture injected by the emulated CAN layer (see emul/can.cpp).
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

constexpr uint32_t ZOE_UDS_REQUEST_ID = 0x79B;
constexpr uint32_t ZOE_UDS_RESPONSE_ID = 0x7BB;

// Single-frame ISO-TP response from the BMS (PCI byte + payload, 0x55 padding).
CAN_frame bms_response(std::initializer_list<uint8_t> uds_message) {
  CAN_frame frame = {};
  frame.ID = ZOE_UDS_RESPONSE_ID;
  frame.DLC = 8;
  frame.data.u8[0] = static_cast<uint8_t>(uds_message.size());
  uint8_t i = 1;
  for (uint8_t b : uds_message) {
    frame.data.u8[i++] = b;
  }
  while (i < 8) {
    frame.data.u8[i++] = 0x55;
  }
  return frame;
}

class RenaultZoeGen1Test : public ::testing::Test {
 protected:
  void SetUp() override {
    clear_transmitted_frames();
    datalayer = DataLayer();
    now = 1000;
    set_millis64(now);
    battery = new RenaultZoeGen1Battery();
    battery->setup();
  }

  void TearDown() override { delete battery; }

  // Run one 100ms UDS tick and return the UDS requests sent since the previous
  // tick (follow-up steps may go out directly from the receive path).
  std::vector<std::vector<uint8_t>> tick() {
    now += 100;
    set_millis64(now);
    battery->transmit_can(now);
    std::vector<std::vector<uint8_t>> requests;
    for (const auto& frame : get_transmitted_frames()) {
      if (frame.ID == ZOE_UDS_REQUEST_ID) {
        const uint8_t len = frame.data.u8[0] & 0x0F;
        requests.emplace_back(frame.data.u8 + 1, frame.data.u8 + 1 + len);
      }
    }
    clear_transmitted_frames();
    return requests;
  }

  void feed(std::initializer_list<uint8_t> uds_message) {
    battery->handle_incoming_can_frame(bms_response(uds_message));
  }

  RenaultZoeGen1Battery* battery = nullptr;
  unsigned long now = 0;
};

using Bytes = std::vector<uint8_t>;

// Clear sequence: 10 C0 -> 14 FF FF FF -> 31 05 00 -> 19 02 09.
const Bytes SESSION_C0 = {0x10, 0xC0};
const Bytes NORMAL_CLEAR = {0x14, 0xFF, 0xFF, 0xFF};
const Bytes PROTECTED_CLEAR = {0x31, 0x05, 0x00};
const Bytes READ_DTC = {0x19, 0x02, 0x09};

TEST_F(RenaultZoeGen1Test, DtcClearRunsNormalAndProtectedClearInSessionC0ThenRereadsDtcs) {
  battery->dtc = &datalayer.battery.dtc;
  EXPECT_TRUE(battery->supports_reset_DTC());
  battery->reset_DTC();

  ASSERT_EQ(tick(), (std::vector<Bytes>{SESSION_C0}));
  feed({0x50, 0xC0});
  ASSERT_EQ(tick(), (std::vector<Bytes>{NORMAL_CLEAR}));
  feed({0x54});
  ASSERT_EQ(tick(), (std::vector<Bytes>{PROTECTED_CLEAR}));
  feed({0x71, 0x05});
  ASSERT_EQ(tick(), (std::vector<Bytes>{READ_DTC}));

  // No DTCs left.
  feed({0x59, 0x02, 0xFF});
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 0);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
  EXPECT_FALSE(battery->uds_is_busy());
}

TEST_F(RenaultZoeGen1Test, DtcClearStopsWhenSessionIsRefused) {
  battery->reset_DTC();
  ASSERT_EQ(tick(), (std::vector<Bytes>{SESSION_C0}));

  feed({0x7F, 0x10, 0x12});
  EXPECT_FALSE(battery->uds_is_busy());
  for (int i = 0; i < 5; i++) {
    for (const auto& request : tick()) {
      EXPECT_NE(request[0], 0x14);  // Neither clear is attempted without the session.
      EXPECT_NE(request[0], 0x31);
    }
  }
}

TEST_F(RenaultZoeGen1Test, ProtectedClearStillRunsWhenNormalClearIsRefused) {
  battery->reset_DTC();
  tick();
  feed({0x50, 0xC0});
  ASSERT_EQ(tick(), (std::vector<Bytes>{NORMAL_CLEAR}));

  feed({0x7F, 0x14, 0x22});
  ASSERT_EQ(tick(), (std::vector<Bytes>{PROTECTED_CLEAR}));
  feed({0x71, 0x05});
  ASSERT_EQ(tick(), (std::vector<Bytes>{READ_DTC}));
}

TEST_F(RenaultZoeGen1Test, DtcsAreRereadWhenProtectedClearIsRefused) {
  battery->reset_DTC();
  tick();
  feed({0x50, 0xC0});
  tick();
  feed({0x54});
  ASSERT_EQ(tick(), (std::vector<Bytes>{PROTECTED_CLEAR}));

  // The normal clear may still have worked, so show what's left.
  feed({0x7F, 0x31, 0x22});
  ASSERT_EQ(tick(), (std::vector<Bytes>{READ_DTC}));
}

TEST_F(RenaultZoeGen1Test, DtcClearGivesUpWhenSessionRequestTimesOut) {
  battery->reset_DTC();
  ASSERT_EQ(tick(), (std::vector<Bytes>{SESSION_C0}));

  int session_requests = 1;
  bool pid_scan_resumed = false;
  for (int i = 0; i < 100; i++) {
    for (const auto& request : tick()) {
      pid_scan_resumed |= request[0] == 0x21;  // Sequence gave up, normal PID polling is back.
      session_requests += request == SESSION_C0;
      EXPECT_NE(request[0], 0x14);
    }
  }
  EXPECT_TRUE(pid_scan_resumed);
  EXPECT_EQ(session_requests, 3);  // First attempt + 2 retries.
}

TEST_F(RenaultZoeGen1Test, DtcClearGivesUpWhenNormalClearTimesOut) {
  battery->reset_DTC();
  tick();
  feed({0x50, 0xC0});
  ASSERT_EQ(tick(), (std::vector<Bytes>{NORMAL_CLEAR}));

  int clear_requests = 1;
  bool pid_scan_resumed = false;
  for (int i = 0; i < 100; i++) {
    for (const auto& request : tick()) {
      pid_scan_resumed |= request[0] == 0x21;
      clear_requests += request == NORMAL_CLEAR;
      EXPECT_NE(request[0], 0x31);
    }
  }
  EXPECT_TRUE(pid_scan_resumed);
  EXPECT_EQ(clear_requests, 3);
}

TEST_F(RenaultZoeGen1Test, DtcClearGivesUpWhenProtectedClearTimesOut) {
  battery->reset_DTC();
  tick();
  feed({0x50, 0xC0});
  tick();
  feed({0x54});
  ASSERT_EQ(tick(), (std::vector<Bytes>{PROTECTED_CLEAR}));

  int routine_requests = 1;
  bool pid_scan_resumed = false;
  for (int i = 0; i < 100; i++) {
    for (const auto& request : tick()) {
      pid_scan_resumed |= request[0] == 0x21;
      routine_requests += request == PROTECTED_CLEAR;
      EXPECT_NE(request[0], 0x19);
    }
  }
  EXPECT_TRUE(pid_scan_resumed);
  EXPECT_EQ(routine_requests, 3);
}

TEST_F(RenaultZoeGen1Test, ReadDtcSendsReadWithoutOpeningASession) {
  battery->dtc = &datalayer.battery.dtc;
  battery->read_DTC();
  ASSERT_EQ(tick(), (std::vector<Bytes>{READ_DTC}));

  feed({0x59, 0x02, 0x4F});
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_NE(datalayer.battery.dtc.dtc_last_read_millis, 0u);
  EXPECT_FALSE(battery->uds_is_busy());
}

}  // namespace
