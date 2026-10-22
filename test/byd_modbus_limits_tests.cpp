#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/battery_aggregate.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/BYD-MODBUS.h"

/* BYD-Modbus hands the Fronius its limits as power, in registers 306 (discharge) and 307 (charge).
   A current limit set over MQTT used to land on the current fields only, so the main page showed
   it as applied while the inverter was never told, and the driver's own cap knew only the manual
   limit. These pin the power registers to whichever limit is in force, with one pack and with two. */
namespace {

class BydModbusLimitsProbe : public BydModbusInverter {
 public:
  using BydModbusInverter::mbPV;
};

class BydModbusLimitsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer and deleted the packs.
    datalayer.battery_settings.soc_scaling_active = false;
    datalayer.system.info.configured_batteries = 1;
    battery2_detected = false;
    battery3_detected = false;
    datalayer.battery.status.voltage_dV = 3700;
    datalayer.battery.status.max_charge_power_W = 10000;
    datalayer.battery.status.max_discharge_power_W = 10000;
  }

  // A second pack on the DC link, talking
  static void add_second_pack(uint32_t charge_W, uint32_t discharge_W) {
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    datalayer.system.info.configured_batteries = 2;
    datalayer.system.status.battery2_allowed_contactor_closing = true;
    battery2_detected = true;
    datalayer.battery2.status.voltage_dV = 3700;
    datalayer.battery2.status.max_charge_power_W = charge_W;
    datalayer.battery2.status.max_discharge_power_W = discharge_W;
  }

  // As an MQTT SET_LIMITS message leaves them
  static void set_remote_limits(uint16_t charge_dA, uint16_t discharge_dA) {
    datalayer.battery_settings.max_remote_set_charge_dA = charge_dA;
    datalayer.battery_settings.remote_settings_limit_charge = true;
    datalayer.battery_settings.max_remote_set_discharge_dA = discharge_dA;
    datalayer.battery_settings.remote_settings_limit_discharge = true;
  }

  // The tail of one core loop cycle, as far as this inverter is concerned
  void run_cycle() {
    update_aggregate_values();
    update_aggregate_limits();
    inverter.update_values();
  }

  BydModbusLimitsProbe inverter;
};

}  // namespace

TEST_F(BydModbusLimitsTest, RemoteLimitReachesThePowerRegisters) {
  set_remote_limits(50, 80);

  run_cycle();

  EXPECT_EQ(inverter.mbPV[307], 1850);  // 5.0 A at 370.0 V, not the 10 kW the BMS allows
  EXPECT_EQ(inverter.mbPV[306], 2960);  // 8.0 A at 370.0 V
}

TEST_F(BydModbusLimitsTest, RemoteLimitReachesThePowerRegistersWithTwoPacks) {
  add_second_pack(8000, 9000);
  set_remote_limits(100, 150);

  run_cycle();

  EXPECT_EQ(inverter.mbPV[307], 3700);  // 10.0 A at 370.0 V
  EXPECT_EQ(inverter.mbPV[306], 5550);  // 15.0 A at 370.0 V
}

// While it lasts, a remote limit replaces the manual one, so it may sit above it. The driver used
// to cap to the manual limit on its own, which held it there.
TEST_F(BydModbusLimitsTest, RemoteLimitAboveTheManualOneIsHonoured) {
  datalayer.battery_settings.max_user_set_charge_dA = 100;
  datalayer.battery_settings.max_user_set_discharge_dA = 100;
  set_remote_limits(200, 200);

  run_cycle();

  EXPECT_EQ(inverter.mbPV[307], 7400);  // 20.0 A at 370.0 V
  EXPECT_EQ(inverter.mbPV[306], 7400);
}

// Without a remote limit the manual one still applies, as it did when the driver applied it itself.
TEST_F(BydModbusLimitsTest, ManualLimitStillApplies) {
  datalayer.battery_settings.max_user_set_charge_dA = 100;
  datalayer.battery_settings.max_user_set_discharge_dA = 150;

  run_cycle();

  EXPECT_EQ(inverter.mbPV[307], 3700);  // 10.0 A at 370.0 V
  EXPECT_EQ(inverter.mbPV[306], 5550);  // 15.0 A at 370.0 V
}
