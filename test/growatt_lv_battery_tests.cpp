#include <gtest/gtest.h>

#include <Arduino.h>  // Emul: set_millis64() to control the test clock
#include <initializer_list>

#include "../Software/src/battery/GROWATT-LV-BATTERY.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/types.h"

// Regression test for issue #3034
// (github.com/dalathegreat/Battery-Emulator/issues/3034): only 0x311
// refreshes CAN_battery_still_alive, and the charge/discharge limits it
// carries must go to zero once it has been missing for more than five 1 Hz
// query periods, independently of whatever other frame IDs keep arriving.

namespace {

// Builds an 8-byte CAN frame from a byte list, the same way issue #3034's own
// repro test did.
static CAN_frame Frame(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame f{};
  f.ID = id;
  f.DLC = 8;
  int i = 0;
  for (uint8_t b : bytes) {
    f.data.u8[i++] = b;
  }
  return f;
}

// 0x311: charge voltage (0.1V), charge current limit (0.1A), discharge
// current limit (0.1A), status word (bit 5 = discharge enable, bit 6 =
// charge enable, both in the low/second byte of the pair).
static CAN_frame Frame311(uint16_t ccl_dA, uint16_t dcl_dA, bool charge_en, bool discharge_en, uint16_t cv_dV = 576) {
  uint8_t status_low = (uint8_t)((discharge_en ? 0x20 : 0) | (charge_en ? 0x40 : 0));
  return Frame(0x311, {(uint8_t)(cv_dV >> 8), (uint8_t)(cv_dV & 0xFF), (uint8_t)(ccl_dA >> 8), (uint8_t)(ccl_dA & 0xFF),
                       (uint8_t)(dcl_dA >> 8), (uint8_t)(dcl_dA & 0xFF), 0x00, status_low});
}

// 0x313: pack voltage (0.01V), current (0.1A), temperature (0.1C), SOC, SOH.
// Values are arbitrary and only need to be present/well-formed - this frame
// stands in for "some other frame keeps arriving" in the staleness tests.
static CAN_frame Frame313() {
  return Frame(0x313, {0x14, 0xB4, 0x00, 0x00, 0x00, 0xC8, 0x50, 0x64});  // 53.00V, 0A, 20.0C, 80%, 100%
}

// Replicates check_can_component_alive()'s decrement (safety.cpp): only
// decrements while nonzero. check_can_component_alive() itself is static to
// safety.cpp and not callable from here.
static void DecrementAliveCounterLikeSafetyDoes() {
  if (datalayer.battery.status.CAN_battery_still_alive > 0) {
    datalayer.battery.status.CAN_battery_still_alive--;
  }
}

}  // namespace

// The #3034 repro: one 0x311 with 100A limits and both directions enabled,
// then only 0x313 once a second for 600 simulated seconds, with the
// aliveness counter decremented every second the way check_can_component_alive
// does. Before the fix, the counter never reached zero (no missing-event) and
// the 100A limits were published forever; after the fix, the counter is
// exhausted at 60s and the limits are zeroed once 0x311 is more than 5s old.
TEST(GrowattLvStaleness, LimitsAndAlivenessExpireWhen0x311StopsButOthersContinue) {
  datalayer = DataLayer();
  set_millis64(0);
  GrowattLvBattery b;

  b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
  b.update_values();
  ASSERT_EQ(datalayer.battery.status.max_charge_current_dA, 1000);
  ASSERT_EQ(datalayer.battery.status.max_discharge_current_dA, 1000);
  ASSERT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);

  bool checked_5s_zero = false;
  for (int s = 1; s <= 600; s++) {
    set_millis64((uint64_t)s * 1000ULL);
    DecrementAliveCounterLikeSafetyDoes();
    b.handle_incoming_can_frame(Frame313());
    b.update_values();

    if (s == 6) {  // more than 5s (five missed 1Hz queries) since the only 0x311
      EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 0)
          << "limits must be zeroed once 0x311 is more than 5s stale";
      EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 0);
      checked_5s_zero = true;
    }
  }
  EXPECT_TRUE(checked_5s_zero);

  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 0)
      << "only 0x311 may refresh the aliveness counter - 0x313 alone must let it "
         "run out, which is what makes safety.cpp raise the battery-missing event";
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 0);
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 0);
}

// A healthy pack sending a fresh 0x311 every second must keep both the
// limits and the aliveness counter exactly as they were - the fix must not
// introduce any staleness where there wasn't one before.
TEST(GrowattLvStaleness, FreshFrameEverySecondKeepsLimitsAndAlivenessFull) {
  datalayer = DataLayer();
  set_millis64(0);
  GrowattLvBattery b;

  b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
  b.update_values();
  ASSERT_EQ(datalayer.battery.status.max_charge_current_dA, 1000);

  for (int s = 1; s <= 600; s++) {
    set_millis64((uint64_t)s * 1000ULL);
    DecrementAliveCounterLikeSafetyDoes();
    b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
    b.update_values();
  }

  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 1000);
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 1000);
}

// Once 0x311 comes back after a stale period, the limits must return
// immediately - staleness is not a one-way latch.
TEST(GrowattLvStaleness, LimitsReturnOnceAFreshFrameArrivesAfterBeingStale) {
  datalayer = DataLayer();
  set_millis64(0);
  GrowattLvBattery b;

  b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
  b.update_values();
  ASSERT_EQ(datalayer.battery.status.max_charge_current_dA, 1000);

  set_millis64(6000);  // >5s since the only 0x311, no new frame
  b.update_values();
  ASSERT_EQ(datalayer.battery.status.max_charge_current_dA, 0) << "limits must be zero while 0x311 is stale";
  ASSERT_EQ(datalayer.battery.status.max_discharge_current_dA, 0);

  set_millis64(6500);
  b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
  b.update_values();
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 1000) << "a fresh 0x311 must restore the limits right away";
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 1000);
}

// The staleness check is unsigned-subtraction based, so it must give the
// right answer across a millis() wrap (2^32 ms), not just look like elapsed
// time went backwards or forwards by four billion milliseconds.
TEST(GrowattLvStaleness, StalenessSurvivesAMillisWrap) {
  datalayer = DataLayer();
  GrowattLvBattery b;

  const uint64_t base = 0x100000000ULL - 500;  // 500ms before the wrap
  set_millis64(base);
  b.handle_incoming_can_frame(Frame311(1000, 1000, true, true));
  b.update_values();
  ASSERT_EQ(datalayer.battery.status.max_charge_current_dA, 1000);

  set_millis64(base + 1500);  // 1s after the wrap; ~1.5s really elapsed - still fresh
  b.update_values();
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 1000)
      << "a millis() wrap must not be mistaken for the limits going stale";

  set_millis64(base + 5600);  // ~5.6s really elapsed - now stale
  b.update_values();
  EXPECT_EQ(datalayer.battery.status.max_charge_current_dA, 0)
      << "elapsed time must still be measured correctly across the wrap";
  EXPECT_EQ(datalayer.battery.status.max_discharge_current_dA, 0);

  set_millis64(0);
}
