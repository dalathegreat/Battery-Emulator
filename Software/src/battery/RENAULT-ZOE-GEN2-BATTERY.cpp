#include "RENAULT-ZOE-GEN2-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "../include.h"

/* TODO
- Add //NVROL Reset
- Add //Enable temporisation before sleep (see ljames28 repo)

"If the pack is in a state where it is confused about the time, you may need to reset it's NVROL memory. 
However, if the power is later power cycled, it will revert back to his previous confused state. 
Therefore, after resetting the NVROL you must enable "temporisation before sleep", and then stop streaming 373. 
It will then save the data and go to sleep. When the pack is confused, the state of charge may reset back to incorrect value 
every time the power is reset which can be dangerous. In this state, the voltage will still be accurate"
*/

/* Information in this file is based on:
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe_ph2_obd/src/vehicle_renaultzoe_ph2_obd.cpp
https://github.com/ljames28/Renault-Zoe-PH2-ZE50-Canbus-LBC-Information?tab=readme-ov-file
https://github.com/fesch/CanZE/tree/master/app/src/main/assets/ZOE_Ph2
*/

void RenaultZoeGen2Battery::update_values() {

  datalayer.battery.status.soh_pptt = battery_soh;

  if (battery_soc >= 300) {
    datalayer.battery.status.real_soc = battery_soc - 300;
  } else {
    datalayer.battery.status.real_soc = 0;
  }

  datalayer.battery.status.voltage_dV = battery_pack_voltage;

  datalayer.battery.status.current_dA = ((battery_current - 32640) * 0.3125);

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = battery_max_available * 10;

  datalayer.battery.status.max_charge_power_W = battery_max_generated * 10;

  //Temperatures and voltages update at slow rate. Only publish new values once both have been sampled to avoid events
  if ((battery_min_temp != 920) && (battery_max_temp != 920)) {
    datalayer.battery.status.temperature_min_dC = ((battery_min_temp - 640) * 0.625);
    datalayer.battery.status.temperature_max_dC = ((battery_max_temp - 640) * 0.625);
  }

  if ((battery_min_cell_voltage != 3700) && (battery_max_cell_voltage != 3700)) {
    datalayer.battery.status.cell_min_voltage_mV = (battery_min_cell_voltage * 0.976563);
    datalayer.battery.status.cell_max_voltage_mV = (battery_max_cell_voltage * 0.976563);
  }

  if (battery_12v < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, battery_12v);
  }

  // Update webserver datalayer
  datalayer_extended.zoePH2.battery_soc = battery_soc;
  datalayer_extended.zoePH2.battery_usable_soc = battery_usable_soc;
  datalayer_extended.zoePH2.battery_soh = battery_soh;
  datalayer_extended.zoePH2.battery_pack_voltage = battery_pack_voltage;
  datalayer_extended.zoePH2.battery_max_cell_voltage = battery_max_cell_voltage;
  datalayer_extended.zoePH2.battery_min_cell_voltage = battery_min_cell_voltage;
  datalayer_extended.zoePH2.battery_12v = battery_12v;
  datalayer_extended.zoePH2.battery_avg_temp = battery_avg_temp;
  datalayer_extended.zoePH2.battery_min_temp = battery_min_temp;
  datalayer_extended.zoePH2.battery_max_temp = battery_max_temp;
  datalayer_extended.zoePH2.battery_max_power = battery_max_power;
  datalayer_extended.zoePH2.battery_interlock = battery_interlock;
  datalayer_extended.zoePH2.battery_kwh = battery_kwh;
  datalayer_extended.zoePH2.battery_current = battery_current;
  datalayer_extended.zoePH2.battery_current_offset = battery_current_offset;
  datalayer_extended.zoePH2.battery_max_generated = battery_max_generated;
  datalayer_extended.zoePH2.battery_max_available = battery_max_available;
  datalayer_extended.zoePH2.battery_current_voltage = battery_current_voltage;
  datalayer_extended.zoePH2.battery_charging_status = battery_charging_status;
  datalayer_extended.zoePH2.battery_remaining_charge = battery_remaining_charge;
  datalayer_extended.zoePH2.battery_balance_capacity_total = battery_balance_capacity_total;
  datalayer_extended.zoePH2.battery_balance_time_total = battery_balance_time_total;
  datalayer_extended.zoePH2.battery_balance_capacity_sleep = battery_balance_capacity_sleep;
  datalayer_extended.zoePH2.battery_balance_time_sleep = battery_balance_time_sleep;
  datalayer_extended.zoePH2.battery_balance_capacity_wake = battery_balance_capacity_wake;
  datalayer_extended.zoePH2.battery_balance_time_wake = battery_balance_time_wake;
  datalayer_extended.zoePH2.battery_bms_state = battery_bms_state;
  datalayer_extended.zoePH2.battery_energy_complete = battery_energy_complete;
  datalayer_extended.zoePH2.battery_energy_partial = battery_energy_partial;
  datalayer_extended.zoePH2.battery_slave_failures = battery_slave_failures;
  datalayer_extended.zoePH2.battery_mileage = battery_mileage;
  datalayer_extended.zoePH2.battery_fan_speed = battery_fan_speed;
  datalayer_extended.zoePH2.battery_fan_period = battery_fan_period;
  datalayer_extended.zoePH2.battery_fan_control = battery_fan_control;
  datalayer_extended.zoePH2.battery_fan_duty = battery_fan_duty;
  datalayer_extended.zoePH2.battery_temporisation = battery_temporisation;
  datalayer_extended.zoePH2.battery_time = battery_time;
  datalayer_extended.zoePH2.battery_pack_time = battery_pack_time;
  datalayer_extended.zoePH2.battery_soc_min = battery_soc_min;
  datalayer_extended.zoePH2.battery_soc_max = battery_soc_max;
}

