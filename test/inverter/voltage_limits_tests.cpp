#include <gtest/gtest.h>

#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/inverter/BYD-CAN.h"

static constexpr uint16_t VOLTAGE_OFFSET_DV = 20;

class VoltageLimitsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer.battery.info.number_of_cells = 100;
    datalayer.battery.info.max_cell_voltage_mV = 4300;
    datalayer.battery.info.min_cell_voltage_mV = 2700;
    datalayer.battery.info.max_design_voltage_dV = 4300;
    datalayer.battery.info.min_design_voltage_dV = 2700;

    datalayer.battery.status.voltage_dV = 3700;
    datalayer.battery.status.cell_max_voltage_mV = 3700;
    datalayer.battery.status.cell_min_voltage_mV = 3700;
    datalayer.battery.settings.user_set_voltage_limits_active = false;
    datalayer.battery.settings.max_user_set_charge_voltage_dV = 4500;
    datalayer.battery.settings.max_user_set_discharge_voltage_dV = 3000;
  }

  BydCanInverter inverter;
};

// ── Dynamic voltage limits ──────────────────────────────────────────────

TEST_F(VoltageLimitsTest, DynamicLimits_ZeroCells_ReturnsSafeDefaults) {
  datalayer.battery.info.number_of_cells = 0;
  auto limits = inverter.calculate_dynamic_voltage_limits();
  EXPECT_EQ(limits.min_voltage_dV, 0);
  EXPECT_EQ(limits.max_voltage_dV, UINT16_MAX);
}

TEST_F(VoltageLimitsTest, DynamicLimits_BalancedCells_ReturnsDesignLimitsMinusOffsets) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3700;
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  uint16_t expected_max = ((4300 - 100) * 100) / 100 - 8;
  uint16_t expected_min = ((2700 + 100) * 100) / 100 + 1;
  EXPECT_EQ(limits.max_voltage_dV, expected_max);
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, DynamicLimits_HighCellDeviation_ReducesMaxLimit) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3800;
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // Should be within the original bounds
  EXPECT_LT(limits.max_voltage_dV, datalayer.battery.info.max_design_voltage_dV);
  EXPECT_GT(limits.max_voltage_dV, datalayer.battery.info.min_design_voltage_dV);

  EXPECT_LT(limits.min_voltage_dV, limits.max_voltage_dV);

  int32_t mean_mV = 3700 * 100 / 100;
  int32_t deviation_mV = 3800 - mean_mV;
  int32_t expected_max = ((4300 - 100) * 100) / 100 - (deviation_mV * 100) / 100 - 8;

  EXPECT_EQ(limits.max_voltage_dV, expected_max);
}

TEST_F(VoltageLimitsTest, DynamicLimits_LowCellDeviation_IncreasesMinLimit) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3700;
  datalayer.battery.status.cell_min_voltage_mV = 3600;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // Should be within the original bounds
  EXPECT_LT(limits.min_voltage_dV, datalayer.battery.info.max_design_voltage_dV);
  EXPECT_GT(limits.min_voltage_dV, datalayer.battery.info.min_design_voltage_dV);

  EXPECT_LT(limits.min_voltage_dV, limits.max_voltage_dV);

  int32_t mean_mV = 3700 * 100 / 100;
  int32_t deviation_mV = mean_mV - 3600;
  int32_t expected_min = ((2700 + 100) * 100) / 100 + (deviation_mV * 100) / 100 + 1;
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, DynamicLimits_BothDeviations_NarrowsBothLimits) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3850;
  datalayer.battery.status.cell_min_voltage_mV = 3550;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // Should be within the original bounds
  EXPECT_LT(limits.min_voltage_dV, datalayer.battery.info.max_design_voltage_dV);
  EXPECT_GT(limits.min_voltage_dV, datalayer.battery.info.min_design_voltage_dV);
  EXPECT_LT(limits.max_voltage_dV, datalayer.battery.info.max_design_voltage_dV);
  EXPECT_GT(limits.max_voltage_dV, datalayer.battery.info.min_design_voltage_dV);

  EXPECT_LT(limits.min_voltage_dV, limits.max_voltage_dV);

  int32_t mean_mV = 3700 * 100 / 100;
  int32_t dev_max_mV = 3850 - mean_mV;
  int32_t dev_min_mV = mean_mV - 3550;
  uint16_t expected_max = ((4300 - 100) * 100) / 100 - (dev_max_mV * 100) / 100 - 8;
  uint16_t expected_min = ((2700 + 100) * 100) / 100 + (dev_min_mV * 100) / 100 + 1;

  EXPECT_LE(limits.max_voltage_dV, expected_max);
  EXPECT_GE(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, DynamicLimits_NegativeDeviationMaxDoesNotIncreaseLimit) {
  datalayer.battery.status.cell_max_voltage_mV = 3600;
  datalayer.battery.status.voltage_dV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  uint16_t expected_max = ((4300 - 100) * 100) / 100 - 8;
  EXPECT_EQ(limits.max_voltage_dV, expected_max);
}

