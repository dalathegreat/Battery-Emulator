#include <gtest/gtest.h>
#include <cstring>

#include "../Software/src/devboard/display/display_modes.h"

namespace {

void expect_row(const char* row, const char* text) {
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(std::strlen(row), 21u);
  ASSERT_EQ(std::strncmp(row, text, std::strlen(text)), 0);
  for (size_t i = std::strlen(text); i < 21; i++) {
    EXPECT_EQ(row[i], ' ') << text;
  }
}

}  // namespace

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

TEST(HealthFormatTests, HalfUpBoundaries) {
  EXPECT_EQ(round_half_up(149, 100), 1u);  // 1.49
  EXPECT_EQ(round_half_up(150, 100), 2u);  // 1.50
  EXPECT_EQ(round_half_up(0, 10), 0u);
}

TEST(HealthFormatTests, MockFrameAndFullPrecisionCac) {
  HealthInputs in{};
  in.soc = true;
  in.soc_pptt = 7200;
  in.capacity = true;
  in.capacity_x10 = 821;
  in.soh = true;
  in.soh_current_x10 = 821;
  in.soh_new_x10 = 1000;
  in.imbalance = true;
  in.imbalance_mV = 18;
  // 178.4-181.6 Ah. Rounded endpoints would be 178 and 182 (delta 4); the real delta is 3.2 Ah.
  in.cac = true;
  in.cac_min_x10 = 1784;
  in.cac_max_x10 = 1816;
  in.cac_avg_x10 = 1800;
  in.socimb = true;
  in.soc_min_x10 = 710;
  in.soc_max_x10 = 720;

  HealthRows rows{};
  format_health(in, rows);
  expect_row(rows.line[0], "SOC 72%");
  expect_row(rows.line[1], "Capacity 82.1kWh");
  expect_row(rows.line[2], "SOH 82% (82.1/100kWh)");
  expect_row(rows.line[3], "Imbalance 18mV");
  expect_row(rows.line[4], "CAC 178 - 182Ah");
  expect_row(rows.line[5], "CACimb 3Ah  2%");
  expect_row(rows.line[6], "SOCimb 1%");
  expect_row(rows.line[7], "");
}

TEST(HealthFormatTests, CacPercentUsesUnroundedDeltaAndAverage) {
  HealthInputs in{};
  in.cac = true;
  in.cac_min_x10 = 1784;
  in.cac_max_x10 = 1816;
  // 3.2 / 213.4 Ah = 1.499% -> 1. Subtracting the rounded endpoints (4 Ah) would show 2%.
  in.cac_avg_x10 = 2134;
  in.socimb = true;
  in.soc_min_x10 = 104;  // 10.4%
  in.soc_max_x10 = 116;  // 11.6%; delta 1.2 -> 1, not 12-10

  HealthRows rows{};
  format_health(in, rows);
  expect_row(rows.line[4], "CAC 178 - 182Ah");
  expect_row(rows.line[5], "CACimb 3Ah  1%");
  expect_row(rows.line[6], "SOCimb 1%");
}

TEST(HealthFormatTests, ValidZeroAndUnavailable) {
  HealthInputs zeros{};
  zeros.soc = true;
  zeros.soc_pptt = 0;
  zeros.imbalance = true;
  zeros.imbalance_mV = 0;
  zeros.socimb = true;
  zeros.soc_min_x10 = 500;
  zeros.soc_max_x10 = 500;

  HealthRows rows{};
  format_health(zeros, rows);
  expect_row(rows.line[0], "SOC 0%");
  expect_row(rows.line[1], "Capacity --");
  expect_row(rows.line[2], "SOH --");
  expect_row(rows.line[3], "Imbalance 0mV");
  expect_row(rows.line[4], "CAC --");
  expect_row(rows.line[5], "CACimb --");
  expect_row(rows.line[6], "SOCimb 0%");

  HealthInputs none{};
  format_health(none, rows);
  expect_row(rows.line[0], "SOC --");
  expect_row(rows.line[3], "Imbalance --");
  expect_row(rows.line[6], "SOCimb --");
}

TEST(HealthFormatTests, RejectsReversedCacAndOverlongSoh) {
  HealthInputs in{};
  in.cac = true;
  in.cac_min_x10 = 1816;
  in.cac_max_x10 = 1784;
  in.cac_avg_x10 = 1800;
  in.soh = true;
  in.soh_current_x10 = 381;  // 38.1 / 999.9 kWh does not fit in 21 characters
  in.soh_new_x10 = 9999;

  HealthRows rows{};
  format_health(in, rows);
  expect_row(rows.line[2], "SOH --");
  expect_row(rows.line[4], "CAC --");
  expect_row(rows.line[5], "CACimb --");
}

TEST(HealthFormatTests, CacPercentUnavailableWhenAverageMissing) {
  HealthInputs in{};
  in.cac = true;
  in.cac_min_x10 = 1780;
  in.cac_max_x10 = 1820;
  in.cac_avg_x10 = 0;

  HealthRows rows{};
  format_health(in, rows);
  expect_row(rows.line[4], "CAC 178 - 182Ah");
  expect_row(rows.line[5], "CACimb 4Ah --%");
}

TEST(HealthFormatTests, SocRoundsHalfUp) {
  HealthInputs in{};
  in.soc = true;
  in.soc_pptt = 7249;
  HealthRows rows{};
  format_health(in, rows);
  expect_row(rows.line[0], "SOC 72%");
  in.soc_pptt = 7250;
  format_health(in, rows);
  expect_row(rows.line[0], "SOC 73%");
  in.soc_pptt = 10000;
  format_health(in, rows);
  expect_row(rows.line[0], "SOC 100%");
}

TEST(HealthFormatTests, HoldKeepsEachFieldForThirtyMinutes) {
  HealthHold hold{};
  HealthInputs live{};
  live.soc = true;
  live.soc_pptt = 7200;
  live.imbalance = true;
  live.imbalance_mV = 0;

  const uint32_t t0 = 1000;
  HealthInputs shown = health_hold_step(hold, live, t0);
  EXPECT_TRUE(shown.soc);
  EXPECT_EQ(shown.soc_pptt, 7200);
  EXPECT_TRUE(shown.imbalance);
  EXPECT_EQ(shown.imbalance_mV, 0u);
  EXPECT_FALSE(shown.cac);

  HealthInputs gone{};
  shown = health_hold_step(hold, gone, t0 + DISPLAY_VALUE_HOLD_MS);
  EXPECT_TRUE(shown.soc);
  EXPECT_EQ(shown.soc_pptt, 7200);
  EXPECT_TRUE(shown.imbalance);

  shown = health_hold_step(hold, gone, t0 + DISPLAY_VALUE_HOLD_MS + 1);
  EXPECT_FALSE(shown.soc);
  EXPECT_FALSE(shown.imbalance);
  EXPECT_FALSE(shown.capacity);
}

TEST(HealthFormatTests, HoldSurvivesMillisWrap) {
  HealthHold hold{};
  HealthInputs live{};
  live.capacity = true;
  live.capacity_x10 = 821;
  const uint32_t t0 = 0xFFFF0000u;
  HealthInputs shown = health_hold_step(hold, live, t0);
  EXPECT_TRUE(shown.capacity);

  HealthInputs gone{};
  const uint32_t at_limit = t0 + DISPLAY_VALUE_HOLD_MS;
  shown = health_hold_step(hold, gone, at_limit);
  EXPECT_TRUE(shown.capacity);
  EXPECT_EQ(shown.capacity_x10, 821);
  shown = health_hold_step(hold, gone, at_limit + 1);
  EXPECT_FALSE(shown.capacity);
}
