#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/calculated_soc.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"

// Covers the reported (inverter-facing) SOC and capacity calculation: the
// window maths, how the per-pack values are derived, how they are summed into
// the system total, and the extreme-pack override.
//
// The inverter sees battery 1's reported_* fields as the whole system, so these
// are the numbers a wrong result would be charged or discharged against.

namespace {

constexpr uint16_t kWindowMinPct = 1000;  // 10.00%
constexpr uint16_t kWindowMaxPct = 9000;  // 90.00%

class CalculatedSocTest : public ::testing::Test {
 protected:
  static void set_pack(DATALAYER_BATTERY_TYPE& pack, uint16_t real_soc, uint32_t total_Wh, uint32_t remaining_Wh) {
    pack.status.real_soc = real_soc;
    pack.info.total_capacity_Wh = total_Wh;
    pack.status.remaining_capacity_Wh = remaining_Wh;
  }

  void SetUp() override {
    datalayer = DataLayer();
    init_hal();

    if (!battery3) {
      // The fake battery is the only integration instantiable three times.
      user_selected_battery_type = BatteryType::TestFake;
      user_selected_second_battery = true;
      user_selected_triple_battery = true;
      setup_battery();
    }
    ASSERT_NE(battery, nullptr);
    ASSERT_NE(battery2, nullptr);
    ASSERT_NE(battery3, nullptr);

    datalayer.battery.settings.soc_scaling_active = true;
    datalayer.battery.settings.min_percentage = kWindowMinPct;
    datalayer.battery.settings.max_percentage = kWindowMaxPct;

    // Three equal 10 kWh packs at 50% unless a test says otherwise.
    set_pack(datalayer.battery, 5000, 10000, 5000);
    set_pack(datalayer.battery2, 5000, 10000, 5000);
    set_pack(datalayer.battery3, 5000, 10000, 5000);

    datalayer.system.status.battery2_allowed_contactor_closing = true;
    datalayer.system.status.battery3_allowed_contactor_closing = true;
  }

  // Removes the extra packs for the single-battery cases, since the production
  // code gates on the pointers.
  struct SinglePackScope {
    Battery* saved2;
    Battery* saved3;
    SinglePackScope() : saved2(battery2), saved3(battery3) {
      battery2 = nullptr;
      battery3 = nullptr;
    }
    ~SinglePackScope() {
      battery2 = saved2;
      battery3 = saved3;
    }
  };
};

}  // namespace

// --- The window maths ------------------------------------------------------

// 50% real inside a 10-90% window is (50-10)/(90-10) = 50% reported.
TEST_F(CalculatedSocTest, ScalesRealSocOntoTheConfiguredWindow) {
  SinglePackScope single;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000);
}

TEST_F(CalculatedSocTest, ReportsEmptyAtAndBelowTheWindowFloor) {
  SinglePackScope single;
  datalayer.battery.status.real_soc = kWindowMinPct;
  update_reported_soc_and_capacity();
  EXPECT_EQ(datalayer.battery.status.reported_soc, 0);

  // Below the floor clamps rather than going negative.
  datalayer.battery.status.real_soc = 500;
  update_reported_soc_and_capacity();
  EXPECT_EQ(datalayer.battery.status.reported_soc, 0);
}

TEST_F(CalculatedSocTest, ReportsFullAtAndAboveTheWindowCeiling) {
  SinglePackScope single;
  datalayer.battery.status.real_soc = kWindowMaxPct;
  update_reported_soc_and_capacity();
  EXPECT_EQ(datalayer.battery.status.reported_soc, 10000);

  datalayer.battery.status.real_soc = 9500;
  update_reported_soc_and_capacity();
  EXPECT_EQ(datalayer.battery.status.reported_soc, 10000);
}

// A zero-width window would divide by zero.
TEST_F(CalculatedSocTest, ZeroWidthWindowReportsZeroRatherThanDividingByZero) {
  SinglePackScope single;
  datalayer.battery.settings.min_percentage = 5000;
  datalayer.battery.settings.max_percentage = 5000;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 0);
}

TEST_F(CalculatedSocTest, ScalingDisabledReportsRealValuesUnchanged) {
  SinglePackScope single;
  datalayer.battery.settings.soc_scaling_active = false;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000);
  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 10000u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 5000u);
}

// Usable capacity is only the part inside the window: 80% of 10 kWh.
TEST_F(CalculatedSocTest, ReportedCapacityCoversOnlyTheWindow) {
  SinglePackScope single;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 8000u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 4000u);
}

// A pack reporting zero SOC takes the fallback branch, which copies the raw
// capacity across instead of scaling it. Pinned because the fallback must not
// hand the inverter a full-size capacity for a pack that is empty - and because
// the guard mixes two conditions that are easy to get wrong separately.
TEST_F(CalculatedSocTest, ZeroRealSocFallsBackToRawCapacityRatherThanScaling) {
  SinglePackScope single;
  set_pack(datalayer.battery, 0, 10000, 0);

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 10000u)
      << "the fallback reports the raw capacity, not the windowed one";
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 0u);
}