void RenaultZoeGen2Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x18DAF1DB:  // LBC Reply from active polling

      if (rx_frame.data.u8[0] == 0x10) {  //First frame of a group
        transmit_can_frame(&ZOE_POLL_FLOW_CONTROL, can_config.battery);
        //frame 2 & 3 contains which PID is sent
        reply_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      if (rx_frame.data.u8[0] < 0x10) {  //One line responses
        reply_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      }

      switch (reply_poll) {
        case POLL_SOC:
          battery_soc = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_USABLE_SOC:
          battery_usable_soc = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOH:
          battery_soh = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_PACK_VOLTAGE:
          battery_pack_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_CELL_VOLTAGE:
          temporary_variable = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          if (temporary_variable > 500) {  //Disregard messages with value unavailable
            battery_max_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          }
          break;
        case POLL_MIN_CELL_VOLTAGE:
          temporary_variable = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          if (temporary_variable > 500) {  //Disregard messages with value unavailable
            battery_min_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          }
          break;
        case POLL_12V:
          battery_12v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_AVG_TEMP:
          battery_avg_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_TEMP:
          battery_min_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_TEMP:
          battery_max_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_POWER:
          battery_max_power = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_INTERLOCK:
          battery_interlock = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_KWH:
          battery_kwh = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT:
          battery_current = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT_OFFSET:
          battery_current_offset = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_GENERATED:
          battery_max_generated = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_AVAILABLE:
          battery_max_available = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT_VOLTAGE:
          battery_current_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CHARGING_STATUS:
          battery_charging_status = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_REMAINING_CHARGE:
          battery_remaining_charge = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_TOTAL:
          battery_balance_capacity_total = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_TOTAL:
          battery_balance_time_total = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_SLEEP:
          battery_balance_capacity_sleep = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_SLEEP:
          battery_balance_time_sleep = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_WAKE:
          battery_balance_capacity_wake = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_WAKE:
          battery_balance_time_wake = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BMS_STATE:
          battery_bms_state = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_SWITCHES:
          if (rx_frame.data.u8[0] == 0x23) {
            battery_balancing_shunts[0] = (rx_frame.data.u8[4] & 0x80) >> 7;
            battery_balancing_shunts[1] = (rx_frame.data.u8[4] & 0x40) >> 6;
            battery_balancing_shunts[2] = (rx_frame.data.u8[4] & 0x20) >> 5;
            battery_balancing_shunts[3] = (rx_frame.data.u8[4] & 0x10) >> 4;
            battery_balancing_shunts[4] = (rx_frame.data.u8[4] & 0x08) >> 3;
            battery_balancing_shunts[5] = (rx_frame.data.u8[4] & 0x04) >> 2;
            battery_balancing_shunts[6] = (rx_frame.data.u8[4] & 0x02) >> 1;
            battery_balancing_shunts[7] = (rx_frame.data.u8[4] & 0x01);

            battery_balancing_shunts[8] = (rx_frame.data.u8[5] & 0x80) >> 7;
            battery_balancing_shunts[9] = (rx_frame.data.u8[5] & 0x40) >> 6;
            battery_balancing_shunts[10] = (rx_frame.data.u8[5] & 0x20) >> 5;
            battery_balancing_shunts[11] = (rx_frame.data.u8[5] & 0x10) >> 4;
            battery_balancing_shunts[12] = (rx_frame.data.u8[5] & 0x08) >> 3;
            battery_balancing_shunts[13] = (rx_frame.data.u8[5] & 0x04) >> 2;
            battery_balancing_shunts[14] = (rx_frame.data.u8[5] & 0x02) >> 1;
            battery_balancing_shunts[15] = (rx_frame.data.u8[5] & 0x01);

            battery_balancing_shunts[16] = (rx_frame.data.u8[6] & 0x80) >> 7;
            battery_balancing_shunts[17] = (rx_frame.data.u8[6] & 0x40) >> 6;
            battery_balancing_shunts[18] = (rx_frame.data.u8[6] & 0x20) >> 5;
            battery_balancing_shunts[19] = (rx_frame.data.u8[6] & 0x10) >> 4;
            battery_balancing_shunts[20] = (rx_frame.data.u8[6] & 0x08) >> 3;
            battery_balancing_shunts[21] = (rx_frame.data.u8[6] & 0x04) >> 2;
            battery_balancing_shunts[22] = (rx_frame.data.u8[6] & 0x02) >> 1;
            battery_balancing_shunts[23] = (rx_frame.data.u8[6] & 0x01);

            battery_balancing_shunts[24] = (rx_frame.data.u8[7] & 0x80) >> 7;
            battery_balancing_shunts[25] = (rx_frame.data.u8[7] & 0x40) >> 6;
            battery_balancing_shunts[26] = (rx_frame.data.u8[7] & 0x20) >> 5;
            battery_balancing_shunts[27] = (rx_frame.data.u8[7] & 0x10) >> 4;
            battery_balancing_shunts[28] = (rx_frame.data.u8[7] & 0x08) >> 3;
            battery_balancing_shunts[29] = (rx_frame.data.u8[7] & 0x04) >> 2;
            battery_balancing_shunts[30] = (rx_frame.data.u8[7] & 0x02) >> 1;
            battery_balancing_shunts[31] = (rx_frame.data.u8[7] & 0x01);
          }
          if (rx_frame.data.u8[0] == 0x24) {
            battery_balancing_shunts[32] = (rx_frame.data.u8[1] & 0x80) >> 7;
            battery_balancing_shunts[33] = (rx_frame.data.u8[1] & 0x40) >> 6;
            battery_balancing_shunts[34] = (rx_frame.data.u8[1] & 0x20) >> 5;
            battery_balancing_shunts[35] = (rx_frame.data.u8[1] & 0x10) >> 4;
            battery_balancing_shunts[36] = (rx_frame.data.u8[1] & 0x08) >> 3;
            battery_balancing_shunts[37] = (rx_frame.data.u8[1] & 0x04) >> 2;
            battery_balancing_shunts[38] = (rx_frame.data.u8[1] & 0x02) >> 1;
            battery_balancing_shunts[39] = (rx_frame.data.u8[1] & 0x01);

            battery_balancing_shunts[40] = (rx_frame.data.u8[2] & 0x80) >> 7;
            battery_balancing_shunts[41] = (rx_frame.data.u8[2] & 0x40) >> 6;
            battery_balancing_shunts[42] = (rx_frame.data.u8[2] & 0x20) >> 5;
            battery_balancing_shunts[43] = (rx_frame.data.u8[2] & 0x10) >> 4;
            battery_balancing_shunts[44] = (rx_frame.data.u8[2] & 0x08) >> 3;
            battery_balancing_shunts[45] = (rx_frame.data.u8[2] & 0x04) >> 2;
            battery_balancing_shunts[46] = (rx_frame.data.u8[2] & 0x02) >> 1;
            battery_balancing_shunts[47] = (rx_frame.data.u8[2] & 0x01);

            battery_balancing_shunts[48] = (rx_frame.data.u8[3] & 0x80) >> 7;
            battery_balancing_shunts[49] = (rx_frame.data.u8[3] & 0x40) >> 6;
            battery_balancing_shunts[50] = (rx_frame.data.u8[3] & 0x20) >> 5;
            battery_balancing_shunts[51] = (rx_frame.data.u8[3] & 0x10) >> 4;
            battery_balancing_shunts[52] = (rx_frame.data.u8[3] & 0x08) >> 3;
            battery_balancing_shunts[53] = (rx_frame.data.u8[3] & 0x04) >> 2;
            battery_balancing_shunts[54] = (rx_frame.data.u8[3] & 0x02) >> 1;
            battery_balancing_shunts[55] = (rx_frame.data.u8[3] & 0x01);

            battery_balancing_shunts[56] = (rx_frame.data.u8[4] & 0x80) >> 7;
            battery_balancing_shunts[57] = (rx_frame.data.u8[4] & 0x40) >> 6;
            battery_balancing_shunts[58] = (rx_frame.data.u8[4] & 0x20) >> 5;
            battery_balancing_shunts[59] = (rx_frame.data.u8[4] & 0x10) >> 4;
            battery_balancing_shunts[60] = (rx_frame.data.u8[4] & 0x08) >> 3;
            battery_balancing_shunts[61] = (rx_frame.data.u8[4] & 0x04) >> 2;
            battery_balancing_shunts[62] = (rx_frame.data.u8[4] & 0x02) >> 1;
            battery_balancing_shunts[63] = (rx_frame.data.u8[4] & 0x01);

            battery_balancing_shunts[64] = (rx_frame.data.u8[5] & 0x80) >> 7;
            battery_balancing_shunts[65] = (rx_frame.data.u8[5] & 0x40) >> 6;
            battery_balancing_shunts[66] = (rx_frame.data.u8[5] & 0x20) >> 5;
            battery_balancing_shunts[67] = (rx_frame.data.u8[5] & 0x10) >> 4;
            battery_balancing_shunts[68] = (rx_frame.data.u8[5] & 0x08) >> 3;
            battery_balancing_shunts[69] = (rx_frame.data.u8[5] & 0x04) >> 2;
            battery_balancing_shunts[70] = (rx_frame.data.u8[5] & 0x02) >> 1;
            battery_balancing_shunts[71] = (rx_frame.data.u8[5] & 0x01);

            battery_balancing_shunts[72] = (rx_frame.data.u8[6] & 0x80) >> 7;
            battery_balancing_shunts[73] = (rx_frame.data.u8[6] & 0x40) >> 6;
            battery_balancing_shunts[74] = (rx_frame.data.u8[6] & 0x20) >> 5;
            battery_balancing_shunts[75] = (rx_frame.data.u8[6] & 0x10) >> 4;
            battery_balancing_shunts[76] = (rx_frame.data.u8[6] & 0x08) >> 3;
            battery_balancing_shunts[77] = (rx_frame.data.u8[6] & 0x04) >> 2;
            battery_balancing_shunts[78] = (rx_frame.data.u8[6] & 0x02) >> 1;
            battery_balancing_shunts[79] = (rx_frame.data.u8[6] & 0x01);

            battery_balancing_shunts[80] = (rx_frame.data.u8[7] & 0x80) >> 7;
            battery_balancing_shunts[81] = (rx_frame.data.u8[7] & 0x40) >> 6;
            battery_balancing_shunts[82] = (rx_frame.data.u8[7] & 0x20) >> 5;
            battery_balancing_shunts[83] = (rx_frame.data.u8[7] & 0x10) >> 4;
            battery_balancing_shunts[84] = (rx_frame.data.u8[7] & 0x08) >> 3;
            battery_balancing_shunts[85] = (rx_frame.data.u8[7] & 0x04) >> 2;
            battery_balancing_shunts[86] = (rx_frame.data.u8[7] & 0x02) >> 1;
            battery_balancing_shunts[87] = (rx_frame.data.u8[7] & 0x01);
          }
          if (rx_frame.data.u8[0] == 0x25) {
            battery_balancing_shunts[88] = (rx_frame.data.u8[1] & 0x80) >> 7;
            battery_balancing_shunts[89] = (rx_frame.data.u8[1] & 0x40) >> 6;
            battery_balancing_shunts[90] = (rx_frame.data.u8[1] & 0x20) >> 5;
            battery_balancing_shunts[91] = (rx_frame.data.u8[1] & 0x10) >> 4;
            battery_balancing_shunts[92] = (rx_frame.data.u8[1] & 0x08) >> 3;
            battery_balancing_shunts[93] = (rx_frame.data.u8[1] & 0x04) >> 2;
            battery_balancing_shunts[94] = (rx_frame.data.u8[1] & 0x02) >> 1;
            battery_balancing_shunts[95] = (rx_frame.data.u8[1] & 0x01);

            memcpy(datalayer.battery.status.cell_balancing_status, battery_balancing_shunts, 96 * sizeof(bool));
          }
          break;
        case POLL_ENERGY_COMPLETE:
          battery_energy_complete = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_ENERGY_PARTIAL:
          battery_energy_partial = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SLAVE_FAILURES:
          battery_slave_failures = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MILEAGE:
          battery_mileage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_SPEED:
          battery_fan_speed = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_PERIOD:
          battery_fan_period = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_CONTROL:
          battery_fan_control = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_DUTY:
          battery_fan_duty = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_TEMPORISATION:
          battery_temporisation = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_TIME:
          battery_time = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_PACK_TIME:
          battery_pack_time = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_MIN:
          battery_soc_min = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_MAX:
          battery_soc_max = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_0:
          datalayer.battery.status.cell_voltages_mV[0] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_1:
          datalayer.battery.status.cell_voltages_mV[1] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_2:
          datalayer.battery.status.cell_voltages_mV[2] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_3:
          datalayer.battery.status.cell_voltages_mV[3] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_4:
          datalayer.battery.status.cell_voltages_mV[4] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_5:
          datalayer.battery.status.cell_voltages_mV[5] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_6:
          datalayer.battery.status.cell_voltages_mV[6] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_7:
          datalayer.battery.status.cell_voltages_mV[7] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_8:
          datalayer.battery.status.cell_voltages_mV[8] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_9:
          datalayer.battery.status.cell_voltages_mV[9] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_10:
          datalayer.battery.status.cell_voltages_mV[10] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_11:
          datalayer.battery.status.cell_voltages_mV[11] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_12:
          datalayer.battery.status.cell_voltages_mV[12] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_13:
          datalayer.battery.status.cell_voltages_mV[13] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_14:
          datalayer.battery.status.cell_voltages_mV[14] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_15:
          datalayer.battery.status.cell_voltages_mV[15] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_16:
          datalayer.battery.status.cell_voltages_mV[16] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_17:
          datalayer.battery.status.cell_voltages_mV[17] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_18:
          datalayer.battery.status.cell_voltages_mV[18] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_19:
          datalayer.battery.status.cell_voltages_mV[19] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_20:
          datalayer.battery.status.cell_voltages_mV[20] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_21:
          datalayer.battery.status.cell_voltages_mV[21] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_22:
          datalayer.battery.status.cell_voltages_mV[22] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_23:
          datalayer.battery.status.cell_voltages_mV[23] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_24:
          datalayer.battery.status.cell_voltages_mV[24] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_25:
          datalayer.battery.status.cell_voltages_mV[25] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_26:
          datalayer.battery.status.cell_voltages_mV[26] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_27:
          datalayer.battery.status.cell_voltages_mV[27] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_28:
          datalayer.battery.status.cell_voltages_mV[28] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_29:
          datalayer.battery.status.cell_voltages_mV[29] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_30:
          datalayer.battery.status.cell_voltages_mV[30] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_31:
          datalayer.battery.status.cell_voltages_mV[31] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_32:
          datalayer.battery.status.cell_voltages_mV[32] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_33:
          datalayer.battery.status.cell_voltages_mV[33] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_34:
          datalayer.battery.status.cell_voltages_mV[34] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_35:
          datalayer.battery.status.cell_voltages_mV[35] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_36:
          datalayer.battery.status.cell_voltages_mV[36] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_37:
          datalayer.battery.status.cell_voltages_mV[37] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_38:
          datalayer.battery.status.cell_voltages_mV[38] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_39:
          datalayer.battery.status.cell_voltages_mV[39] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_40:
          datalayer.battery.status.cell_voltages_mV[40] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_41:
          datalayer.battery.status.cell_voltages_mV[41] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_42:
          datalayer.battery.status.cell_voltages_mV[42] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_43:
          datalayer.battery.status.cell_voltages_mV[43] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_44:
          datalayer.battery.status.cell_voltages_mV[44] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_45:
          datalayer.battery.status.cell_voltages_mV[45] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_46:
          datalayer.battery.status.cell_voltages_mV[46] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_47:
          datalayer.battery.status.cell_voltages_mV[47] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_48:
          datalayer.battery.status.cell_voltages_mV[48] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_49:
          datalayer.battery.status.cell_voltages_mV[49] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_50:
          datalayer.battery.status.cell_voltages_mV[50] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_51:
          datalayer.battery.status.cell_voltages_mV[51] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_52:
          datalayer.battery.status.cell_voltages_mV[52] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_53:
          datalayer.battery.status.cell_voltages_mV[53] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_54:
          datalayer.battery.status.cell_voltages_mV[54] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_55:
          datalayer.battery.status.cell_voltages_mV[55] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_56:
          datalayer.battery.status.cell_voltages_mV[56] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_57:
          datalayer.battery.status.cell_voltages_mV[57] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_58:
          datalayer.battery.status.cell_voltages_mV[58] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_59:
          datalayer.battery.status.cell_voltages_mV[59] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_60:
          datalayer.battery.status.cell_voltages_mV[60] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_61:
          datalayer.battery.status.cell_voltages_mV[61] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_62:
          datalayer.battery.status.cell_voltages_mV[62] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_63:
          datalayer.battery.status.cell_voltages_mV[63] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_64:
          datalayer.battery.status.cell_voltages_mV[64] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_65:
          datalayer.battery.status.cell_voltages_mV[65] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_66:
          datalayer.battery.status.cell_voltages_mV[66] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_67:
          datalayer.battery.status.cell_voltages_mV[67] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_68:
          datalayer.battery.status.cell_voltages_mV[68] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_69:
          datalayer.battery.status.cell_voltages_mV[69] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_70:
          datalayer.battery.status.cell_voltages_mV[70] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_71:
          datalayer.battery.status.cell_voltages_mV[71] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_72:
          datalayer.battery.status.cell_voltages_mV[72] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_73:
          datalayer.battery.status.cell_voltages_mV[73] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_74:
          datalayer.battery.status.cell_voltages_mV[74] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_75:
          datalayer.battery.status.cell_voltages_mV[75] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_76:
          datalayer.battery.status.cell_voltages_mV[76] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_77:
          datalayer.battery.status.cell_voltages_mV[77] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_78:
          datalayer.battery.status.cell_voltages_mV[78] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_79:
          datalayer.battery.status.cell_voltages_mV[79] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_80:
          datalayer.battery.status.cell_voltages_mV[80] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_81:
          datalayer.battery.status.cell_voltages_mV[81] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_82:
          datalayer.battery.status.cell_voltages_mV[82] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_83:
          datalayer.battery.status.cell_voltages_mV[83] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_84:
          datalayer.battery.status.cell_voltages_mV[84] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_85:
          datalayer.battery.status.cell_voltages_mV[85] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_86:
          datalayer.battery.status.cell_voltages_mV[86] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_87:
          datalayer.battery.status.cell_voltages_mV[87] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_88:
          datalayer.battery.status.cell_voltages_mV[88] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_89:
          datalayer.battery.status.cell_voltages_mV[89] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_90:
          datalayer.battery.status.cell_voltages_mV[90] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_91:
          datalayer.battery.status.cell_voltages_mV[91] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_92:
          datalayer.battery.status.cell_voltages_mV[92] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_93:
          datalayer.battery.status.cell_voltages_mV[93] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_94:
          datalayer.battery.status.cell_voltages_mV[94] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_95:
          datalayer.battery.status.cell_voltages_mV[95] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        default:  // Unknown reply
          break;
      }
      break;
    default:
      break;
  }
}