TEST_F(VoltageLimitsTest, DynamicLimits_NegativeDeviationMinDoesNotDecreaseLimit) {
  datalayer.battery.status.cell_min_voltage_mV = 3750;
  datalayer.battery.status.voltage_dV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  uint16_t expected_min = ((2700 + 100) * 100) / 100 + 1;
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, DynamicLimits_MaxLimitClampedToZero) {
  datalayer.battery.info.number_of_cells = 1;
  datalayer.battery.info.max_cell_voltage_mV = 1;
  datalayer.battery.status.voltage_dV = 0;
  datalayer.battery.status.cell_max_voltage_mV = 5000;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  EXPECT_EQ(limits.max_voltage_dV, 0);
}

TEST_F(VoltageLimitsTest, DynamicLimits_MaxLimitClampedToUINT16_MAX) {
  datalayer.battery.info.max_cell_voltage_mV = UINT16_MAX;
  datalayer.battery.info.number_of_cells = 200;
  datalayer.battery.status.voltage_dV = UINT16_MAX;
  datalayer.battery.status.cell_max_voltage_mV = 0;
  datalayer.battery.status.cell_min_voltage_mV = 0;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  EXPECT_EQ(limits.max_voltage_dV, UINT16_MAX);
}

TEST_F(VoltageLimitsTest, DynamicLimits_MinLimitClampedToZero) {
  datalayer.battery.info.number_of_cells = 1;
  datalayer.battery.info.min_cell_voltage_mV = 0;
  datalayer.battery.status.voltage_dV = 0;
  datalayer.battery.status.cell_min_voltage_mV = 0;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // min = (0+100)*1/100 + 1 = 1 + 1 = 2
  EXPECT_EQ(limits.min_voltage_dV, 2);
}

TEST_F(VoltageLimitsTest, DynamicLimits_MinLimitClampedToUINT16_MAX) {
  datalayer.battery.info.number_of_cells = 1;
  datalayer.battery.info.min_cell_voltage_mV = UINT16_MAX;
  datalayer.battery.status.voltage_dV = 0;
  datalayer.battery.status.cell_min_voltage_mV = 0;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // min = (65535+100)*1/100 + 1 = 656 + 1 = 657
  // Not clamped to UINT16_MAX because calculation doesn't overflow
  EXPECT_EQ(limits.min_voltage_dV, 657);
}

TEST_F(VoltageLimitsTest, DynamicLimits_SingleCellPack) {
  datalayer.battery.info.number_of_cells = 1;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3700;
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // With 1 cell: max = (4300-100)*1/100 - 8 = 42 - 8 = 34
  // min = (2700+100)*1/100 + 1 = 28 + 1 = 29
  // But deviation increases min: mean=370000, dev_min=370000-3700=366300, increase=366300*1/100=3663
  // min = 28 + 3663 + 1 = 3692
  EXPECT_EQ(limits.max_voltage_dV, 34);
  EXPECT_EQ(limits.min_voltage_dV, 3692);
}

TEST_F(VoltageLimitsTest, DynamicLimits_SingleCellWithDeviation) {
  datalayer.battery.info.number_of_cells = 1;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3800;
  datalayer.battery.status.cell_min_voltage_mV = 3600;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // mean = 3700*100/1 = 370000mV
  // dev_max = 3800-370000 = -366200 (negative, no reduction)
  // dev_min = 370000-3600 = 366400, increase = 366400*1/100 = 3664
  // max = (4300-100)*1/100 - 8 = 42 - 8 = 34
  // min = (2700+100)*1/100 + 3664 + 1 = 28 + 3664 + 1 = 3693
  EXPECT_EQ(limits.max_voltage_dV, 34);
  EXPECT_EQ(limits.min_voltage_dV, 3693);
}