// The same fallback fires when a pack has no capacity figure at all.
TEST_F(CalculatedSocTest, UnknownCapacityFallsBackRatherThanScalingToZero) {
  SinglePackScope single;
  set_pack(datalayer.battery, 5000, 0, 4000);

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 0u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 4000u)
      << "the raw remaining capacity is passed through when it cannot be scaled";
}

// --- Multi-pack aggregation ------------------------------------------------

TEST_F(CalculatedSocTest, TwoPacksSumIntoTheSystemTotal) {
  Battery* saved3 = battery3;
  battery3 = nullptr;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 16000u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 8000u);

  battery3 = saved3;
}

// The regression: with scaling active the third pack used to be left out of the
// sum entirely, so a triple-pack system reported two thirds of its capacity.
TEST_F(CalculatedSocTest, ThirdPackIsIncludedInTheSystemTotalWhenScaling) {
  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 24000u)
      << "battery 3's capacity must be part of the system total";
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 12000u);
}

TEST_F(CalculatedSocTest, ThirdPackIsIncludedWithScalingDisabledToo) {
  datalayer.battery.settings.soc_scaling_active = false;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 30000u);
  EXPECT_EQ(datalayer.battery.status.reported_remaining_capacity_Wh, 15000u);
}

// Each pack's own capacity has to drive its own reported figures. Battery 1's
// scaled capacity used to be copied onto the extra packs, so a system with
// differently sized packs reported the wrong totals.
TEST_F(CalculatedSocTest, EachPackIsScaledFromItsOwnCapacity) {
  set_pack(datalayer.battery, 5000, 10000, 5000);
  set_pack(datalayer.battery2, 5000, 20000, 10000);  // twice the size
  set_pack(datalayer.battery3, 5000, 5000, 2500);    // half the size

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery2.info.reported_total_capacity_Wh, 16000u) << "pack 2 must be scaled from its own 20 kWh";
  EXPECT_EQ(datalayer.battery3.info.reported_total_capacity_Wh, 4000u) << "pack 3 must be scaled from its own 5 kWh";
  // 8000 + 16000 + 4000
  EXPECT_EQ(datalayer.battery.info.reported_total_capacity_Wh, 28000u);
}

// --- Per-pack reported SOC -------------------------------------------------

// The #2751 display symptom: the extra packs reported their raw SOC while
// battery 1 reported a scaled one, so the screen mixed the two domains.
TEST_F(CalculatedSocTest, EveryPackReportsScaledSocWhenScalingIsActive) {
  set_pack(datalayer.battery, 5000, 10000, 5000);
  set_pack(datalayer.battery2, 3000, 10000, 3000);
  set_pack(datalayer.battery3, 7000, 10000, 7000);

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000);
  EXPECT_EQ(datalayer.battery2.status.reported_soc, 2500) << "(30-10)/(90-10) = 25%";
  EXPECT_EQ(datalayer.battery3.status.reported_soc, 7500) << "(70-10)/(90-10) = 75%";
}

TEST_F(CalculatedSocTest, EveryPackReportsRealSocWhenScalingIsDisabled) {
  datalayer.battery.settings.soc_scaling_active = false;
  set_pack(datalayer.battery2, 3000, 10000, 3000);
  set_pack(datalayer.battery3, 7000, 10000, 7000);

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery2.status.reported_soc, 3000);
  EXPECT_EQ(datalayer.battery3.status.reported_soc, 7000);
}

// --- The extreme-pack override ---------------------------------------------

// A nearly empty extra pack must pull the system's reported SOC down so the
// inverter stops discharging - but the value written has to stay in the scaled
// domain, since that is the field the inverter reads.
TEST_F(CalculatedSocTest, NearlyEmptyExtraPackOverridesWithAScaledValue) {
  datalayer.battery2.status.real_soc = 50;  // below the 1% extreme threshold

  update_reported_soc_and_capacity();

  // 0.5% real is below the window floor, so it scales to 0 - not the raw 50.
  EXPECT_EQ(datalayer.battery.status.reported_soc, 0)
      << "the override must not write an unscaled value into the reported field";
}

TEST_F(CalculatedSocTest, NearlyFullExtraPackOverridesWithAScaledValue) {
  datalayer.battery2.status.real_soc = 9950;  // above the 99% extreme threshold

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 10000);
}

// With scaling off the override is the raw value, because everything is raw.
TEST_F(CalculatedSocTest, ExtremeOverrideUsesRawValueWhenScalingDisabled) {
  datalayer.battery.settings.soc_scaling_active = false;
  datalayer.battery2.status.real_soc = 50;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 50);
}

// A pack that is not in the mix must not drag the system reading with it.
TEST_F(CalculatedSocTest, ExtremePackIsIgnoredWhileItsContactorsAreOpen) {
  datalayer.system.status.battery2_allowed_contactor_closing = false;
  datalayer.battery2.status.real_soc = 50;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 5000) << "a disconnected pack must not override the system SOC";
}

TEST_F(CalculatedSocTest, ThirdPackAtAnExtremeAlsoOverrides) {
  datalayer.battery3.status.real_soc = 9950;

  update_reported_soc_and_capacity();

  EXPECT_EQ(datalayer.battery.status.reported_soc, 10000);
}
