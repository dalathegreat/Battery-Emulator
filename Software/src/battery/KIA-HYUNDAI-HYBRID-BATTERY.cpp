#include "KIA-HYUNDAI-HYBRID-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* TODO: 
- The HEV battery seems to turn off after 1 minute of use. When this happens SOC% stops updating.
- We need to figure out how to keep the BMS alive. Most likely we need to send a specific CAN message
*/

void KiaHyundaiHybridBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = SOC * 50;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = available_discharge_power * 10;

  datalayer.battery.status.max_charge_power_W = available_charge_power * 10;

  datalayer.battery.status.temperature_min_dC = (int16_t)(battery_module_min_temperature * 10);

  datalayer.battery.status.temperature_max_dC = (int16_t)(battery_module_max_temperature * 10);

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage_mv;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage_mv;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mv, 98 * sizeof(uint16_t));

  if (interlock_missing) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
}

void KiaHyundaiHybridBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x5F1:
      break;
    case 0x51E:
      break;
    case 0x588:
      break;
    case 0x5AE:
      interlock_missing = (bool)(rx_frame.data.u8[1] & 0x02) >> 1;
      break;
    case 0x5AF:
      break;
    case 0x5AD:
      break;
    case 0x670:
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[3] == poll_data_pid) {
            transmit_can_frame(&KIA_7E4_ack, can_config.battery);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:                      //First frame in PID group
          if (poll_data_pid == 1) {     // 21 01
            SOC = rx_frame.data.u8[1];  // 0xBC = 188 /2 = 94%
            available_charge_power = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            available_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
            battery_current_high_byte = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {  //21 02
            cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {  //21 03
            cellvoltages_mv[31] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[32] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {  //21 05
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {
            battery_current = ((battery_current_high_byte << 8) | rx_frame.data.u8[1]);
            battery_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            battery_module_max_temperature = rx_frame.data.u8[4];
            battery_module_min_temperature = rx_frame.data.u8[5];
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);

          } else if (poll_data_pid == 3) {
            cellvoltages_mv[37] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[38] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[39] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            max_cell_voltage_mv = rx_frame.data.u8[6] * 20;
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[43] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[45] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[46] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            min_cell_voltage_mv = rx_frame.data.u8[1] * 20;
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[49] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[52] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[53] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            SOC_display = rx_frame.data.u8[7];  //0x26 = 38%
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {
            cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          break;
        case 0x27:  //Seventh datarow in PID group
          break;
        case 0x28:  //Eighth datarow in PID group
          break;
      }
      break;
    default:
      break;
  }
}

void KiaHyundaiHybridBattery::transmit_can(unsigned long currentMillis) {

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //PID data is polled after last message sent from battery:
    if (poll_data_pid >= 5) {  //polling one of 5 PIDs at 100ms, resolution = 500ms
      poll_data_pid = 0;
    }
    poll_data_pid++;
    if (poll_data_pid == 1) {
      transmit_can_frame(&KIA_7E4_id1, can_config.battery);
    } else if (poll_data_pid == 2) {
      transmit_can_frame(&KIA_7E4_id2, can_config.battery);
    } else if (poll_data_pid == 3) {
      transmit_can_frame(&KIA_7E4_id3, can_config.battery);
    } else if (poll_data_pid == 4) {

    } else if (poll_data_pid == 5) {
      transmit_can_frame(&KIA_7E4_id5, can_config.battery);
    }
  }
}

void KiaHyundaiHybridBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 56;  // HEV , TODO: Make dynamic according to HEV/PHEV
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
