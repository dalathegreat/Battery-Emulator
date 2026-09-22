#include <gtest/gtest.h>

#include <string>

#include "../../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

namespace {

class TestFakeBatteryTest : public ::testing::Test {
 protected:
  // The datalayer is a global shared by every test, so start each one from its power-on state
  void SetUp() override {
    datalayer = DataLayer();
    pack2.battery_index = 2;
    pack1.setup();
    pack2.setup();
  }

  TestFakeBattery pack1;
  TestFakeBattery pack2{&datalayer.battery2, CAN_NATIVE};
};

// Test: Voltage and SOH set on one pack stay on that pack, and its SOC follows its own voltage
TEST_F(TestFakeBatteryTest, EachPackKeepsItsOwnVoltageAndSoh) {
  pack2.set_fake_voltage(380.0f);
  pack2.set_fake_soh(80.0f);

  pack1.update_values();
  pack2.update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3700);
  EXPECT_EQ(datalayer.battery2.status.voltage_dV, 3800);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_EQ(datalayer.battery2.status.soh_pptt, 8000);
  EXPECT_GT(datalayer.battery2.status.real_soc, datalayer.battery.status.real_soc);
}

// Test: Entered values are rounded to the nearest step instead of truncated (370.3 V is not 370.2 V)
TEST_F(TestFakeBatteryTest, SettersRoundToTheNearestStep) {
  pack1.set_fake_voltage(370.3f);
  pack1.set_fake_soh(87.35f);

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8735);
}

// Test: The page of battery 2 shows and edits battery 2's own values
TEST_F(TestFakeBatteryTest, PageEditsItsOwnPack) {
  pack2.set_fake_voltage(355.5f);
  pack2.set_fake_soh(91.5f);
  pack2.update_values();

  BatteryHtmlRenderer& renderer = static_cast<Battery&>(pack2).get_status_renderer();
  const std::string html = renderer.get_status_html().c_str();

  EXPECT_TRUE(renderer.renders_own_battery_data());
  EXPECT_NE(html.find("Voltage: 355.5 V"), std::string::npos);
  EXPECT_NE(html.find("SOH: 91.50%"), std::string::npos);
  EXPECT_NE(html.find("editFake(2,'Voltage',5000)"), std::string::npos);
  EXPECT_NE(html.find("editFake(2,'SOH',100)"), std::string::npos);
  EXPECT_NE(html.find("</script>"), std::string::npos);
}

}  // namespace