TEST_F(VoltageLimitsTest, DynamicLimits_PackVoltageInconsistentWithCellVoltages) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 4200;           // Pack says 420V
  datalayer.battery.status.cell_max_voltage_mV = 3700;  // Cells say 3.7V
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  // mean = 4200*100/100 = 4200mV (from pack voltage)
  // dev_max = 3700-4200 = -500 (negative, no reduction)
  // dev_min = 4200-3700 = 500, increase = 500*100/100 = 500
  // max = 4200*100/100 - 8 = 4192
  // min = 2800*100/100 + 500 + 1 = 3301
  EXPECT_EQ(limits.max_voltage_dV, 4192);
  EXPECT_EQ(limits.min_voltage_dV, 3301);
}

// ── Top-level voltage limits ────────────────────────────────────────────

TEST_F(VoltageLimitsTest, TopLevel_NominalCase_ReturnsDesignLimitsWithOffset) {
  auto limits = inverter.calculate_voltage_limits();

  // Dynamic limits are narrower than static limits, so they override
  EXPECT_EQ(limits.max_voltage_dV, 4192);
  EXPECT_EQ(limits.min_voltage_dV, 2801);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsActive_ChargeLimitApplied) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 4000;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 3000;

  auto limits = inverter.calculate_voltage_limits();

  EXPECT_EQ(limits.max_voltage_dV, 4000);
  EXPECT_EQ(limits.min_voltage_dV, 3000);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsActive_ChargeLimitZero_Ignored) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 0;

  auto limits = inverter.calculate_voltage_limits();

  // User limit ignored, dynamic limits override static limits
  EXPECT_EQ(limits.max_voltage_dV, 4192);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsActive_ChargeLimitAboveDesign_Ignored) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 5500;

  auto limits = inverter.calculate_voltage_limits();

  // User limit ignored (above design), dynamic limits override static limits
  EXPECT_EQ(limits.max_voltage_dV, 4192);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsActive_DischargeLimitZero_Ignored) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 0;

  auto limits = inverter.calculate_voltage_limits();

  // User limit ignored, dynamic limits override static limits
  EXPECT_EQ(limits.min_voltage_dV, 2801);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsActive_DischargeLimitBelowDesign_Ignored) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 1000;

  auto limits = inverter.calculate_voltage_limits();

  // User limit ignored (below design), dynamic limits override static limits
  EXPECT_EQ(limits.min_voltage_dV, 2801);
}

TEST_F(VoltageLimitsTest, TopLevel_UserLimitsNarrowerThanDynamic_Wins) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = 4000;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 3000;

  auto limits = inverter.calculate_voltage_limits();

  // User limits (4000/3000) are narrower than dynamic limits (4192/2801)
  // User limits should win
  EXPECT_EQ(limits.max_voltage_dV, 4000);
  EXPECT_EQ(limits.min_voltage_dV, 3000);
}

TEST_F(VoltageLimitsTest, TopLevel_DynamicLimitsNarrowMax) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 4300;
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_voltage_limits();

  int32_t mean_mV = 3700 * 100 / 100;
  int32_t dev_max_mV = 4300 - mean_mV;
  uint16_t dynamic_max = ((4300 - 100) * 100) / 100 - (dev_max_mV * 100) / 100 - 8;
  EXPECT_EQ(limits.max_voltage_dV, dynamic_max);
  EXPECT_LT(limits.max_voltage_dV, 5000 - VOLTAGE_OFFSET_DV);
}

TEST_F(VoltageLimitsTest, TopLevel_DynamicLimitsNarrowMin) {
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3700;
  datalayer.battery.status.cell_max_voltage_mV = 3700;
  datalayer.battery.status.cell_min_voltage_mV = 2700;

  auto limits = inverter.calculate_voltage_limits();

  int32_t mean_mV = 3700 * 100 / 100;
  int32_t dev_min_mV = mean_mV - 2700;
  uint16_t dynamic_min = ((2700 + 100) * 100) / 100 + (dev_min_mV * 100) / 100 + 1;
  EXPECT_EQ(limits.min_voltage_dV, dynamic_min);
  EXPECT_GT(limits.min_voltage_dV, 2500 + VOLTAGE_OFFSET_DV);
}