void RenaultZoeGen2Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer_extended.zoePH2.UserRequestNVROLReset) {
    // Send NVROL reset frames
    transmit_reset_nvrol_frames();
  } else {
    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      /* FIXME: remove if not needed
      if ((counter_373 / 5) % 2 == 0) {  // Alternate every 5 messages between these two
        ZOE_373.data.u8[2] = 0xB2;
        ZOE_373.data.u8[3] = 0xB2;
      } else {
        ZOE_373.data.u8[2] = 0x5D;
        ZOE_373.data.u8[3] = 0x5D;
      }
      counter_373 = (counter_373 + 1) % 10;
      */

      transmit_can_frame(&ZOE_373, can_config.battery);
      transmit_can_frame_376();
    }

    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;

      // Update current poll from the array
      currentpoll = poll_commands[poll_index];
      poll_index = (poll_index + 1) % 163;

      ZOE_POLL_18DADBF1.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
      ZOE_POLL_18DADBF1.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

      transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
    }

    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      // Time in seconds emulated
      ZOE_376_time_now_s++;  // Increment by 1 second
    }
  }
}

void RenaultZoeGen2Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

void RenaultZoeGen2Battery::transmit_can_frame_376(void) {
  unsigned int secondsSinceProduction = ZOE_376_time_now_s - kProductionTimestamp_s;
  float minutesSinceProduction = (float)secondsSinceProduction / 60.0;
  float yearUnfloored = minutesSinceProduction / 255.0 / 255.0;
  int yearSeg = floor(yearUnfloored);
  float remainderYears = yearUnfloored - yearSeg;
  float remainderHoursUnfloored = (remainderYears * 255.0);
  int hourSeg = floor(remainderHoursUnfloored);
  float remainderHours = remainderHoursUnfloored - hourSeg;
  int minuteSeg = floor(remainderHours * 255.0);

  ZOE_376.data.u8[0] = yearSeg;
  ZOE_376.data.u8[1] = hourSeg;
  ZOE_376.data.u8[2] = minuteSeg;
  ZOE_376.data.u8[3] = yearSeg;
  ZOE_376.data.u8[4] = hourSeg;
  ZOE_376.data.u8[5] = minuteSeg;

  transmit_can_frame(&ZOE_376, can_config.battery);
}

