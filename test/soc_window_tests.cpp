#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/settings_validation.h"

// Tests for the SOC window validator: the single set of rules enforced by the
// /updateSocMin and /updateSocMax webserver routes and by the boot-time load
// of stored settings in init_stored_settings().

class SocWindowTest : public ::testing::Test {
 protected:
  void SetUp() override { datalayer = DataLayer(); }
};

TEST_F(SocWindowTest, CompiledDefaultWindowIsValid) {
  EXPECT_TRUE(
      validate_soc_window(datalayer.battery.settings.min_percentage, datalayer.battery.settings.max_percentage));
}

TEST_F(SocWindowTest, TypicalPairsAccepted) {
  EXPECT_TRUE(validate_soc_window(2000, 8000));
  EXPECT_TRUE(validate_soc_window(0, 10000));
  // Negative minimum: supported so the inverter never sees a fully empty battery
  EXPECT_TRUE(validate_soc_window(-1000, 10000));
  // Large reserve floor (e.g. backup power use). The old boot guard silently
  // reverted any stored minimum above 50% to the compiled default.
  EXPECT_TRUE(validate_soc_window(6000, 8000));
}

TEST_F(SocWindowTest, MinEqualToMaxRejected) {
  EXPECT_FALSE(validate_soc_window(5000, 5000));
}

TEST_F(SocWindowTest, MinAboveMaxRejected) {
  EXPECT_FALSE(validate_soc_window(8000, 2000));
}

TEST_F(SocWindowTest, MaxAboveCeilingRejected) {
  EXPECT_FALSE(validate_soc_window(2000, 10100));
}

TEST_F(SocWindowTest, MinBelowFloorRejected) {
  EXPECT_FALSE(validate_soc_window(-1100, 8000));
}

TEST_F(SocWindowTest, MaxBelowFloorRejected) {
  // max = 0.00% would collide with the NVS "never stored" sentinel and be
  // silently replaced by the compiled default at boot; the floor makes that
  // combination unrepresentable. 1.00% is the lowest accepted maximum.
  EXPECT_FALSE(validate_soc_window(-1000, 0));
  EXPECT_TRUE(validate_soc_window(-1000, 100));
}

TEST_F(SocWindowTest, GapBoundary) {
  // Exactly the minimum 1% gap is accepted, one pptt less is rejected
  EXPECT_TRUE(validate_soc_window(7900, 8000));
  EXPECT_FALSE(validate_soc_window(7901, 8000));
}

TEST_F(SocWindowTest, SetAppliesValidPair) {
  EXPECT_TRUE(set_soc_window(6000, 9000));
  EXPECT_EQ(datalayer.battery.settings.min_percentage, 6000);
  EXPECT_EQ(datalayer.battery.settings.max_percentage, 9000);
}

TEST_F(SocWindowTest, SetRejectsInvalidPairWithoutModifying) {
  int16_t previous_min = datalayer.battery.settings.min_percentage;
  uint16_t previous_max = datalayer.battery.settings.max_percentage;

  EXPECT_FALSE(set_soc_window(9000, 8000));
  EXPECT_EQ(datalayer.battery.settings.min_percentage, previous_min);
  EXPECT_EQ(datalayer.battery.settings.max_percentage, previous_max);
}
