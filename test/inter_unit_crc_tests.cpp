#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <vector>

#include "../Software/src/communication/can/BATTERY-NODE-CAN.h"
#include "../Software/src/communication/can/CONTROLLER-CAN.h"
#include "../Software/src/communication/can/INTER-UNIT-PROTOCOL.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"

// TX capture hooks provided by the test emul (test/emul/can.cpp).
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();
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
  const uint16_t chg_wire = 3000 / IU_POWER_W_WIRE_SCALE;  // max charge W in 10 W steps
  const uint16_t dch_wire = 4000 / IU_POWER_W_WIRE_SCALE;  // max discharge W in 10 W steps
  f.data.u8[0] = chg_wire >> 8;
  f.data.u8[1] = chg_wire & 0xFF;
  f.data.u8[2] = dch_wire >> 8;
  f.data.u8[3] = dch_wire & 0xFF;
  uint16_t rem_word = (uint16_t)((10000 / IU_REM_WH_WIRE_SCALE) & IU_NODE_REM_VALUE_MASK) | IU_NODE_REM_BALANCING_BIT;
  f.data.u8[4] = rem_word >> 8;
  f.data.u8[5] = rem_word & 0xFF;
  f.data.u8[6] = (uint8_t)(int8_t)10;  // temp_min
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  controller_can.receive_can_frame(&f);

  const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  EXPECT_EQ(n.max_charge_W, 3000);
  EXPECT_EQ(n.max_discharge_W, 4000);
  EXPECT_EQ(n.remaining_Wh, 10000);  // 1000 * 10
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

TEST_F(ControllerRxTest, InfoFrameRescalesCapacityPast16Bits) {
  CAN_frame f = {};
  f.ID = IU_NODE_INFO_ID(1);
  f.DLC = 8;
  const uint16_t cap_wire = 84000 / IU_CAP_WH_WIRE_SCALE;  // 2x BMW i3 120Ah, does not fit 16 bits in Wh
  f.data.u8[0] = cap_wire >> 8;
  f.data.u8[1] = cap_wire & 0xFF;
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);

  controller_can.receive_can_frame(&f);

  EXPECT_EQ(datalayer.system.battery_nodes[0].total_capacity_Wh, 84000u);
}

TEST_F(ControllerRxTest, IdentFromOtherProtocolVersionIsIgnored) {
  CAN_frame f = {};
  f.ID = IU_NODE_IDENT_ID(1);
  f.DLC = 8;
  f.data.u8[4] = IU_PROTOCOL_VERSION - 1;  // e.g. a node still on the 2 Wh capacity layout
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);
  controller_can.receive_can_frame(&f);
  EXPECT_FALSE(datalayer.system.battery_nodes[0].ident_received);

  f.data.u8[4] = IU_PROTOCOL_VERSION;
  iu_crc_stamp(f.ID, f.data.u8, f.DLC);
  controller_can.receive_can_frame(&f);
  EXPECT_TRUE(datalayer.system.battery_nodes[0].ident_received);
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

  // The node reports the installation behind it (datalayer.aggregate), like any inverter would;
  // only the link voltage and the balancing flag come from the pack itself.
  datalayer.system.status.battery_node_id = 1;
  datalayer.battery.status.voltage_dV = 4000;
  datalayer.aggregate.reported_soc = 8000;  // 80.00%
  datalayer.aggregate.current_dA = -50;
  datalayer.aggregate.temperature_max_dC = 250;     // 25.0 C -> 25
  datalayer.aggregate.temperature_min_dC = 100;     // 10.0 C -> 10
  datalayer.aggregate.max_charge_power_W = 100000;  // above the old 16-bit W ceiling
  datalayer.aggregate.max_discharge_power_W = 4000;
  datalayer.aggregate.remaining_capacity_Wh = 100000;  // above the old 16-bit Wh ceiling
  datalayer.aggregate.total_capacity_Wh = 160000;
  datalayer.battery.status.offline_balancing = true;

  // Deliver a valid heartbeat so the node schedules its reply burst.
  CAN_frame hb = {};
  hb.ID = IU_CONTROLLER_HEARTBEAT_ID;
  hb.DLC = 1;
  iu_crc_stamp(hb.ID, hb.data.u8, hb.DLC);
  battery_node_can.receive_can_frame(&hb);

  clear_transmitted_frames();
  battery_node_can.transmit(1000000);  // well past the (node_id * 5ms) reply delay

  // Replay every frame the node put on the wire into the controller.
  ASSERT_FALSE(get_transmitted_frames().empty());
  for (CAN_frame frame : get_transmitted_frames()) {
    controller_can.receive_can_frame(&frame);
  }

  const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[0];
  EXPECT_EQ(n.voltage_dV, 4000);
  EXPECT_EQ(n.real_soc, 8000);
  EXPECT_EQ(n.current_dA, -50);
  EXPECT_EQ(n.temp_max_dC, 25);
  EXPECT_EQ(n.temp_min_dC, 10);
  EXPECT_EQ(n.max_charge_W, 100000u);
  EXPECT_EQ(n.max_discharge_W, 4000);
  EXPECT_EQ(n.remaining_Wh, 100000u);
  EXPECT_TRUE(n.balancing);
  EXPECT_TRUE(n.online);
}

