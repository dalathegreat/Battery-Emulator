#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64() to control the test clock

#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

// Covers the contactor closing sequence in handle_contactors(): the startup
// inhibit, the timed negative -> precharge -> positive -> economize ladder, the
// permission gate, and the unrecoverable fault latch. The equipment-stop
// opening path is covered by estop_graceful_open_tests.cpp; this file starts
// where that one leaves off.
//
// Assertions are on datalayer state (contactors_engaged, dc_bus_live) rather
// than on pin writes: those two are what the webserver, MQTT and the rest of
// the firmware actually read, so they are the observable contract.

namespace {

// Mirrors the file-scope FSM in comm_contactorcontrol.cpp. Must match it.
enum SeqState { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };

// Values published in datalayer.system.status.contactors_engaged. Named here
// because the production code writes the numbers inline.
constexpr uint8_t kEngagedNone = 0;
constexpr uint8_t kEngagedEconomized = 1;
constexpr uint8_t kEngagedFaultLatched = 2;
constexpr uint8_t kEngagedClosing = 3;

// Timings from comm_contactorcontrol.cpp. Kept in step with it deliberately:
// if the ladder is retimed these tests must be revisited, not silently pass.
constexpr unsigned long kNegativeToPrechargeMs = 500;
constexpr unsigned long kPrechargeToEconomizeMs = 1000;
constexpr unsigned long kFaultTicksBeforeShutdown = 1000;
constexpr unsigned long kBootMs = 100000;  // Well past INTERVAL_10_S

}  // namespace

extern SeqState contactorStatus;

class ContactorSequenceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    init_hal();
    set_millis64(kBootMs);
    contactor_control_enabled = true;
    contactorStatus = DISCONNECTED;
    emulator_pause_status = NORMAL;
    battery_detected = true;
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.inverter_allows_contactor_closing = true;
    datalayer.system.info.equipment_stop_active = false;

    // handle_contactors() counts consecutive faulted ticks in a file-static
    // that nothing resets, so a test that ran the system in FAULT would leave
    // the next one closer to the shutdown latch than it expects. One pass with
    // the system healthy zeroes it; the state it advances is put back after.
    handle_contactors();
    contactorStatus = DISCONNECTED;
    reset_all_events();
  }

  void TearDown() override {
    contactor_control_enabled = false;
    contactorStatus = DISCONNECTED;
    emulator_pause_status = NORMAL;
    datalayer.system.info.equipment_stop_active = false;
    set_millis64(0);
  }

  // Advances the clock and runs one pass of the state machine.
  static void tick_at(unsigned long ms) {
    set_millis64(ms);
    handle_contactors();
  }
};

// --- The startup inhibit ---------------------------------------------------

// The ladder must not start inside the first 10 seconds after boot. That window
// exists so the battery has a chance to report a fault before anything closes.
TEST_F(ContactorSequenceTest, DoesNotCloseContactorsDuringTheStartupWindow) {
  contactorStatus = START_PRECHARGE;

  for (unsigned long t = 0; t < INTERVAL_10_S; t += 1000) {
    tick_at(t);
    ASSERT_EQ(contactorStatus, START_PRECHARGE) << "the ladder advanced at t=" << t << ", inside the startup window";
  }

  tick_at(INTERVAL_10_S + 1);
  EXPECT_EQ(contactorStatus, PRECHARGE) << "the ladder must start once the window has passed";
}

// No battery communication means nothing is known about the pack, so the ladder
// stays put however long the system has been up.
TEST_F(ContactorSequenceTest, DoesNotCloseContactorsBeforeBatteryIsDetected) {
  battery_detected = false;
  contactorStatus = START_PRECHARGE;

  for (int i = 0; i < 10; ++i) {
    tick_at(kBootMs + i * 1000);
  }

  EXPECT_EQ(contactorStatus, START_PRECHARGE);
}

// --- The permission gate ---------------------------------------------------

TEST_F(ContactorSequenceTest, StaysDisconnectedWithoutInverterPermission) {
  datalayer.system.status.inverter_allows_contactor_closing = false;

  tick_at(kBootMs);

  EXPECT_EQ(contactorStatus, DISCONNECTED);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedNone);
}

