#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64() to control the test clock

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/inverter/INVERTERS.h"

// Regression tests for millis() 32-bit wrap handling (2^32 ms = 49.7 days).
// The emulated millis() truncates to 32 bits like the ESP32 does, and
// production time variables are uint32_t, so the host runs exactly the
// arithmetic the target runs - setting the 64-bit test clock past 2^32 makes
// millis() read small values again while millis64() keeps counting.

TEST(MillisWrapTest, EmulatedClockTruncatesLikeTheTarget) {
  set_millis64(0x100000000ULL + 1000);
  EXPECT_EQ(millis(), 1000UL);
  EXPECT_EQ(millis64(), 0x100000000ULL + 1000);
  set_millis64(0);
}

// The SMA-family startup grace window is a since-boot gate: with a plain
// millis() comparison it would re-arm for 5 minutes after every wrap,
// suppressing detection of a new inverter-comms loss as if the system had
// just booted.
TEST(MillisWrapTest, InverterStartupGraceDoesNotRearmAfterWrap) {
  datalayer = DataLayer();
  reset_all_events();
  init_hal();
  // Avoid tripping the low-heap check (CPU_free_heap defaults to 0)
  datalayer.system.info.CPU_free_heap = 200000;
  if (!inverter) {
    user_selected_inverter_protocol = InverterProtocolType::SmaBydH;
    setup_inverter();
  }
  ASSERT_NE(inverter, nullptr);
  ASSERT_TRUE(inverter->needs_can_startup_grace());

  set_millis64(0x100000000ULL + 1000);
  datalayer.system.status.CAN_inverter_still_alive = 0;

  update_machineryprotection();

  EXPECT_EQ(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state, EVENT_STATE_ACTIVE)
      << "The startup grace window must not re-arm after the millis() wrap";
  set_millis64(0);
}

// Remote-limit expiry across the wrap, both failure directions of the old
// "currentMillis > timestamp + timeout" comparison:
// (a) a limit set just before the wrap must not expire immediately - the sum
//     overflows to a small number, so any pre-wrap check tripped it, and
// (b) a limit whose timeout genuinely elapsed across the wrap must still
//     expire - post-wrap currentMillis is small, so the inverted comparison
//     kept stale limits alive.
TEST(MillisWrapTest, RemoteLimitExpiryIsWrapSafe) {
  datalayer = DataLayer();

  // (a) Set 16 ms before the wrap; timestamp + timeout overflows to ~59984
  datalayer.battery.settings.remote_set_timestamp = 0xFFFFFFF0u;
  datalayer.battery.settings.remote_set_timeout = 60000;
  datalayer.battery.settings.remote_settings_limit_charge = true;
  datalayer.battery.settings.remote_settings_limit_discharge = true;
  datalayer.battery.settings.max_remote_set_charge_dA = 100;
  datalayer.battery.settings.max_remote_set_discharge_dA = 100;

  // 8 ms later, still before the wrap: must survive
  update_remote_limit_expiry(0xFFFFFFF8u);
  EXPECT_TRUE(datalayer.battery.settings.remote_settings_limit_charge)
      << "A fresh remote limit must not expire because timestamp + timeout overflowed";
  // 1 s after the wrap (~1 s elapsed): must still survive
  update_remote_limit_expiry(1000);
  EXPECT_TRUE(datalayer.battery.settings.remote_settings_limit_charge)
      << "A fresh remote limit must not expire just because the clock wrapped";
  // 70 s after the wrap: the 60 s timeout has genuinely elapsed
  update_remote_limit_expiry(70000);
  EXPECT_FALSE(datalayer.battery.settings.remote_settings_limit_charge);

  // (b) Set ~65 s before the wrap; the sum does NOT overflow (~4294961760),
  // so after the wrap the naive comparison never becomes true again
  datalayer.battery.settings.remote_set_timestamp = 0xFFFF0000u;
  datalayer.battery.settings.remote_set_timeout = 60000;
  datalayer.battery.settings.remote_settings_limit_charge = true;
  datalayer.battery.settings.remote_settings_limit_discharge = true;
  datalayer.battery.settings.max_remote_set_charge_dA = 100;
  datalayer.battery.settings.max_remote_set_discharge_dA = 100;

  // 70 s after the wrap: ~135 s elapsed, far past the 60 s timeout
  update_remote_limit_expiry(70000);
  EXPECT_FALSE(datalayer.battery.settings.remote_settings_limit_charge)
      << "A stale remote limit must not persist across the wrap";
  EXPECT_FALSE(datalayer.battery.settings.remote_settings_limit_discharge);
  EXPECT_EQ(datalayer.battery.settings.max_remote_set_charge_dA, 0);
  EXPECT_EQ(datalayer.battery.settings.max_remote_set_discharge_dA, 0);
}
