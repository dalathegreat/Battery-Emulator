#include "KIA-HYUNDAI-64-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void KiaHyundai64Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer_battery->status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer_battery->status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer_battery->status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0) , invert the sign

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_charge_power_W = allowedChargePower * 10;

  datalayer_battery->status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer_battery->status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer_battery->status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer_battery->status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer_battery->status.cell_min_voltage_mV = CellVoltMin_mV;

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Update webserver datalayer
  datalayer_battery_extended->total_cell_count = datalayer_battery->info.number_of_cells;
  datalayer_battery_extended->battery_12V = leadAcidBatteryVoltage;
  datalayer_battery_extended->waterleakageSensor = waterleakageSensor;
  datalayer_battery_extended->temperature_water_inlet = temperature_water_inlet;
  datalayer_battery_extended->powerRelayTemperature = powerRelayTemperature * 2;
  datalayer_battery_extended->batteryManagementMode = batteryManagementMode;
  datalayer_battery_extended->BMS_ign = BMS_ign;
  datalayer_battery_extended->batteryRelay = batteryRelay;
  datalayer_battery_extended->inverterVoltage = inverterVoltage;
  memcpy(datalayer_battery_extended->ecu_serial_number, ecu_serial_number, sizeof(ecu_serial_number));
  memcpy(datalayer_battery_extended->ecu_version_number, ecu_version_number, sizeof(ecu_version_number));
  datalayer_battery_extended->cumulative_charge_current_ah = cumulative_charge_current_ah;
  datalayer_battery_extended->cumulative_discharge_current_ah = cumulative_discharge_current_ah;
  datalayer_battery_extended->cumulative_energy_charged_kWh = cumulative_energy_charged_kWh;
  datalayer_battery_extended->cumulative_energy_discharged_kWh = cumulative_energy_discharged_kWh;
  datalayer_battery_extended->powered_on_total_time = powered_on_total_time;
  datalayer_battery_extended->isolation_resistance_kOhm = isolation_resistance_kOhm;
  datalayer_battery_extended->number_of_standard_charging_sessions = number_of_standard_charging_sessions;
  datalayer_battery_extended->number_of_fastcharging_sessions = number_of_fastcharging_sessions;
  datalayer_battery_extended->accumulated_normal_charging_energy_kWh = accumulated_normal_charging_energy_kWh;
  datalayer_battery_extended->accumulated_fastcharging_energy_kWh = accumulated_fastcharging_energy_kWh;
}

void KiaHyundai64Battery::update_number_of_cells() {
  // Check if we have 98S or 90S battery. If the 98th cell is valid range, we are on a 98S battery
  if ((datalayer_battery->status.cell_voltages_mV[97] > 2000) &&
      (datalayer_battery->status.cell_voltages_mV[97] < 4500)) {
    datalayer_battery->info.number_of_cells = 98;
    datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;
    datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_98S_DV;
    datalayer_battery->info.total_capacity_Wh = 64000;
  } else {
    datalayer_battery->info.number_of_cells = 90;
    datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_90S_DV;
    datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;
    datalayer_battery->info.total_capacity_Wh = 40000;
  }
}

