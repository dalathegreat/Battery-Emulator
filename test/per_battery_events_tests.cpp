#include <gtest/gtest.h>

#include <string>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"

/* The events a battery driver raises are now per-pack triplets, because driver code is
   per-instance: the same line runs for pack 1, 2 and 3. These tests pin the two things that
   silently break if the enum is edited carelessly - the 1,2,3 layout the resolver depends on,
   and the independence of the three variants. */

namespace {

// Every base event that a battery driver can raise. Pack 1 keeps the original name.
const EVENTS_ENUM_TYPE kDriverEventBases[] = {
    EVENT_BALANCING_START,
    EVENT_BALANCING_END,
    EVENT_12V_LOW,
    EVENT_HVIL_FAILURE,
    EVENT_CONTACTOR_WELDED,
    EVENT_CONTACTOR_OPEN,
    EVENT_WATER_INGRESS,
    EVENT_THERMAL_RUNAWAY,
    EVENT_INTERNAL_OPEN_FAULT,
    EVENT_STALE_VALUE,
    EVENT_SOC_UNAVAILABLE,
    EVENT_KWH_PLAUSIBILITY_ERROR,
    EVENT_PID_FAILED,
    EVENT_RJXZS_LOG,
    EVENT_BMS_RESET_REQ_SUCCESS,
    EVENT_BMS_RESET_REQ_FAIL,
    EVENT_BYD_AUTO_SOC_CALIBRATION,
    EVENT_BYD_CHARGE_TERMINATED,
    EVENT_BYD_CONTACTOR_MISMATCH,
    EVENT_BYD_CONTACTOR_FORCE_OPEN,
    EVENT_BYD_CONTACTOR_OPEN_REQ,
    EVENT_BYD_CONTACTOR_CLOSE_REQ,
    EVENT_BYD_CONTACTOR_CLOSE_BLOCKED,
};

class PerBatteryEventsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Explicit, and restored in TearDown: the suite is also run shuffled in one process, so a
    // test that inherited this from another one would pass or fail depending on order.
    saved_pack_count = datalayer.system.info.configured_batteries;
    datalayer.system.info.configured_batteries = 3;
    init_events();
    reset_all_events();
  }

  void TearDown() override { datalayer.system.info.configured_batteries = saved_pack_count; }

  uint8_t saved_pack_count = 1;

  static EVENTS_STATE_TYPE state_of(int event) {
    return get_event_pointer(static_cast<EVENTS_ENUM_TYPE>(event))->state;
  }
};

// Raising an event for one pack must not touch the other two.
TEST_F(PerBatteryEventsTest, EachPackResolvesToItsOwnVariant) {
  for (EVENTS_ENUM_TYPE base : kDriverEventBases) {
    for (uint8_t pack = 1; pack <= 3; pack++) {
      reset_all_events();
      set_event(base, 42, pack);
      for (uint8_t other = 1; other <= 3; other++) {
        const int variant = base + (other - 1);
        if (other == pack) {
          EXPECT_EQ(state_of(variant), EVENT_STATE_ACTIVE)
              << get_event_enum_string(static_cast<EVENTS_ENUM_TYPE>(variant)) << " should be active";
          EXPECT_EQ(get_event_pointer(static_cast<EVENTS_ENUM_TYPE>(variant))->data, 42);
        } else {
          EXPECT_EQ(state_of(variant), EVENT_STATE_INACTIVE)
              << get_event_enum_string(static_cast<EVENTS_ENUM_TYPE>(variant)) << " should be untouched";
        }
      }
    }
  }
}

/* The regression this whole change exists to prevent: before it, pack 2 and pack 3 shared one
   event with pack 1, so a healthy pack cleared a warning another pack had just raised. */
TEST_F(PerBatteryEventsTest, ClearingOnePackLeavesTheOthersActive) {
  for (EVENTS_ENUM_TYPE base : kDriverEventBases) {
    reset_all_events();
    set_event(base, 0, 1);
    set_event(base, 0, 3);
    clear_event(base, 3);
    EXPECT_EQ(state_of(base), EVENT_STATE_ACTIVE) << get_event_enum_string(base) << " for pack 1 was cleared by pack 3";
    EXPECT_EQ(state_of(base + 2), EVENT_STATE_INACTIVE);
  }
}

