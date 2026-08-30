#include <gtest/gtest.h>

#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/devboard/utils/types.h"

/* An offgrid system has no grid-tied inverter to lose, so the inverter going
 * missing is not a fault there. It matters beyond the label: the event's
 * ERROR level raises system_status to FAULT, which gates precharge and so
 * blocks a black start outright. */
class OffgridDowngradeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    user_selected_inverter_offgrid = false;
    init_events();
  }
  void TearDown() override { user_selected_inverter_offgrid = false; }
};

TEST_F(OffgridDowngradeTest, InverterMissingIsAnErrorWhenGridTied) {
  set_event(EVENT_CAN_INVERTER_MISSING, 0);
  EXPECT_STREQ(get_event_level_string(EVENT_CAN_INVERTER_MISSING), "ERROR");
  EXPECT_EQ(get_event_level(), EVENT_LEVEL_ERROR);
}

TEST_F(OffgridDowngradeTest, InverterMissingIsAWarningWhenOffgrid) {
  user_selected_inverter_offgrid = true;
  set_event(EVENT_CAN_INVERTER_MISSING, 0);
  EXPECT_STREQ(get_event_level_string(EVENT_CAN_INVERTER_MISSING), "WARNING")
      << "Downgraded before storage, so the events page shows what was recorded";
  EXPECT_EQ(get_event_level(), EVENT_LEVEL_WARNING)
      << "The aggregate must not reach ERROR, or system_status still gates precharge";
}

// The allowlist is the whole point: being offgrid must not soften anything
// that is still a real fault when standalone.
TEST_F(OffgridDowngradeTest, OffgridDoesNotDowngradeUnlistedEvents) {
  user_selected_inverter_offgrid = true;
  set_event(EVENT_BATTERY_OVERHEAT, 0, 1);
  EXPECT_EQ(get_event_level(), EVENT_LEVEL_ERROR);
}
