#include <gtest/gtest.h>
#include "../../Software/src/battery/MG-4-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

class Mg4BatteryTest : public ::testing::Test {
 protected:
  Mg4Battery* battery;

  void SetUp() override {
    battery = new Mg4Battery();
    // Reset datalayer to a known state
    memset(&datalayer, 0, sizeof(datalayer));
    user_selected_battery_chemistry = battery_chemistry_enum::NMC;
    battery->setup();
    // Set some default info since setup sets some but not all
    datalayer.battery.info.total_capacity_Wh = 51000;  // 51kWh pack
    datalayer.battery.info.number_of_cells = 104;
  }

  void TearDown() override { delete battery; }

  void send_can_12c_fd(uint16_t voltage_dV, int16_t current_dA, uint16_t min_mV, uint16_t max_mV) {
    CAN_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.ID = 0x12C;
    frame.FD = true;
    frame.DLC = 64;  // MG4 FD frame is long

    // voltage_dV = (((rx_frame.data.u8[8] << 4) | (rx_frame.data.u8[9] >> 4)) * 5) / 2;
    // inverse: (voltage_dV * 2 / 5) = (u8[8] << 4) | (u8[9] >> 4)
    uint16_t v_raw = (voltage_dV * 2) / 5;
    frame.data.u8[8] = (v_raw >> 4) & 0xFF;
    frame.data.u8[9] = (v_raw & 0x0F) << 4;

    // current_dA = -(((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) - 20000) / 2;
    // inverse: -2 * current_dA + 20000 = (u8[6] << 8) | u8[7]
    uint16_t c_raw = (uint16_t)(-2 * current_dA + 20000);
    frame.data.u8[6] = (c_raw >> 8) & 0xFF;
    frame.data.u8[7] = c_raw & 0xFF;

    // cell_min_voltage_mV = ((rx_frame.data.u8[30] << 8) | (rx_frame.data.u8[31])) / 8;
    uint16_t min_raw = min_mV * 8;
    frame.data.u8[30] = (min_raw >> 8) & 0xFF;
    frame.data.u8[31] = min_raw & 0xFF;

    // cell_max_voltage_mV = ((rx_frame.data.u8[32] << 8) | (rx_frame.data.u8[33])) / 8;
    uint16_t max_raw = max_mV * 8;
    frame.data.u8[32] = (max_raw >> 8) & 0xFF;
    frame.data.u8[33] = max_raw & 0xFF;

    battery->handle_incoming_can_frame(frame);
  }
};

TEST_F(Mg4BatteryTest, CoulombCountAndLimitsTest) {
  // Initial state: 3.7V per cell (approx 50% SoC for NMC)
  uint16_t cell_mV = 3700;
  uint16_t pack_voltage_dV = (cell_mV * 104) / 100;

  // First frame to initialize
  send_can_12c_fd(pack_voltage_dV, 0, cell_mV, cell_mV);
  battery->update_values();

  uint16_t initial_soc = datalayer.battery.status.real_soc;
  // Check SoC is in a believable range
  EXPECT_GE(initial_soc, 4000);
  EXPECT_LE(initial_soc, 6000);

  // Test Discharge Limit Observation
  // MG-4-BATTERY.cpp: get_working_min_cell_voltage_mV() = min_cell_voltage_mV + 300
  // min_cell_voltage_mV for NMC is 2800, so working_min is 3100mV.

  // Simulate hitting working min
  send_can_12c_fd(310 * 104 / 10, 0, 3099, 3100);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 0);
  EXPECT_EQ(datalayer.battery.status.real_soc, 0);

  // Test Charge Limit Observation
  // get_working_max_cell_voltage_mV() = max_cell_voltage_mV - 150
  // max_cell_voltage_mV for NMC is 4300, so working_max is 4150mV.

  // Simulate hitting working max
  send_can_12c_fd(415 * 104 / 10, 0, 4150, 4150);
  battery->update_values();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0);
  EXPECT_EQ(datalayer.battery.status.real_soc, 10000);

  // Test Coulomb Counting
  // Reset to 3.7V
  send_can_12c_fd(pack_voltage_dV, 0, 3700, 3700);
  battery->update_values();
  uint16_t soc_start = datalayer.battery.status.real_soc;

  // Simulate discharge: 100A (1000dA) for some "ticks"
  // update_values is called "every second" in real life.
  // total_discharge_dC -= current_dA; (since discharge is negative current)
  // Actually MG-4 current_dA is calculated as:
  // current_dA = -(((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) - 20000) / 2;
  // If we want 100A discharge, current_dA should be -1000.

  for (int i = 0; i < 2000; i++) {  // 100A discharge
    send_can_12c_fd(pack_voltage_dV, -1000, 3700, 3700);
    battery->update_values();
  }

  uint16_t soc_after_discharge = datalayer.battery.status.real_soc;
  // Should be ~10% now
  EXPECT_LE(soc_after_discharge, 1500);
  EXPECT_GE(soc_after_discharge, 500);

  for (int i = 0; i < 500; i++) {  // 100A discharge
    send_can_12c_fd(pack_voltage_dV, -1000, 3700, 3700);
    battery->update_values();
  }

  soc_after_discharge = datalayer.battery.status.real_soc;
  // Should be floored at 1% (since min cell voltage is still above working min)
  EXPECT_EQ(soc_after_discharge, 100);
}
