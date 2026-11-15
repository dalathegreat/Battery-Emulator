#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"

/* The settings page offers the "Battery chemistry" selector only for the integrations that use it,
   and setup_battery() applies the stored value only to those. Both go through
   battery_supports_chemistry_selection(). */

TEST(BatteryChemistrySelectionTests, IntegrationsWithChemistryDependentLimitsAreListed) {
  for (auto type : {BatteryType::GeelySea, BatteryType::MgGen1, BatteryType::TeslaModel3Y}) {
    EXPECT_TRUE(battery_supports_chemistry_selection(type)) << static_cast<int>(type);
  }
}

TEST(BatteryChemistrySelectionTests, IntegrationsThatCannotKnowTheChemistryAreListed) {
  for (auto type : {BatteryType::DalyBms, BatteryType::Pylon, BatteryType::SimpBms, BatteryType::TestFake}) {
    EXPECT_TRUE(battery_supports_chemistry_selection(type)) << static_cast<int>(type);
  }
}

TEST(BatteryChemistrySelectionTests, FixedChemistryIntegrationsAreNotListed) {
  for (auto type : {BatteryType::None, BatteryType::NissanLeaf, BatteryType::TeslaModelSX, BatteryType::BydAtto3,
                    BatteryType::BYDBatteryBoxPremium, BatteryType::FordMachE}) {
    EXPECT_FALSE(battery_supports_chemistry_selection(type)) << static_cast<int>(type);
  }
}

namespace {

class ChemistrySetupTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_type = user_selected_battery_type;
    saved_chemistry = user_selected_battery_chemistry;
  }

  // Other tests set up TestFake too, which takes the stored chemistry, so hand it back as found.
  void TearDown() override {
    user_selected_battery_type = saved_type;
    user_selected_battery_chemistry = saved_chemistry;
  }

  static void setup_with(BatteryType type, battery_chemistry_enum stored) {
    user_selected_battery_type = type;
    user_selected_battery_chemistry = stored;
    setup_battery();
  }

  BatteryType saved_type;
  battery_chemistry_enum saved_chemistry;
};

}  // namespace

TEST_F(ChemistrySetupTest, StoredChemistryReachesEveryPackOfAnIntegrationThatUsesIt) {
  user_selected_second_battery = true;
  setup_with(BatteryType::TestFake, battery_chemistry_enum::LFP);

  ASSERT_NE(battery2, nullptr);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  EXPECT_EQ(datalayer.battery2.info.chemistry, battery_chemistry_enum::LFP);
}

// The selector is hidden for a fixed-chemistry pack, so a value stored while another integration
// was selected must not make it LFP: the LFP-only inverters would pass its cell voltages through.
TEST_F(ChemistrySetupTest, StoredChemistryDoesNotReachAFixedChemistryPack) {
  setup_with(BatteryType::NissanLeaf, battery_chemistry_enum::LFP);

  EXPECT_NE(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
}

// Hidden as well, yet always LFP: the driver has to say so itself now.
TEST_F(ChemistrySetupTest, BatteryBoxPremiumDeclaresLfpItself) {
  setup_with(BatteryType::BYDBatteryBoxPremium, battery_chemistry_enum::NCA);

  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
}
