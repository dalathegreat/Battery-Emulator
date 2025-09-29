#include "FORD-MACH-E-BATTERY.h"
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/logging.h"

void FordMachEBattery::update_values() {

  datalayer.battery.status.real_soc = battery_soc;

  datalayer.battery.status.soh_pptt = battery_soh * 100;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;

  maximum_cellvoltage_mV = datalayer.battery.status.cell_voltages_mV[0];
  minimum_cellvoltage_mV = datalayer.battery.status.cell_voltages_mV[0];

  // Loop through the array to find min and max cellvoltages, ignoring 0 values
  for (uint8_t i = 0; i < MAX_AMOUNT_CELLS; i++) {
    if (datalayer.battery.status.cell_voltages_mV[i] != 0) {  // Ignore unavailable values
      if (datalayer.battery.status.cell_voltages_mV[i] < minimum_cellvoltage_mV) {
        minimum_cellvoltage_mV = datalayer.battery.status.cell_voltages_mV[i];
      }
      if (datalayer.battery.status.cell_voltages_mV[i] > maximum_cellvoltage_mV) {
        maximum_cellvoltage_mV = datalayer.battery.status.cell_voltages_mV[i];
      }
    }
  }

  // Initialize highest and lowest to the first element
  maximum_temperature = cell_temperature[0];
  minimum_temperature = cell_temperature[0];

  // Iterate through the array to find the highest and lowest values
  for (uint8_t i = 1; i < 6; ++i) {
    if (cell_temperature[i] > maximum_temperature) {
      maximum_temperature = cell_temperature[i];
    }
    if (cell_temperature[i] < minimum_temperature) {
      minimum_temperature = cell_temperature[i];
    }
  }
  datalayer.battery.status.temperature_min_dC = minimum_temperature * 10;

  datalayer.battery.status.temperature_max_dC = maximum_temperature * 10;
}

void FordMachEBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x07a:           //10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x07b:  //10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x073:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24b:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24c:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_soc = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      break;
    case 0x24d:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24e:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24f:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2e4:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x444:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x458:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x47d:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_soh = rx_frame.data.u8[1];
      break;
    case 0x490:  //1s Cellvoltages
    case 0x491:
    case 0x492:
    case 0x493:
    case 0x494:
    case 0x495:
    case 0x496:
    case 0x497:
    case 0x498:
    case 0x499:
    case 0x49a:
    case 0x49b:
    case 0x49c:
    case 0x49d:
    case 0x49e:
    case 0x49f:
    case 0x4a0:
    case 0x4a1:
    case 0x4a2: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      // Calculate starting cell index: (CAN_ID - 0x490) * 5 (Ends at 0x4A2)
      const uint8_t start_index = (rx_frame.ID - 0x490) * 5;

      // Each CAN message contains 5 cellvoltages
      for (uint8_t i = 0; i < 5; i++) {
        const uint8_t byte_offset = i * 3;

        if (i % 2 == 0) {
          // Even indices: bytes 0,1 | 3,4 | 6,7
          datalayer.battery.status.cell_voltages_mV[start_index + i] =
              (((rx_frame.data.u8[byte_offset] << 4) | (rx_frame.data.u8[byte_offset + 1] >> 4)) + 1000);
        } else {
          // Odd indices: bytes 1,2 | 4,5
          datalayer.battery.status.cell_voltages_mV[start_index + i] =
              ((((rx_frame.data.u8[byte_offset] & 0x0F) << 8) | rx_frame.data.u8[byte_offset + 1]) + 1000);
        }
      }
      break;
    }
    case 0x4a3:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer.battery.status.cell_voltages_mV[95] = (rx_frame.data.u8[0] << 4) | (rx_frame.data.u8[1] >> 4);
      //Celltemperatures
      cell_temperature[0] = ((rx_frame.data.u8[2] - 40) / 2);
      cell_temperature[1] = ((rx_frame.data.u8[2] - 40) / 2);
      cell_temperature[2] = ((rx_frame.data.u8[2] - 40) / 2);
      cell_temperature[3] = ((rx_frame.data.u8[2] - 40) / 2);
      cell_temperature[4] = ((rx_frame.data.u8[2] - 40) / 2);
      cell_temperature[5] = ((rx_frame.data.u8[2] - 40) / 2);
      break;
    case 0x4a4:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4a5:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4a7:  //1s
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x46f:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void FordMachEBattery::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    transmit_can_frame(&FORD_217);
    transmit_can_frame(&FORD_442);
  }

  // Send 30ms CAN Message
  if (currentMillis - previousMillis30 >= INTERVAL_30_MS) {
    previousMillis30 = currentMillis;

    counter_30ms = (counter_30ms + 1) % 16;  // cycles 0-15

    // Byte 2: upper nibble = 0xF, lower nibble = (0xF - counter_10ms) % 16
    FORD_7D.data.u8[2] = 0xF0 | ((0xF - counter_30ms) & 0x0F);
    // Byte 3: upper nibble = counter_10ms, lower nibble = 0x0
    FORD_7D.data.u8[3] = (counter_30ms << 4) & 0xF0;

    // Byte 0: starts at 0xC0 and increments by 4 each step, wraps every 16 steps
    FORD_204.data.u8[0] = 0xC0 + (counter_30ms * 4);
    // Byte 5: starts at 0xFC and decrements by 1 each step, wraps every 16 steps
    FORD_204.data.u8[5] = 0xFC - counter_30ms;

    // Byte 4: upper nibble = counter_10ms, lower nibble = 0x0
    FORD_4B0.data.u8[4] = (counter_30ms << 4) & 0xF0;
    // Byte 7: upper nibble = 0xF, lower nibble = (0xF - counter_10ms) % 16
    FORD_4B0.data.u8[7] = 0xF0 | ((0xF - counter_30ms) & 0x0F);

    // Byte 2: starts at 0xC0 and increments by 4 each step, wraps every 16 steps
    FORD_415.data.u8[2] = 0xC0 + (counter_30ms * 4);
    // Byte 3: starts at 0xFC and decrements by 1 each step, wraps every 16 steps
    FORD_415.data.u8[3] = 0xFC - counter_30ms;

    //TODO: handle FORD_4C properly very odd looping

    transmit_can_frame(&FORD_77);
    transmit_can_frame(&FORD_7D);
    transmit_can_frame(&FORD_167);
    transmit_can_frame(&FORD_48F);  //Only sent in AC charging logs!
    transmit_can_frame(&FORD_204);
    transmit_can_frame(&FORD_4B0);
    transmit_can_frame(&FORD_47);
    transmit_can_frame(&FORD_230);
    transmit_can_frame(&FORD_415);
    transmit_can_frame(&FORD_4C);
    transmit_can_frame(&FORD_7E);
    transmit_can_frame(&FORD_48);
    transmit_can_frame(&FORD_165);
  }
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;
    transmit_can_frame(&FORD_42C);
    transmit_can_frame(&FORD_42F);
    transmit_can_frame(&FORD_43D);
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(
        &FORD_12F);  //This message actually has checksum/counter, but it seems to close contactors without those
    transmit_can_frame(&FORD_332);
    transmit_can_frame(&FORD_333);
    transmit_can_frame(&FORD_42B);
    transmit_can_frame(&FORD_2EC);
    transmit_can_frame(&FORD_156);
    transmit_can_frame(
        &FORD_5A);  //This message actually has checksum/counter, but it seems to close contactors without those
    transmit_can_frame(&FORD_166);
    transmit_can_frame(&FORD_175);
  }
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
    transmit_can_frame(&FORD_3C3);
    transmit_can_frame(&FORD_581);
  }
}

void FordMachEBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;  //TODO, Are all mach-e batteries 96S?
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
