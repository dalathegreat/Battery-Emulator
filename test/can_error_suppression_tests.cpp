#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* A BMS reset takes the battery off the bus on purpose. With no other node left to
   acknowledge them, the emulator's frames fail to send and the controller reports bus
   errors, which used to surface as "CAN failed to send" and "Multiple CAN TX/RX errors"
   on every reset. These pin that they are reported normally, and only silenced for the
   part of a reset where the battery is expected to be gone. */

namespace {

void setup_can_error_test() {
  datalayer = DataLayer();
  reset_all_events();
  init_hal();
  datalayer.system.info.CPU_free_heap = 200000;  // avoid tripping the low-heap check
}

bool event_is_active(EVENTS_ENUM_TYPE event) {
  return get_event_pointer(event)->state == EVENT_STATE_ACTIVE;
}

}  // namespace

TEST(CanErrorSuppressionTests, ShouldReportCanFaultsDuringNormalOperation) {
  setup_can_error_test();
  datalayer.system.status.bms_reset_status = BMS_RESET_IDLE;

  datalayer.system.info.can_native_send_fail = true;
  datalayer.system.info.can_native_bus_error = true;
  update_machineryprotection();

  EXPECT_TRUE(event_is_active(EVENT_CAN_NATIVE_BUFFER_FULL));
  EXPECT_TRUE(event_is_active(EVENT_CAN_NATIVE_BUS_ERROR));
}

TEST(CanErrorSuppressionTests, ShouldNotReportCanFaultsWhileTheBatteryIsBeingResetAway) {
  setup_can_error_test();

  for (auto state : {BMS_RESET_SHUTDOWN_SEQUENCE, BMS_RESET_POWERED_OFF, BMS_RESET_POWERING_ON}) {
    datalayer.system.status.bms_reset_status = state;
    datalayer.system.info.can_native_send_fail = true;
    datalayer.system.info.can_native_bus_error = true;
    update_machineryprotection();

    EXPECT_FALSE(event_is_active(EVENT_CAN_NATIVE_BUFFER_FULL)) << "state " << state;
    EXPECT_FALSE(event_is_active(EVENT_CAN_NATIVE_BUS_ERROR)) << "state " << state;
    // The latches must be consumed, or they would fire on the first pass after the reset.
    EXPECT_FALSE(datalayer.system.info.can_native_send_fail);
    EXPECT_FALSE(datalayer.system.info.can_native_bus_error);
  }
}

/* Waiting for the battery to go idle happens before anything is done to it, so a fault
   there is still a real one. */
TEST(CanErrorSuppressionTests, ShouldStillReportCanFaultsWhileWaitingForPause) {
  setup_can_error_test();
  datalayer.system.status.bms_reset_status = BMS_RESET_WAITING_FOR_PAUSE;

  datalayer.system.info.can_native_send_fail = true;
  update_machineryprotection();

  EXPECT_TRUE(event_is_active(EVENT_CAN_NATIVE_BUFFER_FULL));
}
