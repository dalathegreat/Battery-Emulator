#include <gtest/gtest.h>

#include <vector>

#include "../Software/src/communication/can/BATTERY-NODE-CAN.h"
#include "../Software/src/communication/can/CONTROLLER-CAN.h"
#include "../Software/src/communication/can/INTER-UNIT-PROTOCOL.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"

// TX capture hook provided by the test emul (test/emul/can.cpp).
extern std::vector<CAN_frame>& emul_tx_frames();
// Emul clock control (test/emul/time.cpp) — reset so reply timing is deterministic.
void set_millis64(uint64_t time);

// ---------------------------------------------------------------------------
// CRC helper (iu_crc8 / iu_crc_stamp / iu_crc_valid) — pure-function behaviour
// ---------------------------------------------------------------------------

TEST(InterUnitCrc, StampThenValidateRoundTrips) {
  uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0x00};
  iu_crc_stamp(IU_NODE_STATUS_ID(1), data, 8);
  EXPECT_TRUE(iu_crc_valid(IU_NODE_STATUS_ID(1), data, 8));
}

TEST(InterUnitCrc, SingleBitCorruptionIsRejected) {
  uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0x00};
  iu_crc_stamp(IU_NODE_STATUS_ID(1), data, 8);
  // Flip a bit in a payload byte — CRC must no longer match.
  data[3] ^= 0x01;
  EXPECT_FALSE(iu_crc_valid(IU_NODE_STATUS_ID(1), data, 8));
}

TEST(InterUnitCrc, IdSeedDetectsMisdeliveredFrame) {
  // Same payload signed for one ID must fail validation against another ID.
  uint8_t data[5] = {1, 2, 3, 4, 0};
  iu_crc_stamp(IU_NODE_IP_ID(1), data, 5);
  EXPECT_TRUE(iu_crc_valid(IU_NODE_IP_ID(1), data, 5));
  EXPECT_FALSE(iu_crc_valid(IU_NODE_IP_ID(2), data, 5));
}

TEST(InterUnitCrc, EmptyFrameIsRejected) {
  uint8_t data[1] = {0};
  EXPECT_FALSE(iu_crc_valid(IU_NODE_STATUS_ID(1), data, 0));
}

// ---------------------------------------------------------------------------
// Controller RX: decode + on-wire rescale, and fail-safe drop on CRC mismatch
// ---------------------------------------------------------------------------

class ControllerRxTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    controller_can.begin();  // clears all battery_nodes
  }
};

TEST_F(ControllerRxTest, StatusFrameDecodesAndRescalesSoc) {
  CAN_frame f = {};
  f.ID = IU_NODE_STATUS_ID(1);
  f.DLC = 8;
  f.data.u8[0] = 4000 >> 8;  // voltage 400.0 V
  f.data.u8[1] = 4000 & 0xFF;
  f.data.u8[2] = 8000 / IU_SOC_WIRE_SCALE;  // SOC 80.00% -> wire 160
  uint16_t cur = (uint16_t)(int16_t)-50;
  f.data.u8[3] = cur >> 8;
  f.data.u8[4] = cur & 0xFF;
  f.data.u8[5] = (uint8_t)(int8_t)25;  // temp 25 C
  f.data.u8[6] = 0;                    // flags
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  controller_can.receive_can_frame(&f);

  const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  EXPECT_EQ(n.voltage_dV, 4000);
  EXPECT_EQ(n.real_soc, 8000);  // 160 * 50
  EXPECT_EQ(n.current_dA, -50);
  EXPECT_EQ(n.temp_max_dC, 25);
  EXPECT_TRUE(n.online);
  EXPECT_EQ(n.still_alive, CAN_STILL_ALIVE);
}

TEST_F(ControllerRxTest, PowerFrameDecodesRemainingAndBalancingBit) {
  CAN_frame f = {};
  f.ID = IU_NODE_POWER_ID(1);
  f.DLC = 8;
  f.data.u8[0] = 3000 >> 8;  // max charge W
  f.data.u8[1] = 3000 & 0xFF;
  f.data.u8[2] = 4000 >> 8;  // max discharge W
  f.data.u8[3] = 4000 & 0xFF;
  uint16_t rem_word = (uint16_t)((10000 / IU_REM_WH_WIRE_SCALE) & IU_NODE_REM_VALUE_MASK) | IU_NODE_REM_BALANCING_BIT;
  f.data.u8[4] = rem_word >> 8;
  f.data.u8[5] = rem_word & 0xFF;
  f.data.u8[6] = (uint8_t)(int8_t)10;  // temp_min
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  controller_can.receive_can_frame(&f);

  const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  EXPECT_EQ(n.max_charge_W, 3000);
  EXPECT_EQ(n.max_discharge_W, 4000);
  EXPECT_EQ(n.remaining_Wh, 10000);  // 5000 * 2
  EXPECT_EQ(n.temp_min_dC, 10);
  EXPECT_TRUE(n.balancing);
}

