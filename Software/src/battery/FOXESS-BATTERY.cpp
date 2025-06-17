#include "FOXESS-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/*
Can bus @ 500k - all Extended ID, little endian
All info from https://github.com/FozzieUK/FoxESS-Canbus-Protocol

TODO:
- Check that current is signed right way
*/

void FoxessBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W =
      ((datalayer.battery.status.voltage_dV * max_discharge_power_dA) / 100);

  datalayer.battery.status.max_charge_power_W = ((datalayer.battery.status.voltage_dV * max_charge_power_dA) / 100);

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mV, 128 * sizeof(uint16_t));

  switch (NUMBER_OF_PACKS) {
    case 1:  //HS2.6 (48V invalid combo for most HV inverters)
      datalayer.battery.info.number_of_cells = 16;
      datalayer.battery.info.max_design_voltage_dV = 584;
      datalayer.battery.info.min_design_voltage_dV = 400;
      break;
    case 2:  //HS5.2
      datalayer.battery.info.number_of_cells = 32;
      datalayer.battery.info.max_design_voltage_dV = 1168;
      datalayer.battery.info.min_design_voltage_dV = 800;
      break;
    case 3:  //HS7.8
      datalayer.battery.info.number_of_cells = 48;
      datalayer.battery.info.max_design_voltage_dV = 1752;
      datalayer.battery.info.min_design_voltage_dV = 1200;
      break;
    case 4:  //HS10.4
      datalayer.battery.info.number_of_cells = 64;
      datalayer.battery.info.max_design_voltage_dV = 2336;
      datalayer.battery.info.min_design_voltage_dV = 1600;
      break;
    case 5:  //HS13
      datalayer.battery.info.number_of_cells = 80;
      datalayer.battery.info.max_design_voltage_dV = 2920;
      datalayer.battery.info.min_design_voltage_dV = 2000;
      break;
    case 6:  //HS15.6
      datalayer.battery.info.number_of_cells = 96;
      datalayer.battery.info.max_design_voltage_dV = 3504;
      datalayer.battery.info.min_design_voltage_dV = 2400;
      break;
    case 7:  //HS18.2
      datalayer.battery.info.number_of_cells = 112;
      datalayer.battery.info.max_design_voltage_dV = 4088;
      datalayer.battery.info.min_design_voltage_dV = 2800;
      break;
    case 8:  //HS20.8
      datalayer.battery.info.number_of_cells = 128;
      datalayer.battery.info.max_design_voltage_dV = 4672;
      datalayer.battery.info.min_design_voltage_dV = 3200;
      break;
    default:
      //Data not available yet
      break;
  }
}

void FoxessBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1872:  //BMS_Limits
      datalayer.battery.info.max_design_voltage_dV = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      datalayer.battery.info.min_design_voltage_dV = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      if (!charging_disabled) {
        max_charge_power_dA = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      }
      max_discharge_power_dA = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x1873:  //BMS_PackData
      datalayer.battery.status.voltage_dV = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      datalayer.battery.status.current_dA =
          (int16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);  //TODO: Direction right way?
      datalayer.battery.status.real_soc = (rx_frame.data.u8[4] * 100);
      datalayer.battery.status.remaining_capacity_Wh = ((rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 10);
      break;
    case 0x1874:  //BMS_CellData
      datalayer.battery.status.temperature_max_dC = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      datalayer.battery.status.temperature_min_dC = (int16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cut_mv_max = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cut_mv_min = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x1875:  //BMS_Status
      temperature_average = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      STATUS_OPERATIONAL_PACKS = (uint8_t)rx_frame.data.u8[2];
      NUMBER_OF_PACKS = (uint8_t)rx_frame.data.u8[3];
      contactor_status = (uint8_t)rx_frame.data.u8[4];
      cycle_count = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x1876:  //BMS_PackTemps
      // 0x1876 b0 bit 0 appears to be 1 when at maxsoc and BMS says charge is not allowed -
      // when at 0 indicates charge is possible - additional note there is something more to it than this,
      // it's not as straight forward - needs more testing to find what sets/unsets bit0 of byte0
      if ((rx_frame.data.u8[0] & 0x01) > 0) {
        max_charge_power_dA = 0;
        charging_disabled = true;
      } else {
        charging_disabled = false;
      }

      datalayer.battery.status.cell_max_voltage_mV = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      datalayer.battery.status.cell_min_voltage_mV = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x1877:  //BMS_ErrorsBrand
      pack_error = (uint8_t)(rx_frame.data.u8[0]);
      firmware_pack_minor = (uint8_t)(rx_frame.data.u8[6] & 0x0F);
      firmware_pack_major = (uint8_t)((rx_frame.data.u8[6] & 0xF0) >> 4);
      break;
    case 0x1878:  //BMS_PackStats
      max_ac_voltage = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      total_watt_hours = (uint32_t)(rx_frame.data.u8[7] << 24 | rx_frame.data.u8[6] << 16 | rx_frame.data.u8[5] << 8 |
                                    rx_frame.data.u8[4]);
      break;
    case 0x1879:  //BMS_PackState
      b0_idle = (bool)(rx_frame.data.u8[1] & 0x01);
      b1_ok_discharge = (bool)((rx_frame.data.u8[1] & 0x02) >> 1);
      b2_ok_charge = (bool)((rx_frame.data.u8[1] & 0x04) >> 2);
      b3_discharging = (bool)((rx_frame.data.u8[1] & 0x08) >> 3);
      b4_charging = (bool)((rx_frame.data.u8[1] & 0x20) >> 4);
      b5_operational = (bool)((rx_frame.data.u8[1] & 0x40) >> 5);
      b6_active_error = (bool)((rx_frame.data.u8[1] & 0x80) >> 6);
      break;
    case 0x1881:  //These contain serial number. No interest to us
    case 0x1882:  //These contain serial number. No interest to us
    case 0x1883:  //These contain serial number. No interest to us
      break;
    case 0x0C05:  //Pack 1
      pack1_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack1_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack1_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack1_SOC = (uint8_t)rx_frame.data.u8[4];
      pack1_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C06:  //Pack 2
      pack2_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack2_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack2_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack2_SOC = (uint8_t)rx_frame.data.u8[4];
      pack2_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C07:  //Pack 3
      pack3_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack3_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack3_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack3_SOC = (uint8_t)rx_frame.data.u8[4];
      pack3_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C08:  //Pack 4
      pack4_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack4_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack4_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack4_SOC = (uint8_t)rx_frame.data.u8[4];
      pack4_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C09:  //Pack 5
      pack5_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack5_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack5_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack5_SOC = (uint8_t)rx_frame.data.u8[4];
      pack5_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C0A:  //Pack 6
      pack6_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack6_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack6_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack6_SOC = (uint8_t)rx_frame.data.u8[4];
      pack6_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C0B:  //Pack 7
      pack7_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack7_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack7_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack7_SOC = (uint8_t)rx_frame.data.u8[4];
      pack7_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C0C:  //Pack 8
      pack8_current_sensor = (int16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      pack8_temperature_avg_high = (int16_t)(rx_frame.data.u8[2] - 50);
      pack8_temperature_avg_low = (int16_t)(rx_frame.data.u8[3] - 50);
      pack8_SOC = (uint8_t)rx_frame.data.u8[4];
      pack8_voltage = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0D21:  //These CAN messages contain raw cell temperatures
    case 0x0D29:  //They are not required, we already read min/max from each pack
    case 0x0D31:
    case 0x0D39:
    case 0x0D41:
    case 0x0D49:
    case 0x0D51:
    case 0x0D59:
      break;
    case 0x0C1D:  // Cellvolts 1
      cellvoltages_mV[0] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[1] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[2] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[3] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C21:  // Cellvolts 2
      cellvoltages_mV[4] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[5] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[6] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[7] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C25:  // Cellvolts 3
      cellvoltages_mV[8] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[9] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[10] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[11] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C29:  // Cellvolts 4
      cellvoltages_mV[12] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[13] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[14] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[15] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C2D:  // Cellvolts 5
      cellvoltages_mV[16] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[17] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[18] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[19] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C31:  // Cellvolts 6
      cellvoltages_mV[20] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[21] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[22] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[23] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C35:  // Cellvolts 7
      cellvoltages_mV[24] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[25] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[26] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[27] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C39:  // Cellvolts 8
      cellvoltages_mV[28] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[29] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[30] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[31] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C3D:  // Cellvolts 9
      cellvoltages_mV[32] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[33] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[34] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[35] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C41:  // Cellvolts 10
      cellvoltages_mV[36] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[37] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[38] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[39] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C45:  // Cellvolts 11
      cellvoltages_mV[40] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[41] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[42] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[43] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C49:  // Cellvolts 12
      cellvoltages_mV[44] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[45] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[46] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[47] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C4D:  // Cellvolts 13
      cellvoltages_mV[48] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[49] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[50] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[51] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C51:  // Cellvolts 14
      cellvoltages_mV[52] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[53] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[54] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[55] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C55:  // Cellvolts 15
      cellvoltages_mV[56] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[57] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[58] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[59] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C59:  // Cellvolts 16
      cellvoltages_mV[60] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[61] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[62] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[63] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C5D:  // Cellvolts 17
      cellvoltages_mV[64] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[65] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[66] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[67] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C61:  // Cellvolts 18
      cellvoltages_mV[68] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[69] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[70] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[71] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C65:  // Cellvolts 19
      cellvoltages_mV[72] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[73] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[74] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[75] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C69:  // Cellvolts 20
      cellvoltages_mV[76] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[77] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[78] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[79] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C6D:  // Cellvolts 21
      cellvoltages_mV[80] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[81] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[82] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[83] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C71:  // Cellvolts 22
      cellvoltages_mV[84] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[85] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[86] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[87] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C75:  // Cellvolts 23
      cellvoltages_mV[88] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[89] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[90] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[91] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C79:  // Cellvolts 24
      cellvoltages_mV[92] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[93] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[94] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[95] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C7D:  // Cellvolts 25
      cellvoltages_mV[96] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[97] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[98] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[99] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C81:  // Cellvolts 26
      cellvoltages_mV[100] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[101] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[102] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[103] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C85:  // Cellvolts 27
      cellvoltages_mV[104] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[105] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[106] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[107] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C89:  // Cellvolts 28
      cellvoltages_mV[108] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[109] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[110] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[111] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C8D:  // Cellvolts 29
      cellvoltages_mV[112] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[113] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[114] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[115] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C91:  // Cellvolts 30
      cellvoltages_mV[116] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[117] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[118] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[119] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C95:  // Cellvolts 31
      cellvoltages_mV[120] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[121] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[122] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[123] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x0C99:  // Cellvolts 32
      cellvoltages_mV[124] = (uint16_t)(rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      cellvoltages_mV[125] = (uint16_t)(rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      cellvoltages_mV[126] = (uint16_t)(rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      cellvoltages_mV[127] = (uint16_t)(rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    default:
      break;
  }
}

void FoxessBattery::transmit_can(unsigned long currentMillis) {
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    switch (statemachine_polling) {
      case 0:  //0.5s
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_statistics
        break;
      case 1:  //1s
        FOX_1871.data.u8[0] = 0x02;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_stop_sending
        break;
      case 2:  //1.5s
        FOX_1871.data.u8[0] = 0x05;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_serial_request
        break;
      case 3:  //2.0s
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_statistics
        break;
      case 4:  //2.5s
        FOX_1871.data.u8[0] = 0x02;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_stop_sending
        break;
      case 5:  //3.0s cell temp and voltages
        //0x1871 [0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00]
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x02;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_cell_volts
        //0x1871 [0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00]
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x04;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_cell_temps
        break;
      case 6:  //3.5s
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_statistics
        break;
      case 7:  //4.0s
        FOX_1871.data.u8[0] = 0x02;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_stop_sending
        break;
      case 8:  //4.5s
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_statistics
        break;
      case 9:  //5.0s
        FOX_1871.data.u8[0] = 0x02;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x00;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_stop_sending
        break;
      case 10:  //5.5s
        //0x1871 [0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00]
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x02;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_cell_volts
        //0x1871 [0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00]
        FOX_1871.data.u8[0] = 0x01;
        FOX_1871.data.u8[1] = 0x00;
        FOX_1871.data.u8[2] = 0x01;
        FOX_1871.data.u8[3] = 0x00;
        FOX_1871.data.u8[4] = 0x04;
        FOX_1871.data.u8[5] = 0x00;
        FOX_1871.data.u8[6] = 0x00;
        FOX_1871.data.u8[7] = 0x00;
        transmit_can_frame(&FOX_1871, can_config.battery);  //bms_send_pack_cell_temps
        break;
      case 11:  //6.0s 0x1871 [0x03, 0x06, 0x17, 0x05, 0x09, 0x09, 0x28, 0x22]
        FOX_1871.data.u8[0] = 0x03;
        FOX_1871.data.u8[1] = 0x06;
        FOX_1871.data.u8[2] = 0x17;
        FOX_1871.data.u8[3] = 0x05;
        FOX_1871.data.u8[4] = 0x09;
        FOX_1871.data.u8[5] = 0x09;
        FOX_1871.data.u8[6] = 0x28;
        FOX_1871.data.u8[7] = 0x22;
        transmit_can_frame(&FOX_1871, can_config.battery);  //timestamp
        break;
      default:
        statemachine_polling = 0;
        break;
    }

    statemachine_polling = (statemachine_polling + 1) % 12;
  }
}

void FoxessBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 0;  //Startup with no cells, populates later when we know packsize
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