// ---------------------------------------------------------------------------
// Node: controller heartbeat watchdog uses the short lost-link timeout
// ---------------------------------------------------------------------------

TEST(InterUnitNode, HeartbeatArmsShortControllerTimeout) {
  init_events();
  battery_node_can.begin();
  datalayer.system.status.CAN_controller_still_alive = 0;

  CAN_frame hb = {};
  hb.ID = IU_CONTROLLER_HEARTBEAT_ID;
  hb.DLC = 1;
  iu_crc_stamp(hb.ID, hb.data.u8, hb.DLC);
  battery_node_can.receive_can_frame(&hb);

  // Must open well before the controller lets the remaining nodes resume.
  EXPECT_EQ(datalayer.system.status.CAN_controller_still_alive, IU_NODE_CONTROLLER_TIMEOUT_S);
  EXPECT_GT(IU_NODE_CONTROLLER_TIMEOUT_S, IU_STATUS_STALE_SECONDS);
  EXPECT_LT(IU_NODE_CONTROLLER_TIMEOUT_S, IU_STALE_DROP_SECONDS);
}

// ---------------------------------------------------------------------------
// Controller scenarios: lost-link sequencing and join reference
// ---------------------------------------------------------------------------

class ControllerScenarioTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    set_millis64(1000);  // _startup_begin_ms == 0 means "not started", so start the clock above 0
    controller_can.begin();
    datalayer.system.info.equipment_stop_active = false;
    datalayer.system.status.inverter_allows_contactor_closing = true;
    datalayer.battery.status.active_power_W = 0;
  }

  // An online, verified node reporting fresh data.
  BATTERY_NODE_TYPE& add_node(uint8_t idx, uint16_t voltage_dV, bool engaged) {
    BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[idx];
    n.online = true;
    n.still_alive = CAN_STILL_ALIVE;
    n.ident_received = true;
    n.fw_version_num = iu_fw_version_num();
    n.battery_type_id = 0;
    n.voltage_dV = voltage_dV;
    n.contactor_engaged = engaged;
    n.max_charge_W = 5000;
    n.max_discharge_W = 5000;
    n.balancing = false;
    n.fault_flags = 0;
    n._last_status_toggle = 0;
    n.status_stale_seconds = 0;
    return n;
  }

  // Run the startup grace period to completion (all currently online matching nodes are allowed).
  void finish_grace() {
    controller_can.update_values();  // starts the grace timer
    set_millis64(1000 + IU_STARTUP_GRACE_S * 1000u + 1u);
    controller_can.update_values();
  }

  // One controller second. Nodes in `frozen` stop refreshing their STATUS toggle.
  void tick(std::initializer_list<uint8_t> frozen = {}) {
    for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
      BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[i];
      bool is_frozen = std::find(frozen.begin(), frozen.end(), i) != frozen.end();
      if (n.online && !is_frozen) {
        n.status_stale_seconds = 0;
        n.still_alive = CAN_STILL_ALIVE;
      }
    }
    controller_can.update_values();
  }
};

