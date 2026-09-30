#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64()

#include "../Software/src/battery/MG-GEN1-BATTERY.h"
#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "stub_pack.h"

// Tests how the handle_contactors() contactor coordinator invites packs and
// manages the link contactor based on the various conditions.

namespace {

// Mirrors the file-scope FSM in comm_contactorcontrol.cpp. Must match it.
enum SeqState { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };

constexpr unsigned long kBootMs = 100000;  // Well past the ladder's startup window

}  // namespace

extern SeqState contactorStatus;

class ContactorCoordinationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(kBootMs);
    contactor_control_enabled = false;
    contactor_control_enabled_double_battery = false;
    contactor_control_inverted_logic = false;
    pwm_contactor_control = false;
    periodic_bms_reset = false;
    remote_bms_reset = false;
    contactorStatus = DISCONNECTED;
    emulator_pause_status = NORMAL;
    pack1 = install_stub_pack(1);
    pack2 = install_stub_pack(2);
    datalayer.system.status.battery2_voltage_matches = true;
  }

  void TearDown() override {
    contactor_control_enabled = false;
    contactor_control_enabled_double_battery = false;
    contactorStatus = DISCONNECTED;
    battery_detected = false;
    battery2_detected = false;
    set_millis64(0);
  }

  static void tick() { handle_contactors(); }
  static void tick_at(unsigned long ms) {
    set_millis64(ms);
    handle_contactors();
  }
  static bool joined() { return datalayer.system.status.battery2_joined; }
  static bool invited(uint8_t n) { return datalayer.system.status.contactor_invite[n - 1]; }
  static bool link_closed() { return datalayer.system.status.contactors_battery2_engaged; }

  StubPack* pack1 = nullptr;
  StubPack* pack2 = nullptr;
};

TEST_F(ContactorCoordinationTest, PacksReportingReadyAreInvited) {
  pack1->state = ContactorState::READY;
  pack2->state = ContactorState::READY;
  contactor_control_enabled_double_battery = true;  // So battery 2 need not wait for battery 1
  tick();
  EXPECT_TRUE(invited(1));
  EXPECT_TRUE(invited(2));
}

TEST_F(ContactorCoordinationTest, UnknownPacksAreNotInvited) {
  pack1->state = ContactorState::UNKNOWN;
  pack2->state = ContactorState::UNKNOWN;
  contactor_control_enabled_double_battery = true;
  tick();
  EXPECT_FALSE(invited(1));
  EXPECT_FALSE(invited(2));
}

// A pack that has stopped talking could be in any state, whatever it last reported
TEST_F(ContactorCoordinationTest, SilentPacksAreNotInvited) {
  pack1->state = ContactorState::READY;
  pack2->state = ContactorState::READY;
  contactor_control_enabled_double_battery = true;
  datalayer.battery.status.CAN_battery_still_alive = 0;
  datalayer.battery2.status.CAN_battery_still_alive = 0;
  tick();
  EXPECT_FALSE(invited(1));
  EXPECT_FALSE(invited(2));
}

TEST_F(ContactorCoordinationTest, SystemFaultUninvitesEveryPackAndOpensTheLink) {
  contactor_control_enabled_double_battery = true;
  tick();
  ASSERT_TRUE(invited(1));
  ASSERT_TRUE(invited(2));
  ASSERT_TRUE(link_closed());

  datalayer.system.status.system_status = FAULT;
  tick();
  EXPECT_FALSE(invited(1));
  EXPECT_FALSE(invited(2));
  EXPECT_FALSE(link_closed());
}

TEST_F(ContactorCoordinationTest, InverterForbiddingClosingUninvitesEveryPack) {
  tick();
  ASSERT_TRUE(invited(1));
  ASSERT_TRUE(invited(2));

  datalayer.system.status.inverter_allows_contactor_closing = false;
  tick();
  EXPECT_FALSE(invited(1));
  EXPECT_FALSE(invited(2));
}

TEST_F(ContactorCoordinationTest, VoltageMismatchKeepsBattery2OffTheDcLink) {
  pack1->state = ContactorState::CLOSED;
  datalayer.system.status.battery2_voltage_matches = false;
  tick();
  EXPECT_TRUE(invited(1));
  EXPECT_FALSE(joined());
  EXPECT_FALSE(invited(2));  // Without a link contactor, the invite is what would join it
}

