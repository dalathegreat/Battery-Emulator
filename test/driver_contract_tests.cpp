#include <gtest/gtest.h>

#include <vector>

#include "../Software/src/battery/CanBattery.h"
#include "../Software/src/communication/can/comm_can_dispatch.h"
#include "../Software/src/datalayer/datalayer.h"

// Covers the contract the battery base classes give every driver, rather than
// any one integration.
//
// Two things matter here. Battery declares a large set of optional
// capabilities, each a supports_*() predicate paired with an action; a driver
// opts in by overriding both. The base defaults are what make it safe for a
// driver to override neither, and what the UI reads to decide which buttons to
// show. And CanBattery wires a driver to a bus: it registers on construction
// and adapts between the CanReceiver/Transmitter interfaces and the driver's
// own methods.

namespace {

// The smallest thing that satisfies the base class: overrides nothing optional.
class MinimalCanBattery : public CanBattery {
 public:
  MinimalCanBattery() : CanBattery(CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS) {}
  explicit MinimalCanBattery(CAN_Interface interface) : CanBattery(interface, CAN_Speed::CAN_SPEED_500KBPS) {}

  void setup() override {}
  void update_values() override {}

  void handle_incoming_can_frame(CAN_frame rx_frame) override { received_ids.push_back(rx_frame.ID); }
  void transmit_can(unsigned long currentMillis) override { transmit_calls.push_back(currentMillis); }

  std::vector<uint32_t> received_ids;
  std::vector<unsigned long> transmit_calls;
};

CAN_frame frame_with_id(uint32_t id) {
  CAN_frame f = {};
  f.ID = id;
  f.DLC = 8;
  return f;
}

class DriverContractTest : public ::testing::Test {
 protected:
  void SetUp() override { clear_can_receivers(); }
  void TearDown() override { clear_can_receivers(); }
};

}  // namespace

// --- The optional-capability defaults --------------------------------------

// A driver that declares no capabilities must report none. The settings and
// battery pages read exactly these to decide which controls to render, so a
// default flipping to true would offer the user a button that does nothing.
TEST_F(DriverContractTest, ADriverDeclaresNoCapabilitiesByDefault) {
  MinimalCanBattery battery;

  EXPECT_FALSE(battery.supports_clear_isolation());
  EXPECT_FALSE(battery.supports_reset_BMS());
  EXPECT_FALSE(battery.supports_reset_SOC());
  EXPECT_FALSE(battery.supports_reset_crash());
  EXPECT_FALSE(battery.supports_reset_NVROL());
  EXPECT_FALSE(battery.supports_reset_DTC());
  EXPECT_FALSE(battery.supports_read_DTC());
  EXPECT_FALSE(battery.supports_reset_SOH());
  EXPECT_FALSE(battery.supports_reset_BECM());
  EXPECT_FALSE(battery.supports_calibrate_SOC());
  EXPECT_FALSE(battery.supports_contactor_close());
  EXPECT_FALSE(battery.supports_contactor_reset());
  EXPECT_FALSE(battery.supports_set_fake_voltage());
  EXPECT_FALSE(battery.supports_manual_balancing());
  EXPECT_FALSE(battery.supports_real_BMS_status());
  EXPECT_FALSE(battery.supports_toggle_SOC_method());
  EXPECT_FALSE(battery.supports_energy_saving_mode_reset());
  EXPECT_FALSE(battery.supports_factory_mode_method());
  EXPECT_FALSE(battery.supports_chademo_restart());
  EXPECT_FALSE(battery.supports_chademo_stop());
  EXPECT_FALSE(battery.supports_balancing());
  EXPECT_FALSE(battery.supports_balancing_request());
  EXPECT_FALSE(battery.supports_isolation_test());
  EXPECT_FALSE(battery.supports_charged_energy());
  EXPECT_FALSE(battery.supports_insulation_resistance());
}

// The actions paired with those predicates must be safe to call even when the
// driver has not implemented them - the base no-op is what makes an unguarded
// call from the UI or MQTT harmless rather than a crash.
TEST_F(DriverContractTest, UnimplementedActionsChangeNothing) {
  MinimalCanBattery battery;

  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.max_charge_power_W = 5000;
  datalayer.battery.status.max_discharge_power_W = 5000;
  datalayer.battery.status.real_bms_status = BMS_ACTIVE;
  const DATALAYER_BATTERY_TYPE before = datalayer.battery;
  const bool start_precharging_before = datalayer.system.info.start_precharging;

  battery.request_isolation_test();
  battery.clear_isolation();
  battery.calibrate_SOC();
  battery.reset_BMS();
  battery.reset_SOC();
  battery.reset_crash();
  battery.reset_contactor();
  battery.reset_NVROL();
  battery.reset_DTC();
  battery.read_DTC();
  battery.reset_SOH();
  battery.reset_BECM();
  battery.request_open_contactors();
  battery.request_close_contactors();
  battery.toggle_SOC_method();
  battery.reset_energy_saving_mode();
  battery.set_factory_mode();
  battery.chademo_restart();
  battery.chademo_stop();
  battery.initiate_balancing();
  battery.end_balancing();
  battery.handle_precharge();
  battery.set_fake_voltage(400.0f);

  // The property is that the defaults change nothing, not merely that they do
  // not crash - an empty body cannot crash, so asserting only that proves
  // nothing. A default that quietly wrote to the datalayer would be caught here.
  EXPECT_EQ(datalayer.battery.status.real_soc, before.status.real_soc);
  EXPECT_EQ(datalayer.battery.status.voltage_dV, before.status.voltage_dV);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, before.status.max_charge_power_W);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, before.status.max_discharge_power_W);
  EXPECT_EQ(datalayer.battery.status.real_bms_status, before.status.real_bms_status);
  EXPECT_EQ(datalayer.battery.settings.user_requests_balancing, before.settings.user_requests_balancing);
  EXPECT_EQ(datalayer.system.info.start_precharging, start_precharging_before);
}

