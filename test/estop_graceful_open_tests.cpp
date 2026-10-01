#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64() to control the test clock

#include "../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

// Mirrors the file-scope contactor FSM in comm_contactorcontrol.cpp so the
// tests can place it in a known state. Must match the definition there.
enum State { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
extern State contactorStatus;

// Regression tests for #1750: equipment stop used to jump COMPLETED ->
// DISCONNECTED in the same 10 ms tick that zeroed the power limits - faster
// than any inverter can ramp down, so the contactors opened under load.
// The e-stop path now waits for the pause state machine to report PAUSED
// (current below 1.8 A) or a bounded timeout before opening.

class EstopGracefulOpenTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    reset_all_events();
    init_hal();
    set_millis64(100000);
    contactor_control_enabled = true;
    contactorStatus = COMPLETED;
    emulator_pause_status = NORMAL;
    battery_detected = true;
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.inverter_allows_contactor_closing = true;
    datalayer.system.info.equipment_stop_active = false;
  }

  void TearDown() override {
    contactor_control_enabled = false;
    contactorStatus = DISCONNECTED;
    emulator_pause_status = NORMAL;
    datalayer.system.info.equipment_stop_active = false;
    set_millis64(0);
  }
};

TEST_F(EstopGracefulOpenTest, EstopHoldsContactorsUntilTimeoutWhenCurrentFlows) {
  datalayer.system.info.equipment_stop_active = true;
  // Pause was requested but current is still flowing: not PAUSED yet

  handle_contactors();
  EXPECT_EQ(contactorStatus, COMPLETED) << "Contactors must not open under load in the same tick";

  set_millis64(100000 + 5000);
  handle_contactors();
  EXPECT_EQ(contactorStatus, COMPLETED) << "Still within the timeout window";
  EXPECT_EQ(get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR)->state, EVENT_STATE_INACTIVE);

  set_millis64(100000 + 8000);
  handle_contactors();
  EXPECT_EQ(contactorStatus, DISCONNECTED) << "The wait is bounded: open anyway after the 7 s timeout";
  EXPECT_EQ(get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR)->state, EVENT_STATE_ACTIVE)
      << "A forced open under load must be visible in the event log";
  EXPECT_EQ(get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR)->data, 1);
}

TEST_F(EstopGracefulOpenTest, EstopOpensPromptlyOncePaused) {
  datalayer.system.info.equipment_stop_active = true;
  emulator_pause_status = PAUSED;

  handle_contactors();

  EXPECT_EQ(contactorStatus, DISCONNECTED) << "Zero current reached: open without waiting for the timeout";
  EXPECT_EQ(get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR)->state, EVENT_STATE_INACTIVE)
      << "A graceful open is not an error";
}

TEST_F(EstopGracefulOpenTest, EstopOpensWhenPauseCompletesMidWait) {
  datalayer.system.info.equipment_stop_active = true;

  handle_contactors();
  EXPECT_EQ(contactorStatus, COMPLETED);

  // The inverter ramps down and the pause FSM reaches PAUSED within the window
  set_millis64(100000 + 3000);
  emulator_pause_status = PAUSED;
  handle_contactors();

  EXPECT_EQ(contactorStatus, DISCONNECTED);
  EXPECT_EQ(get_event_pointer(EVENT_ERROR_OPEN_CONTACTOR)->state, EVENT_STATE_INACTIVE);
}

TEST_F(EstopGracefulOpenTest, InverterCommandedOpeningStaysImmediate) {
  datalayer.system.status.inverter_allows_contactor_closing = false;

  handle_contactors();

  EXPECT_EQ(contactorStatus, DISCONNECTED)
      << "Inverter-commanded opening keeps today's immediate behavior - the inverter has already stopped power";
}