/* A single battery install has nothing to disambiguate, so pack 1 messages carry no suffix
   there. Packs 2 and 3 are unaffected - their events cannot fire without a second pack. */
TEST_F(PerBatteryEventsTest, SinglePackInstallIsNotSuffixed) {
  datalayer.system.info.configured_batteries = 1;
  for (EVENTS_ENUM_TYPE base : kDriverEventBases) {
    const std::string msg = get_event_message_string(base).c_str();
    EXPECT_EQ(msg.find("(Battery"), std::string::npos) << get_event_enum_string(base) << ": " << msg;
    EXPECT_FALSE(msg.empty()) << get_event_enum_string(base) << " has no message text";
  }
  // Two packs configured: pack 1 is named again.
  datalayer.system.info.configured_batteries = 2;
  const std::string msg = get_event_message_string(EVENT_12V_LOW).c_str();
  EXPECT_NE(msg.find("(Battery 1)"), std::string::npos) << msg;
}

// The three variants share one message string; only the pack suffix differs.
TEST_F(PerBatteryEventsTest, MessageNamesThePack) {
  for (EVENTS_ENUM_TYPE base : kDriverEventBases) {
    // The emul String stub has no find()/substr(), so compare as std::string.
    const std::string pack1 = get_event_message_string(base).c_str();
    const std::string pack2 = get_event_message_string(static_cast<EVENTS_ENUM_TYPE>(base + 1)).c_str();
    const std::string pack3 = get_event_message_string(static_cast<EVENTS_ENUM_TYPE>(base + 2)).c_str();
    EXPECT_NE(pack1.find("(Battery 1)"), std::string::npos) << get_event_enum_string(base);
    EXPECT_NE(pack2.find("(Battery 2)"), std::string::npos) << get_event_enum_string(base);
    EXPECT_NE(pack3.find("(Battery 3)"), std::string::npos) << get_event_enum_string(base);
    // Same underlying text, so a driver event never renders as an empty message.
    EXPECT_EQ(pack1.substr(0, pack1.find(" (Battery")), pack2.substr(0, pack2.find(" (Battery")));
    EXPECT_GT(pack1.find(" (Battery"), 0u) << get_event_enum_string(base) << " has no message text";
  }
}

/* The CAN detected/missing events are in the block too, so they take the suffix rather than
   carrying hand written "2nd battery" text. Their levels deliberately differ across the
   triplet, which the block allows - only the 1,2,3 position is fixed. */
TEST_F(PerBatteryEventsTest, CanAliveEventsShareOneMessageAndKeepTheirLevels) {
  for (EVENTS_ENUM_TYPE base : {EVENT_CAN_BATTERY_DETECTED, EVENT_CAN_BATTERY_MISSING}) {
    const std::string pack1 = get_event_message_string(base).c_str();
    const std::string pack2 = get_event_message_string(static_cast<EVENTS_ENUM_TYPE>(base + 1)).c_str();
    EXPECT_EQ(pack1.substr(0, pack1.find(" (Battery")), pack2.substr(0, pack2.find(" (Battery")));
    EXPECT_NE(pack2.find("(Battery 2)"), std::string::npos);
  }
  EXPECT_STREQ(get_event_level_string(EVENT_CAN_BATTERY_MISSING), "ERROR");
  EXPECT_STREQ(get_event_level_string(EVENT_CAN_BATTERY2_MISSING), "WARNING");
  EXPECT_STREQ(get_event_level_string(EVENT_CAN_BATTERY3_MISSING), "WARNING");
}

// An out of range pack number must not land on an unrelated event.
TEST_F(PerBatteryEventsTest, InvalidPackNumberIsRejected) {
  set_event(EVENT_12V_LOW, 0, 0);
  set_event(EVENT_12V_LOW, 0, 4);
  EXPECT_EQ(state_of(EVENT_12V_LOW), EVENT_STATE_INACTIVE);
  EXPECT_EQ(state_of(EVENT_12V_LOW_BAT2), EVENT_STATE_INACTIVE);
  EXPECT_EQ(state_of(EVENT_12V_LOW_BAT3), EVENT_STATE_INACTIVE);
}

}  // namespace