TEST_F(ContactorSequenceTest, StaysDisconnectedWhileEquipmentStopIsActive) {
  datalayer.system.info.equipment_stop_active = true;

  tick_at(kBootMs);

  EXPECT_EQ(contactorStatus, DISCONNECTED);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedNone);
}

// The DISCONNECTED check and the ladder's switch both run in the same pass, so
// permission being granted closes the negative contactor on that very tick -
// START_PRECHARGE is never observable from outside. Pinned because it means
// there is no tick of "permitted but nothing driven" to rely on.
TEST_F(ContactorSequenceTest, LeavesDisconnectedOncePermittedAndNoStop) {
  tick_at(kBootMs);

  EXPECT_EQ(contactorStatus, PRECHARGE);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedClosing);
}

// --- The closing ladder ----------------------------------------------------

// Negative first, then precharge after a settling delay, then positive once the
// precharge timer expires, and only then is the precharge resistor removed and
// the coils economized. Each step is gated on its own timer; running them early
// would close the positive contactor before the DC link has charged.
TEST_F(ContactorSequenceTest, ClosesInOrderWithEachStepGatedOnItsTimer) {
  const unsigned long negative_closed_at = kBootMs;

  // One pass: permission granted and the negative contactor closed.
  tick_at(negative_closed_at);
  ASSERT_EQ(contactorStatus, PRECHARGE);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedClosing);

  // Too early for precharge.
  tick_at(negative_closed_at + kNegativeToPrechargeMs - 1);
  EXPECT_EQ(contactorStatus, PRECHARGE) << "precharge engaged before the negative settling time elapsed";

  tick_at(negative_closed_at + kNegativeToPrechargeMs);
  ASSERT_EQ(contactorStatus, POSITIVE);
  const unsigned long precharge_started_at = negative_closed_at + kNegativeToPrechargeMs;

  // Too early for the positive contactor.
  tick_at(precharge_started_at + precharge_time_ms - 1);
  EXPECT_EQ(contactorStatus, POSITIVE) << "positive contactor closed before precharge finished";

  tick_at(precharge_started_at + precharge_time_ms);
  ASSERT_EQ(contactorStatus, PRECHARGE_OFF);
  const unsigned long positive_closed_at = precharge_started_at + precharge_time_ms;

  // Too early to drop the precharge resistor.
  tick_at(positive_closed_at + kPrechargeToEconomizeMs - 1);
  EXPECT_EQ(contactorStatus, PRECHARGE_OFF);

  tick_at(positive_closed_at + kPrechargeToEconomizeMs);
  EXPECT_EQ(contactorStatus, COMPLETED);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedEconomized);
}

// dc_bus_live is what tells the rest of the firmware that the DC link is
// energized. It must be false for every step of the closing ladder and true
// only once the sequence has completed.
TEST_F(ContactorSequenceTest, DcBusIsLiveOnlyWhenTheSequenceHasCompleted) {
  const unsigned long t = kBootMs;

  tick_at(t);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live) << "reported live while only the negative contactor is closed";

  tick_at(t + kNegativeToPrechargeMs);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live) << "reported live during precharge";

  tick_at(t + kNegativeToPrechargeMs + precharge_time_ms);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live) << "reported live before the sequence completed";

  tick_at(t + kNegativeToPrechargeMs + precharge_time_ms + kPrechargeToEconomizeMs);
  ASSERT_EQ(contactorStatus, COMPLETED);
  // Still false on the completing tick: the flag is computed from the status at
  // the top of the pass, before the ladder advances.
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  tick_at(t + kNegativeToPrechargeMs + precharge_time_ms + kPrechargeToEconomizeMs + 1);
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);
}

// --- The fault latch -------------------------------------------------------

