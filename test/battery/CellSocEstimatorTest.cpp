#include <gtest/gtest.h>

#include "../../Software/src/battery/utils/cell_soc_estimator.h"

// Regression coverage for the SOC-from-cell-voltage tables/interpolation shared by
// RELION-LV-BATTERY, KIA-E-GMP-BATTERY and RJXZS-BMS, extracted so the same OCV curves and math
// aren't duplicated per battery. Values below are exact entries lifted from the original
// per-battery tables (RelionBattery's LFP curve, KiaEGmpBattery's NMC curve), so this also pins
// down that the extraction didn't change any battery's SOC output.

TEST(CellSocEstimatorTest, LfpClampsAboveTopOfCurve) {
  EXPECT_EQ(soc_from_cell_voltage(3600, battery_chemistry_enum::LFP), 10000);
  EXPECT_EQ(soc_from_cell_voltage(4000, battery_chemistry_enum::LFP), 10000);
}

TEST(CellSocEstimatorTest, LfpExactTablePointInterpolatesToItsOwnSoc) {
  // lfpVoltageLookup[17] == 3300, SOC[17] == 8300 (83.00%).
  EXPECT_EQ(soc_from_cell_voltage(3300, battery_chemistry_enum::LFP), 8300);
}

TEST(CellSocEstimatorTest, LfpClampsAtBottomOfCurveToOnePercent) {
  // The bottom clamp checks index numPoints-1 (99), not the table's true last entry (100), so
  // the floor this returns is lfpVoltageLookup[99]==2803mV -> SOC[99]==100 (1.00%), not 0.00%.
  // This is inherited unchanged from the original RelionBattery::estimateSOCfromCellvoltage().
  EXPECT_EQ(soc_from_cell_voltage(2803, battery_chemistry_enum::LFP), 100);
  EXPECT_EQ(soc_from_cell_voltage(2000, battery_chemistry_enum::LFP), 100);
}

TEST(CellSocEstimatorTest, NmcClampsAboveTopOfCurve) {
  EXPECT_EQ(soc_from_cell_voltage(4200, battery_chemistry_enum::NMC), 10000);
  EXPECT_EQ(soc_from_cell_voltage(4300, battery_chemistry_enum::NMC), 10000);
}

TEST(CellSocEstimatorTest, NmcExactTablePointInterpolatesToItsOwnSoc) {
  // nmcVoltageLookup[50] == 3650, SOC[50] == 5000 (50.00%).
  EXPECT_EQ(soc_from_cell_voltage(3650, battery_chemistry_enum::NMC), 5000);
}

TEST(CellSocEstimatorTest, NmcClampsAtBottomOfCurveToOnePercent) {
  // Same off-by-one floor as LFP: nmcVoltageLookup[99]==3090mV -> SOC[99]==100 (1.00%).
  EXPECT_EQ(soc_from_cell_voltage(3090, battery_chemistry_enum::NMC), 100);
  EXPECT_EQ(soc_from_cell_voltage(2500, battery_chemistry_enum::NMC), 100);
}

TEST(CellSocEstimatorTest, NcaFallsBackToTheNmcCurve) {
  EXPECT_EQ(soc_from_cell_voltage(3650, battery_chemistry_enum::NCA),
            soc_from_cell_voltage(3650, battery_chemistry_enum::NMC));
}

TEST(CellSocEstimatorTest, AutodetectReportsZeroRatherThanGuessingNmc) {
  // Nothing here resolves Autodetect to a real chemistry, so it must not silently borrow the
  // NMC/NCA curve - a caller that forgot to gate on cell_voltage_range_matches_chemistry() should
  // see an obviously-wrong 0%, not a plausible-looking but unfounded number.
  EXPECT_EQ(soc_from_cell_voltage(3650, battery_chemistry_enum::Autodetect), 0);
  EXPECT_EQ(soc_from_min_max_cell_voltage(3350, 3641, battery_chemistry_enum::Autodetect), 0);
}

TEST(CellSocEstimatorTest, MinMaxTrustsHighestCellAboveFiftyPercent) {
  // max=3650mV -> 5000 (>=5000, upper half), min=3090mV -> 100. Expect the max-cell reading.
  EXPECT_EQ(soc_from_min_max_cell_voltage(3090, 3650, battery_chemistry_enum::NMC), 5000);
}

TEST(CellSocEstimatorTest, MinMaxTrustsLowestCellBelowFiftyPercent) {
  // max=3641mV -> 4900 (<5000, lower half), min=3350mV -> 1500. Expect the min-cell reading.
  EXPECT_EQ(soc_from_min_max_cell_voltage(3350, 3641, battery_chemistry_enum::NMC), 1500);
}

TEST(CellSocEstimatorTest, TableBoundsMatchTheCurvesUsedAbove) {
  EXPECT_EQ(cell_voltage_table_max_mV(battery_chemistry_enum::LFP), 3600);
  EXPECT_EQ(cell_voltage_table_min_mV(battery_chemistry_enum::LFP), 2803);
  EXPECT_EQ(cell_voltage_table_max_mV(battery_chemistry_enum::NMC), 4200);
  EXPECT_EQ(cell_voltage_table_min_mV(battery_chemistry_enum::NMC), 3090);
}

TEST(CellSocEstimatorTest, AutodetectTableBoundsNeverTriggerAnOvervoltageOrUndervoltageGuard) {
  // No real voltage a caller could plausibly pass should trip an ">= max" or "<= min" guard
  // built directly on these two for Autodetect.
  EXPECT_EQ(cell_voltage_table_max_mV(battery_chemistry_enum::Autodetect), UINT16_MAX);
  EXPECT_EQ(cell_voltage_table_min_mV(battery_chemistry_enum::Autodetect), 0);
}

TEST(CellSocEstimatorTest, RangeMatchesWhenDesignVoltagesFitTheSelectedChemistry) {
  EXPECT_TRUE(cell_voltage_range_matches_chemistry(2800, 3650, battery_chemistry_enum::LFP));
  EXPECT_TRUE(cell_voltage_range_matches_chemistry(3000, 4200, battery_chemistry_enum::NMC));
}

TEST(CellSocEstimatorTest, RangeMismatchesWhenLfpCellsAreConfiguredAsNmc) {
  // A real LFP pack's design range (~2.8V-3.65V) configured under the NMC/NCA default chemistry:
  // the min sits well below the NMC curve's floor even with the safety margin, so this must not
  // enter estimated-SOC mode.
  EXPECT_FALSE(cell_voltage_range_matches_chemistry(2800, 3650, battery_chemistry_enum::NMC));
}

TEST(CellSocEstimatorTest, RangeAlwaysMismatchesForAutodetect) {
  // Nothing here actually detects chemistry, so Autodetect must never be treated as plausible,
  // regardless of how well the voltages would otherwise fit the NMC/NCA fallback curve.
  EXPECT_FALSE(cell_voltage_range_matches_chemistry(3000, 4200, battery_chemistry_enum::Autodetect));
}

TEST(CellSocEstimatorTest, RangeMismatchesWhenNmcCellsAreConfiguredAsLfp) {
  // A real NMC pack's design range (~3.0V-4.2V) configured under LFP: the max sits well above the
  // LFP curve's ceiling even with the safety margin.
  EXPECT_FALSE(cell_voltage_range_matches_chemistry(3000, 4200, battery_chemistry_enum::LFP));
}
