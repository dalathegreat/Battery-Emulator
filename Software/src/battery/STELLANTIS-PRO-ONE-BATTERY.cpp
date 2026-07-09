#include "STELLANTIS-PRO-ONE-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

static uint8_t CalculateCRC8SAEJ1850(CAN_frame rx_frame) {
  uint8_t crc = 0xFF;  // initial value 0xFF
  for (uint8_t j = 0; j < 7; j++) {
    crc = crc8_table_SAE_J1850_ZER0[crc ^ rx_frame.data.u8[j]];
  }
  return crc ^ 0xFF;  // final XOR 0xFF
}

void StellantisProOneBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //datalayer.battery.status.real_soc; //TODO: locate
  //datalayer.battery.status.soh_pptt; //TODO: locate
  //datalayer.battery.status.voltage_dV; //TODO: locate
  //datalayer.battery.status.current_dA; //TODO: locate
  //datalayer.battery.status.remaining_capacity_Wh; //TODO: locate
  //datalayer.battery.status.max_discharge_power_W; //TODO: locate
  //datalayer.battery.status.max_charge_power_W; //TODO: locate
  //datalayer.battery.status.cell_max_voltage_mV; //TODO: locate
  //datalayer.battery.status.cell_min_voltage_mV; //TODO: locate
  //datalayer.battery.status.temperature_min_dC; //TODO: locate
  //datalayer.battery.status.temperature_max_dC; //TODO: locate
}

void StellantisProOneBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x95:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xE0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x107:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      expectedCRC = CalculateCRC8SAEJ1850(rx_frame);
      if (expectedCRC == rx_frame.data.u8[7]) {
        //Message is valid, process it
      } else {
        //CRC error, ignore message
        datalayer.battery.status.CAN_error_counter++;
      }
      break;
    case 0x1D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x220:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x281:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x285:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x306:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x307:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x312:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x354:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x358:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void StellantisProOneBattery::transmit_can(unsigned long currentMillis) {

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;
  }

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    if (UserRequestedDTCReset == true) {
      UserRequestedDTCReset = false;
      transmit_can_frame(&STELLANTIS_CLEAR_DTC);  //Send DTC reset command
    }
  }
}

void StellantisProOneBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