// A sustained FAULT opens the contactors and latches: recovery requires a power
// cycle, deliberately, so a fault that comes and goes cannot re-close the
// contactors behind the operator's back.
TEST_F(ContactorSequenceTest, SustainedFaultLatchesShutdownAndDoesNotRecover) {
  contactorStatus = COMPLETED;
  datalayer.system.status.system_status = FAULT;

  // The latch is a tick count, not a wall-clock time. Tick until it trips
  // rather than assuming which iteration does it, so the assertions below
  // describe the transition itself and not a position in the loop.
  unsigned long tick = kBootMs;
  for (unsigned long i = 0; i <= kFaultTicksBeforeShutdown + 1; ++i) {
    tick_at(tick++);
    if (contactorStatus == SHUTDOWN_REQUESTED) {
      break;
    }
  }

  ASSERT_EQ(contactorStatus, SHUTDOWN_REQUESTED);
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedFaultLatched);

  // dc_bus_live is computed from the status at the top of the pass, so on the
  // tick that latches the shutdown it still reads true even though the pins
  // have just been driven open. It corrects itself on the following pass.
  EXPECT_TRUE(datalayer.system.status.dc_bus_live) << "one tick of staleness is the current behaviour";
  tick_at(tick++);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  // Fault clears and everything is permitted again: the latch must hold.
  datalayer.system.status.system_status = ACTIVE;
  datalayer.system.status.inverter_allows_contactor_closing = true;
  for (int i = 0; i < 50; ++i) {
    tick_at(kBootMs + kFaultTicksBeforeShutdown + 100 + i);
  }

  EXPECT_EQ(contactorStatus, SHUTDOWN_REQUESTED) << "the fault latch must survive the fault clearing";
  EXPECT_EQ(datalayer.system.status.contactors_engaged, kEngagedFaultLatched);
}

// A fault shorter than the allowance must not latch: brief faults are expected
// during normal operation and latching on them would strand the system.
TEST_F(ContactorSequenceTest, BriefFaultDoesNotLatchShutdown) {
  contactorStatus = COMPLETED;
  datalayer.system.status.system_status = FAULT;

  // Each episode is most of the allowance, so together they comfortably exceed
  // it: if the recovery did not zero the counter, the second episode would
  // latch and this test would catch it.
  const unsigned long episode = kFaultTicksBeforeShutdown - 100;
  unsigned long tick = kBootMs;
  for (unsigned long i = 0; i < episode; ++i) {
    tick_at(tick++);
  }
  ASSERT_EQ(contactorStatus, COMPLETED);

  // Recovering resets the counter, so the next fault starts from zero.
  datalayer.system.status.system_status = ACTIVE;
  tick_at(tick++);

  datalayer.system.status.system_status = FAULT;
  for (unsigned long i = 0; i < episode; ++i) {
    tick_at(tick++);
  }

  EXPECT_EQ(contactorStatus, COMPLETED) << "two short faults must not add up to a latch";
}

// The latch is strictly greater-than the allowance, so exactly the allowance
// must not trip it. An off-by-one here would open the contactors a tick early.
TEST_F(ContactorSequenceTest, ExactlyTheAllowedFaultTicksDoesNotLatch) {
  contactorStatus = COMPLETED;
  datalayer.system.status.system_status = FAULT;

  for (unsigned long i = 0; i < kFaultTicksBeforeShutdown; ++i) {
    tick_at(kBootMs + i);
  }

  EXPECT_EQ(contactorStatus, COMPLETED) << "latched at the threshold instead of beyond it";
}

TEST_F(ContactorSequenceTest, ShutdownLatchRaisesTheOpenContactorEvent) {
  contactorStatus = COMPLETED;
  datalayer.system.status.system_status = FAULT;

  for (unsigned long i = 0; i <= kFaultTicksBeforeShutdown + 1 && contactorStatus != SHUTDOWN_REQUESTED; ++i) {
    tick_at(kBootMs + i);
  }
  ASSERT_EQ(contactorStatus, SHUTDOWN_REQUESTED);

  const EVENTS_STRUCT_TYPE* entry = get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR);
  EXPECT_TRUE(entry->state == EVENT_STATE_ACTIVE || entry->state == EVENT_STATE_ACTIVE_LATCHED);
}

// --- Disabled control ------------------------------------------------------

// With contactor control off the firmware drives no contactors at all; the
// state machine must stay where it is rather than tracking permissions.
TEST_F(ContactorSequenceTest, DoesNothingWhenContactorControlIsDisabled) {
  contactor_control_enabled = false;

  for (int i = 0; i < 20; ++i) {
    tick_at(kBootMs + i * 100);
  }

  EXPECT_EQ(contactorStatus, DISCONNECTED);
}
