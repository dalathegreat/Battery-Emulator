#include "RELION-LV-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
/*CAN Type:CAN2.0(Extended)
BPS:250kbps
Data Length: 8
Data Encoded Format:Motorola*/

void RelionBattery::update_values() {

  datalayer.battery.status.real_soc = battery_soc * 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = battery_soh * 100;

  datalayer.battery.status.voltage_dV = battery_total_voltage;

  datalayer.battery.status.current_dA = battery_total_current;  //Charging negative, discharge positive

  datalayer.battery.status.max_charge_power_W =
      ((battery_total_voltage / 10) * charge_current_A);  //90A recommended charge current

  datalayer.battery.status.max_discharge_power_W =
      ((battery_total_voltage / 10) * discharge_current_A);  //150A max continous discharge current

  datalayer.battery.status.temperature_min_dC = max_cell_temperature * 10;

  datalayer.battery.status.temperature_max_dC = max_cell_temperature * 10;

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;
}

void RelionBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x02018100:  //ID1
      battery_total_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      break;
    case 0x02028100:  //ID2
      battery_total_current = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      system_state = rx_frame.data.u8[2];
      battery_soc = rx_frame.data.u8[3];
      battery_soh = rx_frame.data.u8[4];
      most_serious_fault = rx_frame.data.u8[5];
      break;
    case 0x02038100:  //ID3
      max_cell_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      min_cell_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x02628100:  //Temperatures
      max_cell_temperature = rx_frame.data.u8[0] - 50;
      min_cell_temperature = rx_frame.data.u8[2] - 50;
      break;
    case 0x02648100:  //Charging limitis
      charge_current_A = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) - 800;
      regen_charge_current_A = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 800;
      discharge_current_A = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 800;
      break;
    default:
      break;
  }
}

void RelionBattery::transmit_can(unsigned long currentMillis) {
  // No periodic sending for this protocol
}

void RelionBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.chemistry = LFP;
  datalayer.battery.info.number_of_cells = 16;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
