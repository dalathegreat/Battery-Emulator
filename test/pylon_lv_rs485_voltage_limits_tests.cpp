#include <HardwareSerial.h>
#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/PYLON-LV-RS485.h"

class PylonLvRs485VoltageLimits : public ::testing::Test {
 protected:
  PylonLV485InverterProtocol inverter;

  void SetUp() override {
    datalayer = DataLayer();
    datalayer.aggregate.max_design_voltage_dV = 568;
    datalayer.aggregate.min_design_voltage_dV = 475;
    datalayer.aggregate.max_charge_current_dA = 100;
    datalayer.aggregate.max_discharge_current_dA = 200;
    datalayer.battery_settings.max_user_set_charge_voltage_dV = 558;
    datalayer.battery_settings.max_user_set_discharge_voltage_dV = 480;
    Serial2.rx_buffer.clear();
    Serial2.tx_buffer.clear();
  }

  void TearDown() override {
    Serial2.rx_buffer.clear();
    Serial2.tx_buffer.clear();
  }

  // Check the serialized 0x63 response, including mV conversion and checksum.
  void expect_voltage_limits(unsigned charge_mV, unsigned discharge_mV) {
    Serial2.tx_buffer.clear();
    inverter.update_values();
    Serial2.rx_buffer = "~200246630000FDA9\r";
    inverter.receive();

    const auto& frame = Serial2.tx_buffer;
    ASSERT_EQ(frame.size(), 36u);
    EXPECT_EQ(frame.front(), '~');
    EXPECT_EQ(frame.back(), '\r');
    EXPECT_EQ(frame.substr(9, 4), "D012");  // Nine-byte management payload
    EXPECT_EQ(std::stoul(frame.substr(13, 4), nullptr, 16), charge_mV);
    EXPECT_EQ(std::stoul(frame.substr(17, 4), nullptr, 16), discharge_mV);
    EXPECT_EQ(std::stoul(frame.substr(21, 4), nullptr, 16), 100u);
    EXPECT_EQ(std::stoul(frame.substr(25, 4), nullptr, 16), 200u);

    unsigned sum = 0;
    for (size_t i = 1; i < frame.size() - 5; ++i)
      sum += static_cast<unsigned char>(frame[i]);
    sum += std::stoul(frame.substr(frame.size() - 5, 4), nullptr, 16);
    EXPECT_EQ(sum & 0xFFFFu, 0u);
  }
};

TEST_F(PylonLvRs485VoltageLimits, UsesDesignVoltagesWhenManualLimitsAreDisabled) {
  datalayer.battery_settings.user_set_voltage_limits_active = false;
  expect_voltage_limits(56800, 47500);
}

TEST_F(PylonLvRs485VoltageLimits, SendsManualVoltagesInMillivolts) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  expect_voltage_limits(55800, 48000);
}

TEST_F(PylonLvRs485VoltageLimits, CapsManualChargeVoltageAtDesignMaximum) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 600;
  expect_voltage_limits(56800, 48000);
}

TEST_F(PylonLvRs485VoltageLimits, AcceptsManualChargeVoltageEqualToDesignMaximum) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 568;
  expect_voltage_limits(56800, 48000);
}

TEST_F(PylonLvRs485VoltageLimits, AppliesRuntimeChangesAndRestoresDesignVoltagesWhenDisabled) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  expect_voltage_limits(55800, 48000);
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 552;
  datalayer.battery_settings.max_user_set_discharge_voltage_dV = 485;
  expect_voltage_limits(55200, 48500);
  datalayer.battery_settings.user_set_voltage_limits_active = false;
  expect_voltage_limits(56800, 47500);
}

TEST_F(PylonLvRs485VoltageLimits, FloorsManualDischargeVoltageAtDesignMinimum) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_discharge_voltage_dV = 450;
  expect_voltage_limits(55800, 47500);
}

TEST_F(PylonLvRs485VoltageLimits, AcceptsManualDischargeVoltageEqualToDesignMinimum) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_discharge_voltage_dV = 475;
  expect_voltage_limits(55800, 47500);
}
