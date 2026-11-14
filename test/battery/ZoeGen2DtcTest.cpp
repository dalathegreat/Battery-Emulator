#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "../../Software/src/battery/RENAULT-ZOE-GEN2-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

#include "Arduino.h"

// TX frame capture injected by the emulated CAN layer (see emul/can.cpp).
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

// The Zoe Gen2 reads and clears DTCs on two ECUs: the LBC (0x18DADBF1 ->
// 0x18DAF1DB) and a second LBC (0x18DADCF1 -> 0x18DAF1DC). These tests
// run the real integration against simulated ECUs answering single-frame UDS.

namespace {

constexpr uint32_t LBC_REQ = 0x18DADBF1;
constexpr uint32_t LBC_RSP = 0x18DAF1DB;
constexpr uint32_t LBC2_REQ = 0x18DADCF1;
constexpr uint32_t LBC2_RSP = 0x18DAF1DC;

struct Request {
  uint32_t id;
  uint8_t sid;
};

CAN_frame single_frame(uint32_t id, const std::vector<uint8_t>& msg) {
  CAN_frame f = {};
  f.ID = id;
  f.ext_ID = true;
  f.DLC = 8;
  f.data.u8[0] = static_cast<uint8_t>(msg.size());
  for (size_t i = 0; i < msg.size() && i < 7; i++) {
    f.data.u8[i + 1] = msg[i];
  }
  for (size_t i = msg.size() + 1; i < 8; i++) {
    f.data.u8[i] = 0xAA;
  }
  return f;
}

class ZoeGen2DtcTest : public ::testing::Test {
 protected:
  void SetUp() override {
    clear_transmitted_frames();
    datalayer = DataLayer();
    set_millis64(1000);
    battery = new RenaultZoeGen2Battery();
    battery->setup();
  }

  void TearDown() override { delete battery; }

  // What a simulated ECU replies to a request with the given SID (empty = silent).
  std::vector<uint8_t> respond(bool lbc2, const uint8_t* req) {
    if (lbc2 && !lbc2_present) {
      return {};
    }
    switch (req[1]) {
      case 0x22:
        return {0x62, req[2], req[3], 0x00, 0x00};
      case 0x19:
        if (lbc2) {
          return {0x59, 0x02, 0xFF, 0x12, 0x34, 0x56, 0x08};
        }
        return {0x59, 0x02, 0xFF, 0xC1, 0x9B, 0x00, 0xFF};
      case 0x10:
        if (lbc2 && lbc2_rejects_session) {
          return {0x7F, 0x10, 0x12};
        }
        return {0x50, 0x03, 0x00, 0x32, 0x01, 0xF4};
      case 0x14:
        return {0x54};
      default:
        return {0x7F, req[1], 0x11};
    }
  }

  // Runs the battery for `ms` milliseconds in 10ms steps, answering requests
  // (including ones sent from inside the battery's RX handling).
  void run(unsigned long ms) {
    for (unsigned long end = now + ms; now < end; now += 10) {
      set_millis64(now);
      battery->transmit_can(now);
      while (cursor < get_transmitted_frames().size()) {
        const CAN_frame f = get_transmitted_frames()[cursor++];
        if (f.ID != LBC_REQ && f.ID != LBC2_REQ) {
          continue;
        }
        const bool lbc2 = (f.ID == LBC2_REQ);
        requests.push_back({f.ID, f.data.u8[1]});
        std::vector<uint8_t> rsp = respond(lbc2, f.data.u8);
        if (!rsp.empty()) {
          battery->handle_incoming_can_frame(single_frame(lbc2 ? LBC2_RSP : LBC_RSP, rsp));
        }
      }
    }
  }

  // Non-PID-scan requests in order, as (id, sid).
  std::vector<std::pair<uint32_t, uint8_t>> diag_requests() const {
    std::vector<std::pair<uint32_t, uint8_t>> out;
    for (const Request& r : requests) {
      if (r.sid != 0x22) {
        out.push_back({r.id, r.sid});
      }
    }
    return out;
  }

  // Number of PID-scan reads sent to `id`.
  size_t pid_reads_to(uint32_t id) const {
    size_t n = 0;
    for (const Request& r : requests) {
      if (r.sid == 0x22 && r.id == id) {
        n++;
      }
    }
    return n;
  }