void RenaultZoeGen2Battery::transmit_reset_nvrol_frames(void) {
  switch (NVROLstateMachine) {
    case 0:
      startTimeNVROL = millis();
      // NVROL reset, part 1: send 0x021003AAAAAAAAAA
      ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
      transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
      NVROLstateMachine = 1;
      break;
    case 1:  // wait 100 ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // NVROL reset, part 2: send 0x043101B00900AAAA
        ZOE_POLL_18DADBF1.data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 2;
      }
      break;
    case 2:  // wait 1 s
      if ((millis() - startTimeNVROL) > INTERVAL_1_S) {
        // Enable temporisation before sleep, part 1: send 0x021003AAAAAAAAAA
        ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 3;
      }
      break;
    case 3:  //Wait 100ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // Enable temporisation before sleep, part 2: send 0x042E928101AAAAAA
        ZOE_POLL_18DADBF1.data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
        // Set data back to init values, we are done with the ZOE_POLL_18DADBF1 frame
        ZOE_POLL_18DADBF1.data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00};
        poll_index = 0;
        NVROLstateMachine = 4;
      }
      break;
    case 4:  //Wait 30s
      if ((millis() - startTimeNVROL) > INTERVAL_30_S) {
        // after sleeping, set the nvrol reset flag to false, to continue normal operation of sending CAN messages
        datalayer_extended.zoePH2.UserRequestNVROLReset = false;
        // reset state machine, we are done!
        NVROLstateMachine = 0;
      }
      break;
    default:  //Something went catastrophically wrong. Reset state machine
      NVROLstateMachine = 0;
      break;
  }
}
