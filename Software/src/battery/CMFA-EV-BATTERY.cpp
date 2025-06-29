#include "CMFA-EV-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* The raw SOC value sits at 90% when the battery is full, so we should report back 100% once this value is reached
Same goes for low point, when 10% is reached we report 0% */

uint16_t CmfaEvBattery::rescale_raw_SOC(uint32_t raw_SOC) {

  uint32_t calc_soc;
  calc_soc = (raw_SOC * 0.25);
  if (calc_soc > MAXSOC) {  //Constrain if needed
    calc_soc = MAXSOC;
  }
  if (calc_soc < MINSOC) {  //Constrain if needed
    calc_soc = MINSOC;
  }
  // Perform scaling between the two points
  calc_soc = 10000 * (calc_soc - MINSOC);
  calc_soc = calc_soc / (MAXSOC - MINSOC);

  return (uint16_t)calc_soc;
}

void CmfaEvBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer_battery->status.soh_pptt = (SOH * 100);

  datalayer_battery->status.real_soc = rescale_raw_SOC(SOC_raw);

  datalayer_battery->status.current_dA = current * 10;

  datalayer_battery->status.voltage_dV = average_voltage_of_cells / 100;

  datalayer_battery->info.total_capacity_Wh = 27000;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = discharge_power_w;

  datalayer_battery->status.max_charge_power_W = charge_power_w;

  datalayer_battery->status.temperature_min_dC = (lowest_cell_temperature * 10);

  datalayer_battery->status.temperature_max_dC = (highest_cell_temperature * 10);

  datalayer_battery->status.cell_min_voltage_mV = lowest_cell_voltage_mv;

  datalayer_battery->status.cell_max_voltage_mV = highest_cell_voltage_mv;

  //Map all cell voltages to the global array
  memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages_mv, 72 * sizeof(uint16_t));

  if (lead_acid_voltage < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, lead_acid_voltage);
  }

  // Update webserver datalayer
  datalayer_cmfa->soc_u = soc_u;
  datalayer_cmfa->soc_z = soc_z;
  datalayer_cmfa->lead_acid_voltage = lead_acid_voltage;
  datalayer_cmfa->highest_cell_voltage_number = highest_cell_voltage_number;
  datalayer_cmfa->lowest_cell_voltage_number = lowest_cell_voltage_number;
  datalayer_cmfa->max_regen_power = max_regen_power;
  datalayer_cmfa->max_discharge_power = max_discharge_power;
  datalayer_cmfa->average_temperature = average_temperature;
  datalayer_cmfa->minimum_temperature = minimum_temperature;
  datalayer_cmfa->maximum_temperature = maximum_temperature;
  datalayer_cmfa->maximum_charge_power = maximum_charge_power;
  datalayer_cmfa->SOH_available_power = SOH_available_power;
  datalayer_cmfa->SOH_generated_power = SOH_generated_power;
  datalayer_cmfa->cumulative_energy_when_discharging = cumulative_energy_when_discharging;
  datalayer_cmfa->cumulative_energy_when_charging = cumulative_energy_when_charging;
  datalayer_cmfa->cumulative_energy_in_regen = cumulative_energy_in_regen;
  datalayer_cmfa->soh_average = soh_average;
}

void CmfaEvBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x127:           //10ms , Same structure as old Zoe 0x155 message!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      current = (((((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2]) * 0.25) - 500);
      SOC_raw = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x3D6:  //100ms, Same structure as old Zoe 0x424 message!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      charge_power_w = rx_frame.data.u8[2] * 500;
      discharge_power_w = rx_frame.data.u8[3] * 500;
      lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
      SOH = rx_frame.data.u8[5];
      heartbeat = rx_frame.data.u8[6];
      highest_cell_temperature = (rx_frame.data.u8[7] - 40);
      break;
    case 0x3D7:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      pack_voltage = ((rx_frame.data.u8[6] << 4 | (rx_frame.data.u8[5] & 0x0F)));
      break;
    case 0x3D8:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //counter_3D8 = rx_frame.data.u8[3]; //?
      //CRC_3D8 = rx_frame.data.u8[4]; //?
      break;
    case 0x43C:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      heartbeat2 = rx_frame.data.u8[2];  //Alternates between 0x55 and 0xAA every 5th frame
      break;
    case 0x431:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //byte0 9C always
      //byte1 40 always
      break;
    case 0x5A9:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5AB:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C8:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E1:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7BB:                           // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can_frame(&CMFA_ACK, can_config.battery);
      }

      pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];

      switch (pid_reply) {
        case PID_POLL_SOCZ:
          soc_z = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_USOC:
          soc_u = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_SOH_AVERAGE:
          soh_average = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_AVERAGE_VOLTAGE_OF_CELLS:
          average_voltage_of_cells =
              (uint32_t)((rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
          break;
        case PID_POLL_HIGHEST_CELL_VOLTAGE:
          highest_cell_voltage_mv = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE:
          highest_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case PID_POLL_LOWEST_CELL_VOLTAGE:
          lowest_cell_voltage_mv = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE:
          lowest_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case PID_POLL_CURRENT_OFFSET:
          //current_offset =
          break;
        case PID_POLL_INSTANT_CURRENT:
          //instant_offset =
          break;
        case PID_POLL_MAX_REGEN:
          max_regen_power = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_MAX_DISCHARGE_POWER:
          max_discharge_power = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_12V_BATTERY:
          lead_acid_voltage = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_AVERAGE_TEMPERATURE:
          average_temperature = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) - 400) / 2);
          break;
        case PID_POLL_MIN_TEMPERATURE:
          minimum_temperature = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) - 400) / 2);
          break;
        case PID_POLL_MAX_TEMPERATURE:
          maximum_temperature = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) - 400) / 2);
          break;
        case PID_POLL_MAX_CHARGE_POWER:
          maximum_charge_power = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_END_OF_CHARGE_FLAG:
          end_of_charge = rx_frame.data.u8[4];
          break;
        case PID_POLL_INTERLOCK_FLAG:
          interlock_flag = rx_frame.data.u8[4];
          break;
        case PID_POLL_SOH_AVAILABLE_POWER_CALCULATION:
          SOH_available_power = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_SOH_GENERATED_POWER_CALCULATION:
          SOH_generated_power = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING:
          cumulative_energy_when_discharging = (uint64_t)((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                                          (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
          break;
        case PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING:
          cumulative_energy_when_charging = (uint64_t)((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                                       (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
          break;
        case PID_POLL_CUMULATIVE_ENERGY_IN_REGEN:
          cumulative_energy_in_regen = (uint64_t)((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                                  (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
        case PID_POLL_CELL_1:
          cellvoltages_mv[0] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_2:
          cellvoltages_mv[1] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_3:
          cellvoltages_mv[2] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_4:
          cellvoltages_mv[3] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_5:
          cellvoltages_mv[4] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_6:
          cellvoltages_mv[5] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_7:
          cellvoltages_mv[6] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_8:
          cellvoltages_mv[7] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_9:
          cellvoltages_mv[8] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_10:
          cellvoltages_mv[9] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_11:
          cellvoltages_mv[10] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_12:
          cellvoltages_mv[11] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_13:
          cellvoltages_mv[12] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_14:
          cellvoltages_mv[13] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_15:
          cellvoltages_mv[14] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_16:
          cellvoltages_mv[15] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_17:
          cellvoltages_mv[16] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_18:
          cellvoltages_mv[17] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_19:
          cellvoltages_mv[18] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_20:
          cellvoltages_mv[19] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_21:
          cellvoltages_mv[20] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_22:
          cellvoltages_mv[21] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_23:
          cellvoltages_mv[22] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_24:
          cellvoltages_mv[23] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_25:
          cellvoltages_mv[24] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_26:
          cellvoltages_mv[25] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_27:
          cellvoltages_mv[26] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_28:
          cellvoltages_mv[27] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_29:
          cellvoltages_mv[28] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_30:
          cellvoltages_mv[29] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_31:
          cellvoltages_mv[30] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_32:
          cellvoltages_mv[31] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_33:
          cellvoltages_mv[32] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_34:
          cellvoltages_mv[33] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_35:
          cellvoltages_mv[34] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_36:
          cellvoltages_mv[35] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_37:
          cellvoltages_mv[36] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_38:
          cellvoltages_mv[37] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_39:
          cellvoltages_mv[38] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_40:
          cellvoltages_mv[39] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_41:
          cellvoltages_mv[40] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_42:
          cellvoltages_mv[41] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_43:
          cellvoltages_mv[42] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_44:
          cellvoltages_mv[43] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_45:
          cellvoltages_mv[44] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_46:
          cellvoltages_mv[45] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_47:
          cellvoltages_mv[46] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_48:
          cellvoltages_mv[47] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_49:
          cellvoltages_mv[48] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_50:
          cellvoltages_mv[49] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_51:
          cellvoltages_mv[50] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_52:
          cellvoltages_mv[51] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_53:
          cellvoltages_mv[52] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_54:
          cellvoltages_mv[53] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_55:
          cellvoltages_mv[54] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_56:
          cellvoltages_mv[55] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_57:
          cellvoltages_mv[56] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_58:
          cellvoltages_mv[57] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_59:
          cellvoltages_mv[58] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_60:
          cellvoltages_mv[59] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_61:
          cellvoltages_mv[60] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_62:
          cellvoltages_mv[61] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_63:
          cellvoltages_mv[62] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_64:
          cellvoltages_mv[63] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_65:
          cellvoltages_mv[64] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_66:
          cellvoltages_mv[65] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_67:
          cellvoltages_mv[66] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_68:
          cellvoltages_mv[67] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_69:
          cellvoltages_mv[68] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_70:
          cellvoltages_mv[69] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_71:
          cellvoltages_mv[70] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_72:
          cellvoltages_mv[71] = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
          break;
        default:
          break;
      }

      break;
    default:
      break;
  }
}

void CmfaEvBattery::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;
    transmit_can_frame(&CMFA_1EA, can_config.battery);
    transmit_can_frame(&CMFA_135, can_config.battery);
    transmit_can_frame(&CMFA_134, can_config.battery);
    transmit_can_frame(&CMFA_125, can_config.battery);

    CMFA_135.data.u8[1] = content_135[counter_10ms];
    CMFA_125.data.u8[3] = content_125[counter_10ms];
    counter_10ms = (counter_10ms + 1) % 16;  // counter_10ms cycles between 0-1-2-3..15-0-1...
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&CMFA_59B, can_config.battery);
    transmit_can_frame(&CMFA_3D3, can_config.battery);
  }
  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    switch (poll_pid) {
      case PID_POLL_SOH_AVERAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_SOH_AVERAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_SOH_AVERAGE;
        poll_pid = PID_POLL_AVERAGE_VOLTAGE_OF_CELLS;
        break;
      case PID_POLL_AVERAGE_VOLTAGE_OF_CELLS:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_AVERAGE_VOLTAGE_OF_CELLS >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_AVERAGE_VOLTAGE_OF_CELLS;
        poll_pid = PID_POLL_HIGHEST_CELL_VOLTAGE;
        break;
      case PID_POLL_HIGHEST_CELL_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_HIGHEST_CELL_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_HIGHEST_CELL_VOLTAGE;
        poll_pid = PID_POLL_LOWEST_CELL_VOLTAGE;
        break;
      case PID_POLL_LOWEST_CELL_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_LOWEST_CELL_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_LOWEST_CELL_VOLTAGE;
        poll_pid = PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE;
        break;
      case PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE;
        poll_pid = PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE;
        break;
      case PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE;
        poll_pid = PID_POLL_12V_BATTERY;
        break;
      case PID_POLL_12V_BATTERY:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_12V_BATTERY >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_12V_BATTERY;
        poll_pid = PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING;
        break;
      case PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING;
        poll_pid = PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING;
        break;
      case PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING;
        poll_pid = PID_POLL_CUMULATIVE_ENERGY_IN_REGEN;
        break;
      case PID_POLL_CUMULATIVE_ENERGY_IN_REGEN:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CUMULATIVE_ENERGY_IN_REGEN >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CUMULATIVE_ENERGY_IN_REGEN;
        poll_pid = PID_POLL_SOCZ;
        break;
      case PID_POLL_SOCZ:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_SOCZ >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_SOCZ;
        poll_pid = PID_POLL_USOC;
        break;
      case PID_POLL_USOC:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_USOC >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_USOC;
        poll_pid = PID_POLL_CURRENT_OFFSET;
        break;
      case PID_POLL_CURRENT_OFFSET:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CURRENT_OFFSET >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CURRENT_OFFSET;
        poll_pid = PID_POLL_INSTANT_CURRENT;
        break;
      case PID_POLL_INSTANT_CURRENT:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_INSTANT_CURRENT >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_INSTANT_CURRENT;
        poll_pid = PID_POLL_MAX_REGEN;
        break;
      case PID_POLL_MAX_REGEN:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_MAX_REGEN >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_MAX_REGEN;
        poll_pid = PID_POLL_MAX_DISCHARGE_POWER;
        break;
      case PID_POLL_MAX_DISCHARGE_POWER:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_MAX_DISCHARGE_POWER >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_MAX_DISCHARGE_POWER;
        poll_pid = PID_POLL_MAX_CHARGE_POWER;
        break;
      case PID_POLL_MAX_CHARGE_POWER:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_MAX_CHARGE_POWER >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_MAX_CHARGE_POWER;
        poll_pid = PID_POLL_AVERAGE_TEMPERATURE;
        break;
      case PID_POLL_AVERAGE_TEMPERATURE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_AVERAGE_TEMPERATURE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_AVERAGE_TEMPERATURE;
        poll_pid = PID_POLL_MIN_TEMPERATURE;
        break;
      case PID_POLL_MIN_TEMPERATURE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_MIN_TEMPERATURE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_MIN_TEMPERATURE;
        poll_pid = PID_POLL_MAX_TEMPERATURE;
        break;
      case PID_POLL_MAX_TEMPERATURE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_MAX_TEMPERATURE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_MAX_TEMPERATURE;
        poll_pid = PID_POLL_END_OF_CHARGE_FLAG;
        break;
      case PID_POLL_END_OF_CHARGE_FLAG:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_END_OF_CHARGE_FLAG >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_END_OF_CHARGE_FLAG;
        poll_pid = PID_POLL_INTERLOCK_FLAG;
        break;
      case PID_POLL_INTERLOCK_FLAG:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_INTERLOCK_FLAG >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_INTERLOCK_FLAG;
        poll_pid = PID_POLL_CELL_1;
        break;
      case PID_POLL_CELL_1:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_1 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_1;
        poll_pid = PID_POLL_CELL_2;
        break;
      case PID_POLL_CELL_2:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_2 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_2;
        poll_pid = PID_POLL_CELL_3;
        break;
      case PID_POLL_CELL_3:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_3 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_3;
        poll_pid = PID_POLL_CELL_4;
        break;
      case PID_POLL_CELL_4:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_4 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_4;
        poll_pid = PID_POLL_CELL_5;
        break;
      case PID_POLL_CELL_5:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_5 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_5;
        poll_pid = PID_POLL_CELL_6;
        break;
      case PID_POLL_CELL_6:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_6 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_6;
        poll_pid = PID_POLL_CELL_7;
        break;
      case PID_POLL_CELL_7:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_7 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_7;
        poll_pid = PID_POLL_CELL_8;
        break;
      case PID_POLL_CELL_8:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_8 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_8;
        poll_pid = PID_POLL_CELL_9;
        break;
      case PID_POLL_CELL_9:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_9 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_9;
        poll_pid = PID_POLL_CELL_10;
        break;
      case PID_POLL_CELL_10:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_10 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_10;
        poll_pid = PID_POLL_CELL_11;
        break;
      case PID_POLL_CELL_11:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_11 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_11;
        poll_pid = PID_POLL_CELL_12;
        break;
      case PID_POLL_CELL_12:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_12 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_12;
        poll_pid = PID_POLL_CELL_13;
        break;
      case PID_POLL_CELL_13:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_13 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_13;
        poll_pid = PID_POLL_CELL_14;
        break;
      case PID_POLL_CELL_14:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_14 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_14;
        poll_pid = PID_POLL_CELL_15;
        break;
      case PID_POLL_CELL_15:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_15 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_15;
        poll_pid = PID_POLL_CELL_16;
        break;
      case PID_POLL_CELL_16:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_16 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_16;
        poll_pid = PID_POLL_CELL_17;
        break;
      case PID_POLL_CELL_17:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_17 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_17;
        poll_pid = PID_POLL_CELL_18;
        break;
      case PID_POLL_CELL_18:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_18 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_18;
        poll_pid = PID_POLL_CELL_19;
        break;
      case PID_POLL_CELL_19:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_19 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_19;
        poll_pid = PID_POLL_CELL_20;
        break;
      case PID_POLL_CELL_20:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_20 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_20;
        poll_pid = PID_POLL_CELL_21;
        break;
      case PID_POLL_CELL_21:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_21 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_21;
        poll_pid = PID_POLL_CELL_22;
        break;
      case PID_POLL_CELL_22:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_22 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_22;
        poll_pid = PID_POLL_CELL_23;
        break;
      case PID_POLL_CELL_23:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_23 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_23;
        poll_pid = PID_POLL_CELL_24;
        break;
      case PID_POLL_CELL_24:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_24 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_24;
        poll_pid = PID_POLL_CELL_25;
        break;
      case PID_POLL_CELL_25:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_25 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_25;
        poll_pid = PID_POLL_CELL_26;
        break;
      case PID_POLL_CELL_26:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_26 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_26;
        poll_pid = PID_POLL_CELL_27;
        break;
      case PID_POLL_CELL_27:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_27 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_27;
        poll_pid = PID_POLL_CELL_28;
        break;
      case PID_POLL_CELL_28:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_28 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_28;
        poll_pid = PID_POLL_CELL_29;
        break;
      case PID_POLL_CELL_29:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_29 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_29;
        poll_pid = PID_POLL_CELL_30;
        break;
      case PID_POLL_CELL_30:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_30 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_30;
        poll_pid = PID_POLL_CELL_31;
        break;
      case PID_POLL_CELL_31:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_31 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_31;
        poll_pid = PID_POLL_CELL_32;
        break;
      case PID_POLL_CELL_32:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_32 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_32;
        poll_pid = PID_POLL_CELL_33;
        break;
      case PID_POLL_CELL_33:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_33 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_33;
        poll_pid = PID_POLL_CELL_34;
        break;
      case PID_POLL_CELL_34:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_34 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_34;
        poll_pid = PID_POLL_CELL_35;
        break;
      case PID_POLL_CELL_35:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_35 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_35;
        poll_pid = PID_POLL_CELL_36;
        break;
      case PID_POLL_CELL_36:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_36 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_36;
        poll_pid = PID_POLL_CELL_37;
        break;
      case PID_POLL_CELL_37:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_37 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_37;
        poll_pid = PID_POLL_CELL_38;
        break;
      case PID_POLL_CELL_38:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_38 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_38;
        poll_pid = PID_POLL_CELL_39;
        break;
      case PID_POLL_CELL_39:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_39 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_39;
        poll_pid = PID_POLL_CELL_40;
        break;
      case PID_POLL_CELL_40:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_40 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_40;
        poll_pid = PID_POLL_CELL_41;
        break;
      case PID_POLL_CELL_41:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_41 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_41;
        poll_pid = PID_POLL_CELL_42;
        break;
      case PID_POLL_CELL_42:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_42 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_42;
        poll_pid = PID_POLL_CELL_43;
        break;
      case PID_POLL_CELL_43:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_43 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_43;
        poll_pid = PID_POLL_CELL_44;
        break;
      case PID_POLL_CELL_44:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_44 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_44;
        poll_pid = PID_POLL_CELL_45;
        break;
      case PID_POLL_CELL_45:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_45 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_45;
        poll_pid = PID_POLL_CELL_46;
        break;
      case PID_POLL_CELL_46:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_46 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_46;
        poll_pid = PID_POLL_CELL_47;
        break;
      case PID_POLL_CELL_47:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_47 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_47;
        poll_pid = PID_POLL_CELL_48;
        break;
      case PID_POLL_CELL_48:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_48 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_48;
        poll_pid = PID_POLL_CELL_49;
        break;
      case PID_POLL_CELL_49:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_49 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_49;
        poll_pid = PID_POLL_CELL_50;
        break;
      case PID_POLL_CELL_50:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_50 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_50;
        poll_pid = PID_POLL_CELL_51;
        break;
      case PID_POLL_CELL_51:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_51 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_51;
        poll_pid = PID_POLL_CELL_52;
        break;
      case PID_POLL_CELL_52:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_52 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_52;
        poll_pid = PID_POLL_CELL_53;
        break;
      case PID_POLL_CELL_53:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_53 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_53;
        poll_pid = PID_POLL_CELL_54;
        break;
      case PID_POLL_CELL_54:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_54 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_54;
        poll_pid = PID_POLL_CELL_55;
        break;
      case PID_POLL_CELL_55:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_55 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_55;
        poll_pid = PID_POLL_CELL_56;
        break;
      case PID_POLL_CELL_56:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_56 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_56;
        poll_pid = PID_POLL_CELL_57;
        break;
      case PID_POLL_CELL_57:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_57 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_57;
        poll_pid = PID_POLL_CELL_58;
        break;
      case PID_POLL_CELL_58:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_58 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_58;
        poll_pid = PID_POLL_CELL_59;
        break;
      case PID_POLL_CELL_59:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_59 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_59;
        poll_pid = PID_POLL_CELL_60;
        break;
      case PID_POLL_CELL_60:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_60 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_60;
        poll_pid = PID_POLL_CELL_61;
        break;
      case PID_POLL_CELL_61:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_61 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_61;
        poll_pid = PID_POLL_CELL_62;
        break;
      case PID_POLL_CELL_62:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_62 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_62;
        poll_pid = PID_POLL_CELL_63;
        break;
      case PID_POLL_CELL_63:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_63 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_63;
        poll_pid = PID_POLL_CELL_64;
        break;
      case PID_POLL_CELL_64:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_64 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_64;
        poll_pid = PID_POLL_CELL_65;
        break;
      case PID_POLL_CELL_65:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_65 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_65;
        poll_pid = PID_POLL_CELL_66;
        break;
      case PID_POLL_CELL_66:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_66 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_66;
        poll_pid = PID_POLL_CELL_67;
        break;
      case PID_POLL_CELL_67:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_67 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_67;
        poll_pid = PID_POLL_CELL_68;
        break;
      case PID_POLL_CELL_68:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_68 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_68;
        poll_pid = PID_POLL_CELL_69;
        break;
      case PID_POLL_CELL_69:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_69 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_69;
        poll_pid = PID_POLL_CELL_70;
        break;
      case PID_POLL_CELL_70:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_70 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_70;
        poll_pid = PID_POLL_CELL_71;
        break;
      case PID_POLL_CELL_71:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_71 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_71;
        poll_pid = PID_POLL_CELL_72;
        break;
      case PID_POLL_CELL_72:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_72 >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_72;
        poll_pid = PID_POLL_SOH_AVERAGE;
        break;
        poll_pid = PID_POLL_SOH_AVERAGE;
        break;
    }

    transmit_can_frame(&CMFA_POLLING_FRAME, can_config.battery);
  }
}

void CmfaEvBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 72;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