TEST_F(ControllerRxTest, InfoFrameRescalesSoh) {
  CAN_frame f = {};
  f.ID = IU_NODE_INFO_ID(1);
  f.DLC = 8;
  f.data.u8[0] = 0;
  f.data.u8[1] = 0;  // capacity
  f.data.u8[2] = 0;
  f.data.u8[3] = 0;  // max design V
  f.data.u8[4] = 0;
  f.data.u8[5] = 0;                         // min design V
  f.data.u8[6] = 9900 / IU_SOH_WIRE_SCALE;  // SOH 99.00% -> wire 198
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  controller_can.receive_can_frame(&f);

  EXPECT_EQ(datalayer.system.battery_nodes[0].soh_pptt, 9900);  // 198 * 50
}

TEST_F(ControllerRxTest, CorruptFrameIsDroppedWithoutTouchingNodeState) {
  CAN_frame f = {};
  f.ID = IU_NODE_STATUS_ID(1);
  f.DLC = 8;
  f.data.u8[0] = 4000 >> 8;
  f.data.u8[1] = 4000 & 0xFF;
  f.data.u8[2] = 160;
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  // Pre-seed a known node state that a dropped frame must NOT alter.
  BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  n.voltage_dV = 1234;
  n.still_alive = 5;
  n.online = false;

  // Corrupt a payload byte after signing.
  f.data.u8[1] ^= 0xFF;
  controller_can.receive_can_frame(&f);

  // Fail-safe: still_alive not refreshed, online not set, value not overwritten.
  EXPECT_EQ(n.voltage_dV, 1234);
  EXPECT_EQ(n.still_alive, 5);
  EXPECT_FALSE(n.online);
}

// ---------------------------------------------------------------------------
// End-to-end: a node packs frames that the controller accepts and decodes
// ---------------------------------------------------------------------------

TEST(InterUnitEndToEnd, NodeStatusPowerFramesRoundTripThroughController) {
  init_events();
  set_millis64(0);  // deterministic reply timing regardless of test ordering
  controller_can.begin();
  battery_node_can.begin();

  datalayer.system.status.battery_node_id = 1;
  datalayer.battery.status.voltage_dV = 4000;
  datalayer.battery.status.reported_soc = 8000;  // 80.00%
  datalayer.battery.status.current_dA = -50;
  datalayer.battery.status.temperature_max_dC = 250;  // 25.0 C -> 25
  datalayer.battery.status.temperature_min_dC = 100;  // 10.0 C -> 10
  datalayer.battery.status.max_charge_power_W = 3000;
  datalayer.battery.status.max_discharge_power_W = 4000;
  datalayer.battery.status.remaining_capacity_Wh = 10000;
  datalayer.battery.status.offline_balancing = true;

  // Deliver a valid heartbeat so the node schedules its reply burst.
  CAN_frame hb = {};
  hb.ID = IU_CONTROLLER_HEARTBEAT_ID;
  hb.DLC = 1;
  iu_crc_stamp(hb.ID, hb.data.u8, hb.DLC);
  battery_node_can.receive_can_frame(&hb);

  emul_tx_frames().clear();
  battery_node_can.transmit(1000000);  // well past the (node_id * 5ms) reply delay

  // Replay every frame the node put on the wire into the controller.
  ASSERT_FALSE(emul_tx_frames().empty());
  for (CAN_frame frame : emul_tx_frames()) {
    controller_can.receive_can_frame(&frame);
  }

  const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  EXPECT_EQ(n.voltage_dV, 4000);
  EXPECT_EQ(n.real_soc, 8000);
  EXPECT_EQ(n.current_dA, -50);
  EXPECT_EQ(n.temp_max_dC, 25);
  EXPECT_EQ(n.temp_min_dC, 10);
  EXPECT_EQ(n.max_charge_W, 3000);
  EXPECT_EQ(n.max_discharge_W, 4000);
  EXPECT_EQ(n.remaining_Wh, 10000);
  EXPECT_TRUE(n.balancing);
  EXPECT_TRUE(n.online);
}
