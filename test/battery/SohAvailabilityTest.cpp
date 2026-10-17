#include <gtest/gtest.h>

#include "../../Software/src/battery/BOLT-AMPERA-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

// The web UI, MQTT and ESP-NOW all hold the state of health back while soh_available is false.
// Only an integration that cannot report one yet clears the flag; every other one inherits the
// default and publishes as it always has. Guarded here because flipping that default would blank
// the SOH on every integration at once, in all three places, without any of them being touched.

TEST(SohAvailabilityTests, ShouldBeAvailableByDefault) {
  datalayer = DataLayer();

  EXPECT_TRUE(datalayer.battery.status.soh_available);
  EXPECT_TRUE(datalayer.battery2.status.soh_available);
  EXPECT_TRUE(datalayer.battery3.status.soh_available);
}

// Bolt/Ampera publishes a fixed 99%, which is exactly the case the flag must not interfere with:
// the figure is deliberate rather than a placeholder waiting to be replaced.
TEST(SohAvailabilityTests, ShouldStayAvailableForADriverThatNeverSetsIt) {
  datalayer = DataLayer();

  BoltAmperaBattery battery;
  battery.setup();
  battery.update_values();

  EXPECT_TRUE(datalayer.battery.status.soh_available);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900u);
}
