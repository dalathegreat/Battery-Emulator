#include "MG-4-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void Mg4Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second

  uint32_t max_power_W = 10000;

  // Taper charge/discharge power at high/low SoC
  if (datalayer.battery.status.real_soc > 9000) {
    datalayer.battery.status.max_charge_power_W =
        max_power_W * (1.0f - ((datalayer.battery.status.real_soc - 9000) / 1000.0f));
  } else {
    datalayer.battery.status.max_charge_power_W = max_power_W;
  }
  if (datalayer.battery.status.real_soc < 1000) {
    datalayer.battery.status.max_discharge_power_W = max_power_W * (datalayer.battery.status.real_soc / 1000.0f);
  } else {
    datalayer.battery.status.max_discharge_power_W = max_power_W;
  }
}

void Mg4Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  uint32_t soc_times_ten;

  switch (rx_frame.ID) {
    case 0x12C:
      datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 2.5f;
      datalayer.battery.status.current_dA = -(((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 20000) * 0.5f;

      break;
    case 0x401:
      soc_times_ten = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) & 0x3FF;
      datalayer.battery.status.real_soc = soc_times_ten * 10;

      break;
    default:
      break;
  }
}
void Mg4Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    return;
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&MG4_4F3);
    transmit_can_frame(&MG4_047_E9);
    transmit_can_frame(&MG4_047_3B);
  }
}

void Mg4Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;

  datalayer.battery.info.number_of_cells = 104;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Danger limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_cell_voltage_mV = 3650;
    datalayer.battery.info.min_cell_voltage_mV = 2500;
  } else {
    datalayer.battery.info.max_cell_voltage_mV = 4300;
    datalayer.battery.info.min_cell_voltage_mV = 2800;
  }

  datalayer.battery.info.max_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.max_cell_voltage_mV) / 100;
  datalayer.battery.info.min_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.min_cell_voltage_mV) / 100;
}