TEST_F(DriverContractTest, DefaultBalancingStateIsInactiveAndUnnamed) {
  MinimalCanBattery battery;

  EXPECT_FALSE(battery.is_balancing_active());
  EXPECT_EQ(battery.get_balancing_state_string(), nullptr);
}

// The safety monitor raises EVENT_SOC_PLAUSIBILITY_ERROR when a driver says its
// SOC is implausible. Defaulting to true means a driver that has no opinion
// never trips it.
TEST_F(DriverContractTest, SocIsPlausibleUnlessADriverSaysOtherwise) {
  MinimalCanBattery battery;

  EXPECT_TRUE(battery.soc_plausible());
}

// The charge taper is optional except where a chemistry requires it; a driver
// that says nothing must not have it forced on.
TEST_F(DriverContractTest, ChargeTaperIsNotMandatoryByDefault) {
  MinimalCanBattery battery;

  EXPECT_FALSE(battery.mandatory_charge_taper());
}

// --- CanBattery bus wiring -------------------------------------------------

// Constructing a CAN driver is what puts it on a bus. Nothing else registers
// it, so a driver that is constructed but never registered would silently
// receive nothing.
TEST_F(DriverContractTest, ConstructionRegistersTheDriverOnItsInterface) {
  ASSERT_EQ(can_receiver_count(), 0u);

  MinimalCanBattery battery(CAN_ADDON_MCP2515);

  EXPECT_EQ(can_receiver_count(), 1u);
  EXPECT_TRUE(can_receiver_registered(CAN_ADDON_MCP2515));
  EXPECT_FALSE(can_receiver_registered(CAN_NATIVE));
}

TEST_F(DriverContractTest, TheRequestedSpeedIsWhatGetsRegistered) {
  // A non-default speed, so this pins that the driver's request is carried
  // through rather than the class default being echoed back.
  class SlowBattery : public CanBattery {
   public:
    SlowBattery() : CanBattery(CAN_NATIVE, CAN_Speed::CAN_SPEED_125KBPS) {}
    void setup() override {}
    void update_values() override {}
    void handle_incoming_can_frame(CAN_frame rx_frame) override {}
    void transmit_can(unsigned long currentMillis) override {}
  };

  SlowBattery battery;

  EXPECT_EQ(can_receiver_speed(CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS), CAN_Speed::CAN_SPEED_125KBPS);
}

// A frame delivered to the registry has to reach the driver's own handler.
TEST_F(DriverContractTest, ReceivedFramesReachTheDriversHandler) {
  MinimalCanBattery battery(CAN_NATIVE);

  CAN_frame frame = frame_with_id(0x2A0);
  deliver_to_receivers(&frame, CAN_NATIVE);

  ASSERT_EQ(battery.received_ids.size(), 1u);
  EXPECT_EQ(battery.received_ids[0], 0x2A0u);
}

// Two drivers on different buses must not see each other's traffic - the
// property that makes a double-battery setup work at all.
TEST_F(DriverContractTest, DriversOnDifferentBusesDoNotSeeEachOthersFrames) {
  MinimalCanBattery on_native(CAN_NATIVE);
  MinimalCanBattery on_addon(CAN_ADDON_MCP2515);

  CAN_frame frame = frame_with_id(0x3B0);
  deliver_to_receivers(&frame, CAN_ADDON_MCP2515);

  EXPECT_TRUE(on_native.received_ids.empty());
  ASSERT_EQ(on_addon.received_ids.size(), 1u);
  EXPECT_EQ(on_addon.received_ids[0], 0x3B0u);
}

// handle_incoming_can_frame takes the frame BY VALUE, so a driver gets its own
// copy and cannot disturb what other drivers on the same bus then receive.
TEST_F(DriverContractTest, EachDriverOnABusGetsItsOwnCopyOfTheFrame) {
  class MutatingBattery : public MinimalCanBattery {
   public:
    MutatingBattery() : MinimalCanBattery(CAN_NATIVE) {}
    void handle_incoming_can_frame(CAN_frame rx_frame) override {
      rx_frame.ID = 0xDEAD;  // must not be visible to anyone else
      received_ids.push_back(rx_frame.ID);
    }
  };

  MutatingBattery first;
  MinimalCanBattery second(CAN_NATIVE);

  CAN_frame frame = frame_with_id(0x111);
  deliver_to_receivers(&frame, CAN_NATIVE);

  ASSERT_EQ(second.received_ids.size(), 1u);
  EXPECT_EQ(second.received_ids[0], 0x111u) << "one driver's edit must not reach the next";
  EXPECT_EQ(frame.ID, 0x111u) << "nor the caller's frame";
}

// The transmit hook the core loop drives has to reach the driver's own method,
// with the timestamp it was given.
TEST_F(DriverContractTest, TransmitDelegatesToTheDriverWithTheTimestamp) {
  MinimalCanBattery battery(CAN_NATIVE);

  battery.transmit(123456);

  ASSERT_EQ(battery.transmit_calls.size(), 1u);
  EXPECT_EQ(battery.transmit_calls[0], 123456u);
}
