#include <gtest/gtest.h>

#include "../Software/src/devboard/display/display_modes.h"

TEST(DisplayModeTests, MissingAndUnknownValuesAreStatus) {
  EXPECT_EQ(display_mode_from_raw(0), DisplayMode::Status);
  EXPECT_EQ(display_mode_from_raw(3), DisplayMode::Status);
  EXPECT_EQ(display_mode_from_raw(99), DisplayMode::Status);
  EXPECT_EQ(display_mode_from_raw(0xFFFFFFFFu), DisplayMode::Status);
}

TEST(DisplayModeTests, KnownValuesRoundTrip) {
  EXPECT_EQ(display_mode_from_raw(1), DisplayMode::Health);
  EXPECT_EQ(display_mode_from_raw(2), DisplayMode::CellBars);
  EXPECT_STREQ(display_mode_name(DisplayMode::Status), "status-pages");
  EXPECT_STREQ(display_mode_name(DisplayMode::Health), "health");
  EXPECT_STREQ(display_mode_name(DisplayMode::CellBars), "cell-bars");
}