void KiaHyundai64Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4DE:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      startedUp = true;
      break;
    case 0x542:  //BMS SOC
      startedUp = true;
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      startedUp = true;
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      startedUp = true;
      batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (counter_200 > 3) {
        KIA_HYUNDAI_524.data.u8[0] = (uint8_t)(batteryVoltage / 10);
        KIA_HYUNDAI_524.data.u8[1] = (uint8_t)((batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms
      break;
    case 0x596:
      startedUp = true;
      leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      startedUp = true;
      break;
    case 0x5D5:
      startedUp = true;
      waterleakageSensor = rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      startedUp = true;

      //PID data is polled after last message sent from battery every other time this 0x5D8 message arrives:
      if (holdPidCounter == true) {
        holdPidCounter = false;
      } else {
        holdPidCounter = true;

        poll_data_pid++;
        if (poll_data_pid == 1) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_1 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_1 & 0x00FF);
        } else if (poll_data_pid == 2) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_2 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_2 & 0x00FF);
        } else if (poll_data_pid == 3) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_3 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_3 & 0x00FF);
        } else if (poll_data_pid == 4) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_4 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_4 & 0x00FF);
        } else if (poll_data_pid == 5) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_5 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_5 & 0x00FF);
        } else if (poll_data_pid == 6) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_6 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_6 & 0x00FF);
        } else if (poll_data_pid == 7) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_GROUP_11 & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_GROUP_11 & 0x00FF);
        } else if (poll_data_pid == 8) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_ECU_SERIAL & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_ECU_SERIAL & 0x00FF);
        } else if (poll_data_pid == 9) {
          KIA64_7E4_poll.data.u8[2] = (uint8_t)((POLL_ECU_VERSION & 0xFF00) >> 8);
          KIA64_7E4_poll.data.u8[3] = (uint8_t)(POLL_ECU_VERSION & 0x00FF);
          poll_data_pid = 0;
        }
        transmit_can_frame(&KIA64_7E4_poll);
      }
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian

      if (rx_frame.data.u8[0] < 0x10) {  //One line response
        pid_reply = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      }

      if (rx_frame.data.u8[0] == 0x10) {  //Multiframe response, send ACK
        transmit_can_frame(&KIA64_7E4_ack);
        pid_reply = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      switch (rx_frame.data.u8[0]) {  //Multiframe responses
        case 0x10:                    //Header frame sometimes has data
          if (pid_reply == POLL_ECU_SERIAL) {
            ecu_serial_number[0] = rx_frame.data.u8[5];
            ecu_serial_number[1] = rx_frame.data.u8[6];
            ecu_serial_number[2] = rx_frame.data.u8[7];
          } else if (pid_reply == POLL_ECU_VERSION) {
            ecu_version_number[0] = rx_frame.data.u8[5];
            ecu_version_number[1] = rx_frame.data.u8[6];
            ecu_version_number[2] = rx_frame.data.u8[7];
          }
          break;
        case 0x21:  //First frame in PID group
          if (pid_reply == POLL_GROUP_1) {
            batteryRelay = rx_frame.data.u8[7];
          } else if (pid_reply == POLL_GROUP_2) {
            cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_3) {
            cellvoltages_mv[32] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[37] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_4) {
            cellvoltages_mv[64] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[65] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[66] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[67] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[68] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[69] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_11) {
            number_of_standard_charging_sessions = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          } else if (pid_reply == POLL_ECU_SERIAL) {
            ecu_serial_number[3] = rx_frame.data.u8[1];
            ecu_serial_number[4] = rx_frame.data.u8[2];
            ecu_serial_number[5] = rx_frame.data.u8[3];
            ecu_serial_number[6] = rx_frame.data.u8[4];
            ecu_serial_number[7] = rx_frame.data.u8[5];
            ecu_serial_number[8] = rx_frame.data.u8[6];
            ecu_serial_number[9] = rx_frame.data.u8[7];
          } else if (pid_reply == POLL_ECU_VERSION) {
            ecu_version_number[3] = rx_frame.data.u8[1];
            ecu_version_number[4] = rx_frame.data.u8[2];
            ecu_version_number[5] = rx_frame.data.u8[3];
            ecu_version_number[6] = rx_frame.data.u8[4];
            ecu_version_number[7] = rx_frame.data.u8[5];
            ecu_version_number[8] = rx_frame.data.u8[6];
            ecu_version_number[9] = rx_frame.data.u8[7];
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            //battery_max_temperature = rx_frame.data.u8[5];
            //battery_min_temperature = rx_frame.data.u8[6];
            //module_1_temperature = rx_frame.data.u8[7];
          } else if (pid_reply == POLL_GROUP_2) {
            cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_3) {
            cellvoltages_mv[38] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[39] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[43] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_4) {
            cellvoltages_mv[70] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[71] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[72] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[73] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[74] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[75] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[76] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_6) {
            batteryManagementMode = rx_frame.data.u8[5];
          } else if (pid_reply == POLL_GROUP_11) {
            number_of_fastcharging_sessions = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
            accumulated_normal_charging_energy_kWh = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
          } else if (pid_reply == POLL_ECU_SERIAL) {
            ecu_serial_number[10] = rx_frame.data.u8[1];
            ecu_serial_number[11] = rx_frame.data.u8[2];
            ecu_serial_number[12] = rx_frame.data.u8[3];
            ecu_serial_number[13] = rx_frame.data.u8[4];
            ecu_serial_number[14] = rx_frame.data.u8[5];
            ecu_serial_number[15] = rx_frame.data.u8[6];
          } else if (pid_reply == POLL_ECU_VERSION) {
            ecu_version_number[10] = rx_frame.data.u8[1];
            ecu_version_number[11] = rx_frame.data.u8[2];
            ecu_version_number[12] = rx_frame.data.u8[3];
            ecu_version_number[13] = rx_frame.data.u8[4];
            ecu_version_number[14] = rx_frame.data.u8[5];
            ecu_version_number[15] = rx_frame.data.u8[6];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            //module_2_temperature = rx_frame.data.u8[1];
            //module_3_temperature = rx_frame.data.u8[2];
            //module_4_temperature = rx_frame.data.u8[3];
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (pid_reply == POLL_GROUP_2) {
            cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_3) {
            cellvoltages_mv[45] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[46] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[49] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_4) {
            cellvoltages_mv[77] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[78] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[79] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[80] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[81] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[82] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[83] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_5) {
            heatertemp = rx_frame.data.u8[7];
          } else if (pid_reply == POLL_GROUP_11) {
            accumulated_fastcharging_energy_kWh = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (pid_reply == POLL_GROUP_2) {
            cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_3) {
            cellvoltages_mv[52] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[53] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (pid_reply == POLL_GROUP_4) {
            cellvoltages_mv[84] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[85] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[86] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[87] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[88] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[89] = (rx_frame.data.u8[6] * 20);
            if (rx_frame.data.u8[7] > 4) {                       // Data only valid on 98S
              cellvoltages_mv[90] = (rx_frame.data.u8[7] * 20);  // Perform extra checks
            }
          } else if (pid_reply == POLL_GROUP_5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            cumulative_charge_current_ah =
                ((rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            cumulative_discharge_current_ah =
                ((rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          } else if (pid_reply == POLL_GROUP_2) {
            cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[31] = (rx_frame.data.u8[5] * 20);
          } else if (pid_reply == POLL_GROUP_3) {
            cellvoltages_mv[59] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[60] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[61] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[62] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[63] = (rx_frame.data.u8[5] * 20);
          } else if (pid_reply == POLL_GROUP_4) {  // Data only valid on 98S
            if (rx_frame.data.u8[1] > 4) {         // Perform extra checks
              cellvoltages_mv[91] = (rx_frame.data.u8[1] * 20);
            }
            if (rx_frame.data.u8[2] > 4) {  // Perform extra checks
              cellvoltages_mv[92] = (rx_frame.data.u8[2] * 20);
            }
            if (rx_frame.data.u8[3] > 4) {  // Perform extra checks
              cellvoltages_mv[93] = (rx_frame.data.u8[3] * 20);
            }
            if (rx_frame.data.u8[4] > 4) {  // Perform extra checks
              cellvoltages_mv[94] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              cellvoltages_mv[95] = (rx_frame.data.u8[5] * 20);
            }
          } else if (pid_reply == POLL_GROUP_5) {  // Data only valid on 98S
            if (rx_frame.data.u8[4] > 4) {         // Perform extra checks
              cellvoltages_mv[96] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              cellvoltages_mv[97] = (rx_frame.data.u8[5] * 20);
            }
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            cumulative_energy_charged_kWh =
                ((rx_frame.data.u8[2] << 16) | (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
            cumulative_energy_discharged_HIGH_BYTE = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          } else if (pid_reply == POLL_GROUP_5) {
            //We have read all cells, check that content is valid:
            for (uint8_t i = 85; i < 97; ++i) {
              if (cellvoltages_mv[i] < 300) {  // Zero the value if it's below 300
                cellvoltages_mv[i] = 0;        // Some packs incorrectly report the last unpopulated cells as 20-60mV
              }
            }
            //Map all cell voltages to the global array
            memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages_mv, 98 * sizeof(uint16_t));
            //Update number of cells
            update_number_of_cells();
          }
          break;
        case 0x27:  //Seventh datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            cumulative_energy_discharged_kWh = ((cumulative_energy_discharged_HIGH_BYTE << 8) | rx_frame.data.u8[1]);
            powered_on_total_time = ((rx_frame.data.u8[2] << 24) | (rx_frame.data.u8[3] << 16) |
                                     (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (pid_reply == POLL_GROUP_1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];
            isolation_resistance_kOhm = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          }
          break;
        default:
          break;
      }

      break;
    default:
      break;
  }
}

void KiaHyundai64Battery::transmit_can(unsigned long currentMillis) {

  if (!startedUp) {
    return;  // Don't send any CAN messages towards battery until it has started up
  }

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if ((contactor_closing_allowed == nullptr || *contactor_closing_allowed) &&
        datalayer.system.status.inverter_allows_contactor_closing) {
      transmit_can_frame(&KIA64_553);
      transmit_can_frame(&KIA64_57F);
      transmit_can_frame(&KIA64_2A1);
    }
  }

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    if ((contactor_closing_allowed == nullptr || *contactor_closing_allowed) &&
        datalayer.system.status.inverter_allows_contactor_closing) {

      switch (counter_200) {
        case 0:
          KIA_HYUNDAI_200.data.u8[5] = 0x17;
          ++counter_200;
          break;
        case 1:
          KIA_HYUNDAI_200.data.u8[5] = 0x57;
          ++counter_200;
          break;
        case 2:
          KIA_HYUNDAI_200.data.u8[5] = 0x97;
          ++counter_200;
          break;
        case 3:
          KIA_HYUNDAI_200.data.u8[5] = 0xD7;
          ++counter_200;
          break;
        case 4:
          KIA_HYUNDAI_200.data.u8[3] = 0x10;
          KIA_HYUNDAI_200.data.u8[5] = 0xFF;
          ++counter_200;
          break;
        case 5:
          KIA_HYUNDAI_200.data.u8[5] = 0x3B;
          ++counter_200;
          break;
        case 6:
          KIA_HYUNDAI_200.data.u8[5] = 0x7B;
          ++counter_200;
          break;
        case 7:
          KIA_HYUNDAI_200.data.u8[5] = 0xBB;
          ++counter_200;
          break;
        case 8:
          KIA_HYUNDAI_200.data.u8[5] = 0xFB;
          counter_200 = 5;
          break;
      }

      transmit_can_frame(&KIA_HYUNDAI_200);
      transmit_can_frame(&KIA_HYUNDAI_523);
      transmit_can_frame(&KIA_HYUNDAI_524);
    }
  }
}

void KiaHyundai64Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;  //Start with 98S value. Precised later
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;  //Start with 90S value. Precised later
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
