#include "FISKER-OCEAN-BATTERY.h"
#include <cstring>  //For unit test
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
void FiskerOceanBattery::update_values() {
  /*
  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV;
  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.info.max_design_voltage_dV;

  datalayer.battery.info.min_design_voltage_dV;

  datalayer.battery.info.number_of_cells;
  */
}

void FiskerOceanBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x355:
    default:
      break;
  }
}

void FiskerOceanBattery::transmit_can(unsigned long currentMillis) {
  // No periodic transmitting for this battery type
}

void FiskerOceanBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