TEST_F(VoltageLimitsTest, TopLevel_DynamicLimitsWider_DoesNotBroaden) {
  datalayer.battery.info.max_cell_voltage_mV = 5000;
  datalayer.battery.info.min_cell_voltage_mV = 2000;

  auto limits = inverter.calculate_voltage_limits();

  EXPECT_EQ(limits.max_voltage_dV, 4300 - VOLTAGE_OFFSET_DV);
  EXPECT_EQ(limits.min_voltage_dV, 2700 + VOLTAGE_OFFSET_DV);
}

// ── Safety corner cases ─────────────────────────────────────────────────

TEST_F(VoltageLimitsTest, CornerCase_ZeroVoltageWithValidCells) {
  datalayer.battery.status.voltage_dV = 0;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  int32_t mean_mV = 0;
  int32_t dev_max_mV = 3700 - mean_mV;
  int32_t dev_min_mV = mean_mV - 3700;
  uint16_t expected_max = ((4300 - 100) * 100) / 100 - (dev_max_mV * 100) / 100 - 8;
  uint16_t expected_min = ((2700 + 100) * 100) / 100 + 1;

  EXPECT_EQ(limits.max_voltage_dV, expected_max);
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, CornerCase_MaxCells) {
  datalayer.battery.info.number_of_cells = MAX_AMOUNT_CELLS;
  datalayer.battery.info.max_cell_voltage_mV = 4300;
  datalayer.battery.info.min_cell_voltage_mV = 2700;
  datalayer.battery.info.max_design_voltage_dV = MAX_AMOUNT_CELLS * 4300 / 100;
  datalayer.battery.info.min_design_voltage_dV = MAX_AMOUNT_CELLS * 2700 / 100;

  datalayer.battery.status.voltage_dV = 37 * MAX_AMOUNT_CELLS;
  datalayer.battery.status.cell_max_voltage_mV = 3700;
  datalayer.battery.status.cell_min_voltage_mV = 3700;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  uint16_t expected_max = ((4300 - 100) * MAX_AMOUNT_CELLS) / 100 - 8;
  uint16_t expected_min = ((2700 + 100) * MAX_AMOUNT_CELLS) / 100 + 1;
  EXPECT_EQ(limits.max_voltage_dV, expected_max);
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, CornerCase_CellVoltagesAtDesignLimits) {
  datalayer.battery.status.cell_max_voltage_mV = 4300;
  datalayer.battery.status.cell_min_voltage_mV = 2700;
  datalayer.battery.status.voltage_dV = 3500;

  auto limits = inverter.calculate_dynamic_voltage_limits();

  int32_t mean_mV = 3500 * 100 / 100;
  int32_t dev_max_mV = 4300 - mean_mV;
  int32_t dev_min_mV = mean_mV - 2700;
  uint16_t expected_max = ((4300 - 100) * 100) / 100 - (dev_max_mV * 100) / 100 - 8;
  uint16_t expected_min = ((2700 + 100) * 100) / 100 + (dev_min_mV * 100) / 100 + 1;

  EXPECT_EQ(limits.max_voltage_dV, expected_max);
  EXPECT_EQ(limits.min_voltage_dV, expected_min);
}

TEST_F(VoltageLimitsTest, CornerCase_UserLimitsAtExtremes) {
  datalayer.battery.settings.user_set_voltage_limits_active = true;
  datalayer.battery.settings.max_user_set_charge_voltage_dV = UINT16_MAX;
  datalayer.battery.settings.max_user_set_discharge_voltage_dV = 0;

  auto limits = inverter.calculate_voltage_limits();

  // User limits are ignored when outside valid range, dynamic limits override
  EXPECT_EQ(limits.max_voltage_dV, 4192);
  EXPECT_EQ(limits.min_voltage_dV, 2801);
}

TEST_F(VoltageLimitsTest, CornerCase_DesignLimitsEqual_VoltageAtDesign) {
  datalayer.battery.info.max_design_voltage_dV = 3700;
  datalayer.battery.info.min_design_voltage_dV = 3700;
  datalayer.battery.status.voltage_dV = 3700;

  auto limits = inverter.calculate_voltage_limits();

  EXPECT_EQ(limits.max_voltage_dV, 3700 - VOLTAGE_OFFSET_DV);
  EXPECT_EQ(limits.min_voltage_dV, 3700 + VOLTAGE_OFFSET_DV);
}
