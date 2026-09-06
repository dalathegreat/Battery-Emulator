#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/inverter/INVERTERS.h"

// Tests for the inverter CAN aliveness handling in update_machineryprotection().
//
// The detection latch in safety.cpp has no reset of its own, so it used to have
// to be the first test in the file and the suite could not be shuffled. The
// fixture now clears it directly, the same way battery_alive_tests.cpp clears
// the battery and charger latches, so these tests are order-independent.

extern bool inverter_detected;

namespace {

void setup_can_inverter_test() {
  datalayer = DataLayer();
  // Defined in safety.cpp; not exposed via safety.h like the battery latches are.
  inverter_detected = false;
  reset_all_events();
  init_hal();
  // Avoid tripping the low-heap check (CPU_free_heap defaults to 0)
  datalayer.system.info.CPU_free_heap = 200000;
  if (!inverter) {
    // BydCan: a plain CAN inverter without the SMA startup grace window,
    // so EVENT_CAN_INVERTER_MISSING is raised without needing to advance millis()
    user_selected_inverter_protocol = InverterProtocolType::BydCan;
    setup_inverter();
  }
  ASSERT_NE(inverter, nullptr);
}

}  // namespace

// Some drivers deliberately refresh the aliveness counter above CAN_STILL_ALIVE to
// get a longer timeout (SMA x3, Sofar x2). Detection must still fire for them:
// with the previous equality check the detected-event never fired in normal
// operation, and instead fired mid-outage when the decrementing counter happened
// to pass exactly CAN_STILL_ALIVE.
TEST(InverterAliveTests, DetectionFiresWithMultipliedRefreshValue) {
  setup_can_inverter_test();

  // Mimic an SMA-style RX handler refresh
  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE * 3;
  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_CAN_INVERTER_DETECTED)->occurences, 1)
      << "Inverter detection must fire on the first refresh even when the driver "
         "refreshes the counter above CAN_STILL_ALIVE";
}

// Regression test: with the long-CAN-timeout option enabled, a missing-inverter
// event must still clear once the inverter starts sending frames again. The
// clear_event() call was previously only present in the normal-timeout branch,
// so the ERROR-level event (and the resulting FAULT state) survived recovery
// until reboot.
TEST(InverterAliveTests, MissingEventClearsOnRecoveryWithLongTimeout) {
  setup_can_inverter_test();
  user_selected_inverter_long_CAN_timeout = true;

  // Inverter silent: counter has drained to zero -> event raised
  datalayer.system.status.CAN_inverter_still_alive = 0;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state, EVENT_STATE_ACTIVE);

  // Inverter frames arrive again: RX handler refreshes the counter
  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state, EVENT_STATE_INACTIVE)
      << "Missing-inverter event must clear on recovery with the long timeout enabled";

  user_selected_inverter_long_CAN_timeout = false;
}

// Control case: same recovery with the normal timeout (worked before the fix,
// must keep working)
TEST(InverterAliveTests, MissingEventClearsOnRecoveryWithNormalTimeout) {
  setup_can_inverter_test();
  user_selected_inverter_long_CAN_timeout = false;

  datalayer.system.status.CAN_inverter_still_alive = 0;
  update_machineryprotection();
  ASSERT_EQ(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state, EVENT_STATE_ACTIVE);

  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  update_machineryprotection();
  EXPECT_EQ(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state, EVENT_STATE_INACTIVE);
}

// The long-timeout decrement runs at half rate (every 2nd call); make sure the
// hoisted clear_event did not change that behavior.
TEST(InverterAliveTests, LongTimeoutDecrementsOnEveryThirdPass) {
  setup_can_inverter_test();
  user_selected_inverter_long_CAN_timeout = true;

  // The pass counter is a function-local static, so its phase carries over from
  // whatever ran before. Tick until one decrement is observed: that leaves the
  // phase at a known zero.
  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  for (int guard = 0; datalayer.system.status.CAN_inverter_still_alive == CAN_STILL_ALIVE && guard < 4; guard++) {
    update_machineryprotection();
  }
  ASSERT_EQ(datalayer.system.status.CAN_inverter_still_alive, CAN_STILL_ALIVE - 1) << "phase sync failed";

  // From a known phase: six passes are exactly two full 3-tick periods.
  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  for (int i = 0; i < 6; i++) {
    update_machineryprotection();
  }
  EXPECT_EQ(datalayer.system.status.CAN_inverter_still_alive, CAN_STILL_ALIVE - 2)
      << "the long timeout decrements on every third pass - a 3x window, not the 2x the old comment claimed";

  user_selected_inverter_long_CAN_timeout = false;
}