TEST_F(ControllerScenarioTest, StaleNodeHoldsPackAtZeroUntilDroppedThenOthersResume) {
  add_node(0, 4000, true);
  add_node(1, 4000, true);
  BATTERY_NODE_TYPE& lost = add_node(2, 4000, true);
  finish_grace();
  tick();
  ASSERT_TRUE(lost.contactor_allowed);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 15000u);  // 5000 x 3

  // Link to node 3 breaks: its STATUS stops refreshing (the counter already reads 1 after the last
  // fresh update, so it crosses IU_STATUS_STALE_SECONDS on the third missed update).
  for (uint8_t s = 1; s < IU_STATUS_STALE_SECONDS; s++) {
    tick({2});
  }
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 15000u);  // not stale yet

  tick({2});  // now stale
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0u);
  EXPECT_TRUE(lost.contactor_allowed);  // no OPEN command while the inverter ramps down

  while (lost.status_stale_seconds < IU_STALE_OPEN_SECONDS) {
    tick({2});
  }
  EXPECT_FALSE(lost.contactor_allowed);  // OPEN at no load
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);

  while (lost.status_stale_seconds < IU_STALE_DROP_SECONDS) {
    tick({2});
    EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  }
  tick({2});                                                       // past the drop time: resume as N-1
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 10000u);  // 5000 x 2
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 10000u);
}

TEST_F(ControllerScenarioTest, StaleNodeRecoveringBeforeOpenKeepsItsPermission) {
  add_node(0, 4000, true);
  BATTERY_NODE_TYPE& flaky = add_node(1, 4000, true);
  finish_grace();
  for (uint8_t s = 0; s < 6; s++) {
    tick({1});
  }
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  tick();  // data fresh again
  EXPECT_TRUE(flaky.contactor_allowed);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 10000u);
}

TEST_F(ControllerScenarioTest, JoinReferenceIsTheEngagedBusNotAMerelyAllowedPack) {
  add_node(1, 4000, true);  // node 2 holds the bus at 400.0 V
  finish_grace();
  ASSERT_TRUE(datalayer.system.battery_nodes[1].contactor_allowed);

  // Node 1 was allowed but never closed (sits at 380.0 V); node 3 matches node 1, not the bus.
  BATTERY_NODE_TYPE& stuck = add_node(0, 3800, false);
  stuck.contactor_allowed = true;
  BATTERY_NODE_TYPE& joiner = add_node(2, 3800, false);

  for (uint8_t s = 0; s < 70; s++) {
    tick();
    EXPECT_FALSE(joiner.contactor_allowed) << "joined against the wrong pack at t=" << (int)s;
  }
  EXPECT_FALSE(stuck.contactor_allowed);  // revoked: allowed but never reported closed
}

TEST_F(ControllerScenarioTest, JoinWaitsForPendingNodeThenUsesTheBus) {
  add_node(1, 4000, true);
  finish_grace();

  BATTERY_NODE_TYPE& stuck = add_node(0, 3800, false);
  stuck.contactor_allowed = true;
  BATTERY_NODE_TYPE& joiner = add_node(2, 4000, false);  // matches the bus

  // Held while node 1 is allowed-but-not-closed...
  for (uint8_t s = 0; s < 25; s++) {
    tick();
    EXPECT_FALSE(joiner.contactor_allowed);
  }
  // ...then joins against the bus once node 1's permission is revoked.
  for (uint8_t s = 0; s < 20 && !joiner.contactor_allowed; s++) {
    tick();
  }
  EXPECT_TRUE(joiner.contactor_allowed);
  EXPECT_FALSE(stuck.contactor_allowed);
}
