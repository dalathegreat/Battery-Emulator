#include "TEST-FAKE-BATTERY.h"
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/logging.h"

void TestFakeBattery::
    update_values() { /* This function puts fake values onto the parameters sent towards the inverter */

  datalayer_battery->status.soh_pptt = 9900;  // 99.00%

  // Battery 1's voltage is set by the user via the webserver (set_fake_voltage).
  // Batteries 2 and 3 mirror it so all instances report the same pack voltage.
  if (datalayer_battery != &datalayer.battery) {
    datalayer_battery->status.voltage_dV = datalayer.battery.status.voltage_dV;
  }

  datalayer_battery->status.current_dA = 0;  // 0 A

  datalayer_battery->info.total_capacity_Wh = 30000;  // 30kWh

  // SOC follows the user set pack voltage linearly between the design voltage limits,
  // so that 245.0V reads 0.00% and 404.0V reads 100.00%.
  const uint16_t min_voltage_dV = datalayer_battery->info.min_design_voltage_dV;
  const uint16_t max_voltage_dV = datalayer_battery->info.max_design_voltage_dV;
  const uint16_t voltage_dV = datalayer_battery->status.voltage_dV;

  uint16_t calculated_soc_pptt = 0;
  if (max_voltage_dV > min_voltage_dV) {
    if (voltage_dV <= min_voltage_dV) {
      calculated_soc_pptt = 0;
    } else if (voltage_dV >= max_voltage_dV) {
      calculated_soc_pptt = 10000;
    } else {
      calculated_soc_pptt = static_cast<uint16_t>(static_cast<uint32_t>(voltage_dV - min_voltage_dV) * 10000u /
                                                  static_cast<uint32_t>(max_voltage_dV - min_voltage_dV));
    }
  }
  datalayer_battery->status.real_soc = calculated_soc_pptt;

  // Keep the remaining energy consistent with the calculated SOC
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      static_cast<uint64_t>(datalayer_battery->info.total_capacity_Wh) * calculated_soc_pptt / 10000u);

  datalayer_battery->status.temperature_min_dC = 50;  // 5.0*C

  datalayer_battery->status.temperature_max_dC = 60;  // 6.0*C

  datalayer_battery->status.max_discharge_power_W = 5000;  // 5kW

  datalayer_battery->status.max_charge_power_W = 5000;  // 5kW

  // Cell voltages track the pack voltage: the pack is divided evenly over all cells, with a small
  // random spread on top so the cell monitor has something to show.
  const int32_t cell_nominal_mV = (static_cast<int32_t>(voltage_dV) * 100) / NUMBER_OF_CELLS;

  uint32_t cell_voltage_sum_mV = 0;
  uint16_t cell_lowest_mV = UINT16_MAX;
  uint16_t cell_highest_mV = 0;
  for (int i = 0; i < NUMBER_OF_CELLS; ++i) {
    int32_t cell_mV = cell_nominal_mV + random(-CELL_SPREAD_MV, CELL_SPREAD_MV + 1);
    if (cell_mV < 0) {
      cell_mV = 0;  // A pack voltage of 0V must not wrap the unsigned cell voltage around
    }
    datalayer_battery->status.cell_voltages_mV[i] = static_cast<uint16_t>(cell_mV);
    cell_voltage_sum_mV += static_cast<uint32_t>(cell_mV);
    if (cell_mV < cell_lowest_mV) {
      cell_lowest_mV = static_cast<uint16_t>(cell_mV);
    }
    if (cell_mV > cell_highest_mV) {
      cell_highest_mV = static_cast<uint16_t>(cell_mV);
    }
  }
  datalayer_battery->status.cell_min_voltage_mV = cell_lowest_mV;
  datalayer_battery->status.cell_max_voltage_mV = cell_highest_mV;

  if (calculated_soc_pptt > BALANCING_START_SOC_PPTT) {
    // Near the top of the charge the pack starts balancing: every cell sitting above the pack
    // average gets its resistor switched on
    uint16_t cell_voltage_average_mV = cell_voltage_sum_mV / NUMBER_OF_CELLS;
    for (int i = 0; i < NUMBER_OF_CELLS; ++i) {
      datalayer_battery->status.cell_balancing_status[i] =
          (datalayer_battery->status.cell_voltages_mV[i] > cell_voltage_average_mV + 7);
    }
    datalayer_battery->status.balancing_status = BALANCING_STATUS_ACTIVE;
  } else {
    for (int i = 0; i < NUMBER_OF_CELLS; ++i) {
      datalayer_battery->status.cell_balancing_status[i] = false;
    }
    datalayer_battery->status.balancing_status = BALANCING_STATUS_READY;
  }

  //Fake that we get CAN messages
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void TestFakeBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void TestFakeBattery::transmit_can(unsigned long currentMillis) {
  // Fake battery has no CAN sending
}

void TestFakeBattery::setup(void) {  // Performs one time setup at startup
  randomSeed(analogRead(0));

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer_battery->info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  datalayer_battery->info.min_design_voltage_dV = 2450;  // 245.0V under this, discharging further is disabled
  datalayer_battery->info.number_of_cells = NUMBER_OF_CELLS;

  // The fake cell voltages are derived from the pack voltage, so the cell limits have to agree with
  // the pack design limits. With the datalayer defaults (4300mV/2700mV) a pack driven to its own
  // minimum of 245.0V (2552mV per cell) would raise a cell undervoltage event straight away.
  datalayer_battery->info.max_cell_voltage_mV = 4250;  // 404.0V pack -> 4208mV per cell
  datalayer_battery->info.min_cell_voltage_mV = 2500;  // 245.0V pack -> 2552mV per cell

  // Default fake pack voltage for the primary battery; editable via webserver.
  // Batteries 2 and 3 pick this up through the mirror in update_values().
  if (datalayer_battery == &datalayer.battery && datalayer_battery->status.voltage_dV == 0) {
    datalayer_battery->status.voltage_dV = 3700;  // 370.0V
  }

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
