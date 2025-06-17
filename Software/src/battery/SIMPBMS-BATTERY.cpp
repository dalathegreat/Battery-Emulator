#include "SIMPBMS-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

void SimpBmsBattery::update_values() {

  datalayer.battery.status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = (SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery.status.max_charge_power_W = (max_charge_current * (voltage_dV / 10));

  datalayer.battery.status.max_discharge_power_W = (max_discharge_current * (voltage_dV / 10));

  datalayer.battery.info.total_capacity_Wh = ah_total * (voltage_dV / 10);

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min_mV;

  datalayer.battery.status.temperature_min_dC = celltemperature_min_dC;

  datalayer.battery.status.temperature_max_dC = celltemperature_max_dC;

  datalayer.battery.info.max_design_voltage_dV = charge_cutoff_voltage;

  datalayer.battery.info.min_design_voltage_dV = discharge_cutoff_voltage;

  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mV, SIMPBMS_MAX_CELLS * sizeof(uint16_t));

  datalayer.battery.info.number_of_cells = cells_in_series;
}

void SimpBmsBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x355:
      SOC = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0];
      SOH = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2];

      break;
    case 0x351:
      //discharge and charge limits
      charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      discharge_cutoff_voltage = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);
      max_charge_current = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2])) / 10;
      max_discharge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4])) / 10;
      break;
    case 0x356:
      //current status
      current_dA = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2]) / 1000;
      voltage_dV = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]) / 10;
      break;
    case 0x373:
      //min/max values
      cellvoltage_max_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      cellvoltage_min_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      celltemperature_max_dC = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 273.15) * 10;
      celltemperature_min_dC = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 273.15) * 10;
      break;
    case 0x372:
      //cells_in_series = rx_frame.data.u8[0];
      if (rx_frame.data.u8[3] > 0 && rx_frame.data.u8[3] <= SIMPBMS_MAX_CELLS) {
        uint8_t cellnumber = rx_frame.data.u8[3];
        if (cellnumber > cells_in_series) {
          cells_in_series = cellnumber;
        }
        cellvoltages_mV[cellnumber - 1] = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);
      }

      break;
    case 0x379:
      ah_total = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);

      break;

    default:
      break;
  }
}

void SimpBmsBattery::transmit_can(unsigned long currentMillis) {
  // No periodic transmitting for this battery type
}

void SimpBmsBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = CELL_COUNT;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
