#include "RENAULT-TWIZY.h"
#include <cstdint>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

int16_t max_value(int16_t* entries, size_t len) {
  int result = INT16_MIN;
  for (int i = 0; i < len; i++) {
    if (entries[i] > result)
      result = entries[i];
  }

  return result;
}

int16_t min_value(int16_t* entries, size_t len) {
  int result = INT16_MAX;
  for (int i = 0; i < len; i++) {
    if (entries[i] < result)
      result = entries[i];
  }

  return result;
}

void RenaultTwizyBattery::update_values() {

  datalayer.battery.status.real_soc = SOC;
  datalayer.battery.status.soh_pptt = SOH;
  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)
  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0)
  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity_Wh;

  // The twizy provides two values: one for the maximum charge provided by the on-board charger
  //    and one for the maximum charge during recuperation.
  //    For now we use the lower of the two (usually the charger one)
  datalayer.battery.status.max_charge_power_W = max_charge_power < max_recup_power ? max_charge_power : max_recup_power;

  datalayer.battery.status.max_discharge_power_W = max_discharge_power;

  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mV, sizeof(cellvoltages_mV));
  datalayer.battery.status.cell_min_voltage_mV =
      min_value(cellvoltages_mV, sizeof(cellvoltages_mV) / sizeof(*cellvoltages_mV));
  datalayer.battery.status.cell_max_voltage_mV =
      max_value(cellvoltages_mV, sizeof(cellvoltages_mV) / sizeof(*cellvoltages_mV));

  datalayer.battery.status.temperature_min_dC =
      min_value(cell_temperatures_dC, sizeof(cell_temperatures_dC) / sizeof(*cell_temperatures_dC));
  datalayer.battery.status.temperature_max_dC =
      max_value(cell_temperatures_dC, sizeof(cell_temperatures_dC) / sizeof(*cell_temperatures_dC));
}

void RenaultTwizyBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x155:
      // max charge power is in steps of 300W from 0 to 7
      max_charge_power = (uint16_t)rx_frame.data.u8[0] * 300;

      // current is encoded as a 12 bit integer with Amps = value / 4 - 500
      current_dA = (((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]) & 0xfff) * 10 / 4 - 5000;

      // SOC is encoded as 16 bit integer with SOC% = value / 400
      SOC = (((uint16_t)rx_frame.data.u8[4] << 8) | (uint16_t)rx_frame.data.u8[5]) / 4;
      break;
    case 0x424:
      max_recup_power = rx_frame.data.u8[2] * 500;
      max_discharge_power = rx_frame.data.u8[3] * 500;
      SOH = (uint16_t)rx_frame.data.u8[5] * 100;
      break;
    case 0x425:
      remaining_capacity_Wh = (uint16_t)rx_frame.data.u8[1] * 100;
      break;
    case 0x554:
      for (int i = 0; i < 7; i++)
        cell_temperatures_dC[i] = (int16_t)rx_frame.data.u8[i] * 10 - 400;
      break;
    case 0x556:
      // cell voltages are 12 bit with V = value / 200
      cellvoltages_mV[0] = (((int16_t)rx_frame.data.u8[0] << 4) | ((int16_t)rx_frame.data.u8[1] >> 4)) * 10 / 2;
      cellvoltages_mV[1] = (((int16_t)(rx_frame.data.u8[1] & 0xf) << 8) | (int16_t)rx_frame.data.u8[2]) * 10 / 2;
      cellvoltages_mV[2] = (((int16_t)rx_frame.data.u8[3] << 4) | ((int16_t)rx_frame.data.u8[4] >> 4)) * 10 / 2;
      cellvoltages_mV[3] = (((int16_t)(rx_frame.data.u8[4] & 0xf) << 8) | (int16_t)rx_frame.data.u8[5]) * 10 / 2;
      cellvoltages_mV[4] = (((int16_t)rx_frame.data.u8[6] << 4) | ((int16_t)rx_frame.data.u8[7] >> 4)) * 10 / 2;
      break;
    case 0x557:
      // cell voltages are 12 bit with V = value / 200
      cellvoltages_mV[5] = (((int16_t)rx_frame.data.u8[0] << 4) | ((int16_t)rx_frame.data.u8[1] >> 4)) * 10 / 2;
      cellvoltages_mV[6] = (((int16_t)(rx_frame.data.u8[1] & 0xf) << 8) | (int16_t)rx_frame.data.u8[2]) * 10 / 2;
      cellvoltages_mV[7] = (((int16_t)rx_frame.data.u8[3] << 4) | ((int16_t)rx_frame.data.u8[4] >> 4)) * 10 / 2;
      cellvoltages_mV[8] = (((int16_t)(rx_frame.data.u8[4] & 0xf) << 8) | (int16_t)rx_frame.data.u8[5]) * 10 / 2;
      cellvoltages_mV[9] = (((int16_t)rx_frame.data.u8[6] << 4) | ((int16_t)rx_frame.data.u8[7] >> 4)) * 10 / 2;
      break;
    case 0x55E:
      // cell voltages are 12 bit with V = value / 200
      cellvoltages_mV[10] = (((int16_t)rx_frame.data.u8[0] << 4) | ((int16_t)rx_frame.data.u8[1] >> 4)) * 10 / 2;
      cellvoltages_mV[11] = (((int16_t)(rx_frame.data.u8[1] & 0xf) << 8) | (int16_t)rx_frame.data.u8[2]) * 10 / 2;
      cellvoltages_mV[12] = (((int16_t)rx_frame.data.u8[3] << 4) | ((int16_t)rx_frame.data.u8[4] >> 4)) * 10 / 2;
      cellvoltages_mV[13] = (((int16_t)(rx_frame.data.u8[4] & 0xf) << 8) | (int16_t)rx_frame.data.u8[5]) * 10 / 2;
      // battery odometer in bytes 6 and 7
      break;
    case 0x55F:
      // TODO: twizy has two pack voltages, assumingly the minimum and maximum measured.
      // They usually only differ by 0.1V. We use the lower one here
      // The other one is in the last 12 bit of the CAN packet

      // pack voltage is encoded as 16 bit integer in dV
      voltage_dV = (((int16_t)rx_frame.data.u8[5] << 4) | ((int16_t)rx_frame.data.u8[6] >> 4));
      break;
    default:
      break;
  }
}

void RenaultTwizyBattery::transmit_can(unsigned long currentMillis) {
  // we do not need to send anything to the battery for now
}

void RenaultTwizyBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 14;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.total_capacity_Wh = 6600;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