// Behind a link contactor the pack may close its own contactors, but the link stays open
TEST_F(ContactorCoordinationTest, VoltageMismatchKeepsTheLinkOpen) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::CLOSED;
  datalayer.system.status.battery2_voltage_matches = false;
  tick();
  EXPECT_TRUE(invited(2));
  EXPECT_FALSE(joined());
  EXPECT_FALSE(link_closed());
}

// --- Battery 2 without a link contactor -----------------------------------------------------------

// It would be energising the shared DC link itself, so battery 1 has to do that first
TEST_F(ContactorCoordinationTest, WithoutLinkBattery2WaitsForBattery1ToClose) {
  pack1->state = ContactorState::READY;
  pack2->state = ContactorState::READY;
  tick();
  EXPECT_FALSE(invited(2));

  pack1->state = ContactorState::PRECHARGING;
  tick();
  EXPECT_FALSE(invited(2));

  pack1->state = ContactorState::CLOSED;
  tick();
  EXPECT_TRUE(invited(2));
}

// Battery 2 is only ever joined to a live battery 1. If battery 1 opens, battery 2 leaves too, so
// battery 1 recloses onto a dead DC link rather than onto one battery 2 is holding up
TEST_F(ContactorCoordinationTest, WithoutLinkBattery2LeavesWhenBattery1Opens) {
  pack1->state = ContactorState::CLOSED;
  tick();
  ASSERT_TRUE(invited(2));

  pack1->state = ContactorState::READY;
  tick();
  EXPECT_FALSE(invited(2));
  EXPECT_TRUE(invited(1));  // Battery 1 may close again, onto the dead DC link
}

// On rejoin, the voltages have to match again.
TEST_F(ContactorCoordinationTest, WithoutLinkBattery2RejoinsOnlyWhenTheVoltagesMatch) {
  pack1->state = ContactorState::CLOSED;
  tick();
  pack1->state = ContactorState::READY;
  tick();
  ASSERT_FALSE(invited(2));

  datalayer.system.status.battery2_voltage_matches = false;  // Drifted apart while battery 1 was open
  pack1->state = ContactorState::CLOSED;
  tick();
  EXPECT_FALSE(invited(2));

  datalayer.system.status.battery2_voltage_matches = true;
  tick();
  EXPECT_TRUE(invited(2));
}

// A joined pack leaves after 10 seconds of voltage mismatch
TEST_F(ContactorCoordinationTest, JoinedPackLeavesAfter10SecondsOfMismatch) {
  pack1->state = ContactorState::CLOSED;
  tick_at(kBootMs);
  ASSERT_TRUE(joined());

  datalayer.system.status.battery2_voltage_matches = false;
  tick_at(kBootMs + 10000);
  EXPECT_TRUE(joined());
  EXPECT_TRUE(invited(2));

  tick_at(kBootMs + 10001);
  EXPECT_FALSE(joined());
  EXPECT_FALSE(invited(2));
}

// Any matching reading during the mismatch restarts the 10s
TEST_F(ContactorCoordinationTest, AMatchingReadingRestartsTheMismatchAllowance) {
  pack1->state = ContactorState::CLOSED;
  tick_at(kBootMs);
  datalayer.system.status.battery2_voltage_matches = false;
  tick_at(kBootMs + 9000);
  datalayer.system.status.battery2_voltage_matches = true;
  tick_at(kBootMs + 9500);
  datalayer.system.status.battery2_voltage_matches = false;
  tick_at(kBootMs + 19000);
  EXPECT_TRUE(joined());

  tick_at(kBootMs + 19501);
  EXPECT_FALSE(joined());
}

// The same allowance applies behind a link contactor, which opens when the pack leaves
TEST_F(ContactorCoordinationTest, LinkOpensAfter10SecondsOfMismatch) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::CLOSED;
  tick_at(kBootMs);
  ASSERT_TRUE(link_closed());

  datalayer.system.status.battery2_voltage_matches = false;
  tick_at(kBootMs + 10000);
  EXPECT_TRUE(link_closed());

  tick_at(kBootMs + 10001);
  EXPECT_FALSE(link_closed());
  EXPECT_TRUE(invited(2));  // Still free to stay closed on its own isolated side
}