  RenaultZoeGen2Battery* battery = nullptr;
  unsigned long now = 1000;
  bool lbc2_present = true;
  bool lbc2_rejects_session = false;
  size_t cursor = 0;
  std::vector<Request> requests;
};

TEST_F(ZoeGen2DtcTest, ReadDtcQueriesBothEcusIntoSeparateStores) {
  run(500);
  battery->read_DTC();
  run(2000);

  using P = std::pair<uint32_t, uint8_t>;
  EXPECT_EQ(diag_requests(), (std::vector<P>{{LBC_REQ, 0x19}, {LBC2_REQ, 0x19}}));

  const auto& lbc = datalayer.battery.dtc;
  EXPECT_FALSE(lbc.dtc_read_failed);
  ASSERT_EQ(lbc.dtc_count, 1);
  EXPECT_EQ(lbc.dtc_codes[0], 0xC19B00u);

  // The second LBC's code is not mixed into the datalayer...
  String html = battery->get_uds_dtc_html();
  std::string s(html.c_str());
  EXPECT_NE(s.find("LBC Diagnostic Trouble Codes"), std::string::npos);
  EXPECT_NE(s.find("LBC2 Diagnostic Trouble Codes"), std::string::npos);
  // ...and the JSON loader (fixed element IDs) appears only once.
  size_t first = s.find("id='dtcJsonStatus'");
  ASSERT_NE(first, std::string::npos);
  EXPECT_EQ(s.find("id='dtcJsonStatus'", first + 1), std::string::npos);
}

TEST_F(ZoeGen2DtcTest, PidScanNeverTargetsLbc2) {
  battery->read_DTC();
  run(3000);
  battery->reset_DTC();
  run(5000);
  lbc2_present = false;
  battery->read_DTC();
  run(10000);

  EXPECT_EQ(pid_reads_to(LBC2_REQ), 0u);
  EXPECT_GT(pid_reads_to(LBC_REQ), 0u);
}

TEST_F(ZoeGen2DtcTest, ClearDtcClearsBothEcusThenReadsBack) {
  run(500);
  battery->reset_DTC();
  run(4000);

  using P = std::pair<uint32_t, uint8_t>;
  EXPECT_EQ(
      diag_requests(),
      (std::vector<P>{
          {LBC_REQ, 0x10}, {LBC_REQ, 0x14}, {LBC2_REQ, 0x10}, {LBC2_REQ, 0x14}, {LBC_REQ, 0x19}, {LBC2_REQ, 0x19}}));
}

TEST_F(ZoeGen2DtcTest, SilentLbc2FailsOnlyItsOwnRead) {
  lbc2_present = false;
  run(500);
  battery->read_DTC();
  run(10000);

  // LBC read succeeded, the second LBC's read is marked failed.
  EXPECT_FALSE(datalayer.battery.dtc.dtc_read_failed);
  EXPECT_EQ(datalayer.battery.dtc.dtc_count, 1);
  std::string s(battery->get_uds_dtc_html().c_str());
  EXPECT_NE(s.find("Last DTC read failed"), std::string::npos);

  // And the PID scan resumed on the LBC afterwards.
  size_t last_lbc2 = 0;
  for (size_t i = 0; i < requests.size(); i++) {
    if (requests[i].id == LBC2_REQ) {
      last_lbc2 = i;
    }
  }
  bool pid_after = false;
  for (size_t i = last_lbc2 + 1; i < requests.size(); i++) {
    if (requests[i].id == LBC_REQ && requests[i].sid == 0x22) {
      pid_after = true;
    }
  }
  EXPECT_TRUE(pid_after);
}

TEST_F(ZoeGen2DtcTest, SilentLbc2StillClearsLbc) {
  lbc2_present = false;
  run(500);
  battery->reset_DTC();
  run(15000);

  // LBC session + clear, LBC2 session attempts (no clear), then readback.
  const auto diag = diag_requests();
  ASSERT_GE(diag.size(), 3u);
  EXPECT_EQ(diag[0], std::make_pair(LBC_REQ, (uint8_t)0x10));
  EXPECT_EQ(diag[1], std::make_pair(LBC_REQ, (uint8_t)0x14));
  bool lbc2_clear = false;
  bool readback = false;
  for (const auto& r : diag) {
    lbc2_clear |= (r.first == LBC2_REQ && r.second == 0x14);
    readback |= (r.first == LBC_REQ && r.second == 0x19);
  }
  EXPECT_FALSE(lbc2_clear);
  EXPECT_TRUE(readback);
}

TEST_F(ZoeGen2DtcTest, RejectedSessionStillAttemptsClear) {
  lbc2_rejects_session = true;
  run(500);
  battery->reset_DTC();
  run(4000);

  using P = std::pair<uint32_t, uint8_t>;
  EXPECT_EQ(
      diag_requests(),
      (std::vector<P>{
          {LBC_REQ, 0x10}, {LBC_REQ, 0x14}, {LBC2_REQ, 0x10}, {LBC2_REQ, 0x14}, {LBC_REQ, 0x19}, {LBC2_REQ, 0x19}}));
}

}  // namespace
