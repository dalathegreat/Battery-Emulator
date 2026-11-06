#include <gtest/gtest.h>

#include "../Software/src/battery/Battery.h"
#include "../Software/src/inverter/INVERTERS.h"
#include "../Software/src/shunt/Shunt.h"

/* The settings page hides the shunt types the selected battery/inverter can't use, and the save
   handler resets a stored one after a switch. Both go through these predicates. */

TEST(ShuntTypeSupportedTests, CustomClampOnlyWithChademo) {
  EXPECT_TRUE(shunt_type_supported_by_battery(ShuntType::CustomClamp, BatteryType::Chademo));
  EXPECT_FALSE(shunt_type_supported_by_battery(ShuntType::CustomClamp, BatteryType::None));
  EXPECT_FALSE(shunt_type_supported_by_battery(ShuntType::CustomClamp, BatteryType::NissanLeaf));
}

TEST(ShuntTypeSupportedTests, InverterValuesOnlyWithShuntProvidingInverter) {
  EXPECT_TRUE(shunt_type_supported_by_inverter(ShuntType::Inverter, InverterProtocolType::BydCan));
  EXPECT_FALSE(shunt_type_supported_by_inverter(ShuntType::Inverter, InverterProtocolType::None));
  EXPECT_FALSE(shunt_type_supported_by_inverter(ShuntType::Inverter, InverterProtocolType::BydModbus));
}

TEST(ShuntTypeSupportedTests, UnrestrictedTypesAlwaysSupported) {
  for (auto type : {ShuntType::None, ShuntType::BmwSbox}) {
    EXPECT_TRUE(shunt_type_supported_by_battery(type, BatteryType::NissanLeaf));
    EXPECT_TRUE(shunt_type_supported_by_inverter(type, InverterProtocolType::BydModbus));
  }
  // Each restriction only depends on its own selection
  EXPECT_TRUE(shunt_type_supported_by_inverter(ShuntType::CustomClamp, InverterProtocolType::BydModbus));
  EXPECT_TRUE(shunt_type_supported_by_battery(ShuntType::Inverter, BatteryType::NissanLeaf));
}