// After battery 1 drops out, the link only closes again once the voltages match
TEST_F(ContactorCoordinationTest, LinkReclosesOnlyWhenTheVoltagesMatch) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::CLOSED;
  tick();
  ASSERT_TRUE(link_closed());

  pack1->state = ContactorState::READY;  // Battery 1 opens unexpectedly
  tick();
  EXPECT_FALSE(link_closed());

  datalayer.system.status.battery2_voltage_matches = false;  // Drifted apart while battery 1 was open
  pack1->state = ContactorState::CLOSED;
  tick();
  EXPECT_FALSE(link_closed());

  datalayer.system.status.battery2_voltage_matches = true;
  tick();
  EXPECT_TRUE(link_closed());
}

// Check battery2 doesn't join until the main contactors are closed
TEST_F(ContactorCoordinationTest, WithoutLinkBattery2WaitsForTheMainContactors) {
  contactor_control_enabled = true;
  pack1->state = ContactorState::CLOSED;
  tick();
  EXPECT_FALSE(invited(2));

  contactorStatus = COMPLETED;
  tick();
  EXPECT_TRUE(invited(2));
}

// --- Battery 2 behind a link contactor ------------------------------------------------------------

// Check the link contactor waits for the pack to close its own contactors first
TEST_F(ContactorCoordinationTest, LinkClosesOnlyOnceASelfClosingPackHasClosed) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::READY;
  tick();
  EXPECT_TRUE(invited(2));
  EXPECT_FALSE(link_closed());

  pack2->state = ContactorState::PRECHARGING;
  tick();
  EXPECT_FALSE(link_closed());

  pack2->state = ContactorState::CLOSED;
  tick();
  EXPECT_TRUE(link_closed());
}

// Battery 2 may close its own side straight away, but the link waits for a live DC bus
TEST_F(ContactorCoordinationTest, LinkWaitsForBattery1) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::READY;
  pack2->state = ContactorState::CLOSED;
  tick();
  EXPECT_TRUE(invited(2));
  EXPECT_FALSE(link_closed());

  pack1->state = ContactorState::CLOSED;
  tick();
  EXPECT_TRUE(link_closed());
}

// Check that a pack needing external precharge is precharged through the link contactor
TEST_F(ContactorCoordinationTest, PackNeedingPrechargeIsPrechargedThroughTheLink) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::NEEDS_EXTERNAL_PRECHARGE;
  tick();
  EXPECT_TRUE(link_closed());

  for (auto state : {ContactorState::READY, ContactorState::PRECHARGING, ContactorState::CLOSED}) {
    pack2->state = state;
    tick();
    EXPECT_TRUE(link_closed()) << "link opened at state " << (int)state;
    EXPECT_TRUE(invited(2));
  }
}

// Check that the link contactor opens if a pack itself opens
TEST_F(ContactorCoordinationTest, LinkOpensWhenAClosedPackOpensItself) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::CLOSED;
  tick();
  ASSERT_TRUE(link_closed());

  pack2->state = ContactorState::READY;
  tick();
  EXPECT_FALSE(link_closed());
  EXPECT_TRUE(invited(2));  // Still welcome to close again

  pack2->state = ContactorState::CLOSED;
  tick();
  EXPECT_TRUE(link_closed());
}

// Check that a faulted pack stays invited but the link contactor opens
TEST_F(ContactorCoordinationTest, FaultedPackStaysInvitedButTheLinkOpens) {
  contactor_control_enabled_double_battery = true;
  pack1->state = ContactorState::CLOSED;
  pack2->state = ContactorState::CLOSED;
  tick();
  ASSERT_TRUE(link_closed());

  pack2->state = ContactorState::FAULT;
  tick();
  EXPECT_TRUE(invited(2));
  EXPECT_FALSE(link_closed());
}

// Check that packs without feedback join the link as before the feedback mechanism existed
TEST_F(ContactorCoordinationTest, PacksWithoutFeedbackJoinAsBefore) {
  contactor_control_enabled_double_battery = true;
  tick();
  EXPECT_TRUE(invited(1));
  EXPECT_TRUE(invited(2));
  EXPECT_TRUE(link_closed());
}

// --- Battery 1's main GPIO contactors -------------------------------------------------------------

TEST_F(ContactorCoordinationTest, MainContactorsDoNotStartWhileBattery1ReportsFault) {
  contactor_control_enabled = true;
  pack1->state = ContactorState::FAULT;
  tick();
  EXPECT_EQ(contactorStatus, DISCONNECTED);

  pack1->state = ContactorState::READY;
  tick();
  EXPECT_NE(contactorStatus, DISCONNECTED);
}
