#include "KIA-HYUNDAI-HYBRID-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/* TODO: 
- The HEV battery seems to turn off after 1 minute of use. When this happens SOC% stops updating.
- We need to figure out how to keep the BMS alive. Most likely we need to send a specific CAN message
*/

static uint8_t CalculateCRC8(const CAN_frame& frame) {
  uint8_t crc = 0x00;

  for (uint8_t i = 0; i < 8; i++) {
    crc ^= frame.data.u8[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x01;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

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

  if (battery_voltage > 3000) {
    // If total pack voltage is above 300V, we can confirm that we are on 96S 8.9kWh PHEV battery
    datalayer.battery.info.number_of_cells = 96;
    datalayer.battery.info.total_capacity_Wh = 8900;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_PHEV_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_PHEV_DV;
  }

  if (battery_voltage < 2400) {
    // If total pack voltage is under 240V, we can confirm that we are on 56S 1.56kWh HEV battery
    datalayer.battery.info.number_of_cells = 56;
    datalayer.battery.info.total_capacity_Wh = 1560;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_HEV_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_HEV_DV;
  }

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mv, 96 * sizeof(uint16_t));

  if (interlock_missing) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
}

void KiaHyundaiHybridBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x5F1:
      break;
    case 0x51E:
      break;
    case 0x588:
      break;
    case 0x5AE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

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
            transmit_can_frame(&KIA_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:                      //First frame in PID group
          if (poll_data_pid == 1) {     // 21 01
            SOC = rx_frame.data.u8[1];  // 0xBC = 188 /2 = 94%
            available_charge_power = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            available_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
            battery_current_high_byte = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {  //21 02
            cellvoltages_mv[0] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[6] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {  //21 03
            cellvoltages_mv[32] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[37] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[38] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {  //21 04
            cellvoltages_mv[64] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[65] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[66] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[67] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[68] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[69] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[70] = (rx_frame.data.u8[7] * 20);
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
            cellvoltages_mv[7] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[13] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[39] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[43] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[45] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[71] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[72] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[73] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[74] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[75] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[76] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[77] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            max_cell_voltage_mv = rx_frame.data.u8[6] * 20;
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[14] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[20] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[46] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[49] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[52] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[78] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[79] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[80] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[81] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[82] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[83] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[84] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            min_cell_voltage_mv = rx_frame.data.u8[1] * 20;
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[21] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[27] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[53] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[55] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[59] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[85] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[86] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[87] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[88] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[89] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[90] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[91] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            SOC_display = rx_frame.data.u8[7];  //0x26 = 38%
          }
          break;
        case 0x25:                   //Fifth datarow in PID group
          if (poll_data_pid == 1) {  //PHEV: 25 	59 	29 	0 	1 	59 	60 	0

          } else if (poll_data_pid == 2) {
            cellvoltages_mv[28] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[31] = (rx_frame.data.u8[4] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[60] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[61] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[62] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[63] = (rx_frame.data.u8[4] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[92] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[93] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[94] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[95] = (rx_frame.data.u8[4] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x26:                   //Sixth datarow in PID group
          if (poll_data_pid == 1) {  //PHEV: 26 	0 	77 	C8 	0 	0 	73 	ED
          }
          break;
        case 0x27:                   //Seventh datarow in PID group
          if (poll_data_pid == 1) {  //PHEV: 27 	0 	78 	A3 	47 	6D 	2 	FF
          }
          break;
        case 0x28:                   //Eighth datarow in PID group
          if (poll_data_pid == 1) {  //PHEV: 28 	7F 	FF 	7F 	FF 	3 	E8 	0
          }
          break;
      }
      break;
    default:
      break;
  }
}

void KiaHyundaiHybridBattery::transmit_can(unsigned long currentMillis) {

  //Send 10ms CAN message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    KIA_200.data.u8[6] = (counter_200 & 0x0F) << 1;

    // CRC8 – Santa Fe style (byte 7)
    KIA_200.data.u8[7] = 0x00;
    KIA_200.data.u8[7] = CalculateCRC8(KIA_200);

    transmit_can_frame(&KIA_200);

    counter_200++;
    if (counter_200 > 0x0F) {
      counter_200 = 0;
    }

    KIA_2A1.data.u8[0] = 0x09;
    transmit_can_frame(&KIA_2A1);

    KIA_2F0.data.u8[0] = 0x0B;
    transmit_can_frame(&KIA_2F0);
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&KIA_523);
  }

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //PID data is polled after last message sent from battery:
    if (poll_data_pid >= 5) {  //polling one of 5 PIDs at 100ms, resolution = 500ms
      poll_data_pid = 0;
    }
    poll_data_pid++;
    if (poll_data_pid == 1) {
      KIA_7E4.data.u8[2] = 0x01;
      KIA_7E4.data.u8[3] = 0x00;
      transmit_can_frame(&KIA_7E4);
    } else if (poll_data_pid == 2) {
      KIA_7E4.data.u8[2] = 0x02;
      transmit_can_frame(&KIA_7E4);
    } else if (poll_data_pid == 3) {
      KIA_7E4.data.u8[2] = 0x03;
      transmit_can_frame(&KIA_7E4);
    } else if (poll_data_pid == 4) {
      KIA_7E4.data.u8[2] = 0x04;
      transmit_can_frame(&KIA_7E4);
    } else if (poll_data_pid == 5) {
      KIA_7E4.data.u8[2] = 0x05;
      KIA_7E4.data.u8[3] = 0x04;
      transmit_can_frame(&KIA_7E4);
    }
  }
}

void KiaHyundaiHybridBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 56;                              // Startup in 56S mode, switch later
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_PHEV_DV;  //Startup with widest range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_HEV_DV;   //Autodetect later
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
