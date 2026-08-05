#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TESLA-BATTERY.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"

// The Tesla variant is fixed at construction (TeslaModel3YBattery /
// TeslaModelSXBattery) instead of being read from the
// user_selected_battery_type global inside the driver. These tests construct
// each variant while the global says None, and assert the variant-dependent
// setup (pack design limits, reported protocol name) still resolves correctly.

namespace {

void reset_state() {
  datalayer = DataLayer();
  init_hal();
  // Deliberately not a Tesla type: the driver must not depend on this global
  user_selected_battery_type = BatteryType::None;
}

}  // namespace

TEST(TeslaVariantTest, Model3YSetupAppliesModel3YLimits) {
  reset_state();
  TeslaModel3YBattery battery;
  battery.setup();

  EXPECT_STREQ(datalayer.system.info.battery_protocol, TeslaModel3YBattery::Name);
  // NCA chemistry (the datalayer default) -> 3/Y NCMA pack limits
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4030);
}

TEST(TeslaVariantTest, ModelSXSetupAppliesModelSXLimits) {
  reset_state();
  TeslaModelSXBattery battery;
  battery.setup();

  EXPECT_STREQ(datalayer.system.info.battery_protocol, TeslaModelSXBattery::Name);
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4600);
}

TEST(TeslaVariantTest, FactoryCreatesDistinctVariants) {
  reset_state();
  Battery* model_3y = create_battery(BatteryType::TeslaModel3Y);
  ASSERT_NE(model_3y, nullptr);
  EXPECT_TRUE(model_3y->supports_tesla_dcdc_metrics());
  // Battery's destructor is protected non-virtual: deletion happens through
  // the concrete type the factory is known to have produced
  delete static_cast<TeslaModel3YBattery*>(model_3y);

  reset_state();
  Battery* model_sx = create_battery(BatteryType::TeslaModelSX);
  ASSERT_NE(model_sx, nullptr);
  model_sx->setup();
  EXPECT_STREQ(datalayer.system.info.battery_protocol, TeslaModelSXBattery::Name);
  delete static_cast<TeslaModelSXBattery*>(model_sx);
}
