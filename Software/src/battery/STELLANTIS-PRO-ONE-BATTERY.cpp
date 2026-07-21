#include "STELLANTIS-PRO-ONE-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

static uint8_t CalculateCRC8SAEJ1850(CAN_frame rx_frame, uint8_t length) {
  uint8_t crc = 0xFF;  // initial value 0xFF
  for (uint8_t j = 0; j < length;
       j++) {  //Length is the number of bytes to calculate CRC for, not including the CRC byte itself!
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
      expectedCRC = CalculateCRC8SAEJ1850(rx_frame, 7);
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
    case 0x7EF:  //UDS reply
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void StellantisProOneBattery::transmit_can(unsigned long currentMillis) {

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    //Counter goes from 0-0xF and starts over
    counter_10ms = (counter_10ms + 1) & 0x0F;
    //Sentmessages counter
    if (sent_10ms_messages < 254) {
      sent_10ms_messages++;
    }

    if (sent_10ms_messages == 37) {
      ONE_108.data.u8[0] = 0x33;
      ONE_108.data.u8[1] = 0x1D;
      ONE_108.data.u8[2] = 0xF0;
    } else if (sent_10ms_messages == 42) {
      ONE_108.data.u8[1] = 0x25;
      ONE_108.data.u8[2] = 0xC0;
    } else {
    }

    ONE_15A.data.u8[2] = counter_10ms;
    ONE_15A.data.u8[3] = CalculateCRC8SAEJ1850(ONE_15A, 3);
    ONE_1D7.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_1D7.data.u8[7] = CalculateCRC8SAEJ1850(ONE_1D7, 7);
    ONE_175.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_175.data.u8[7] = CalculateCRC8SAEJ1850(ONE_175, 7);
    ONE_108.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_108.data.u8[7] = CalculateCRC8SAEJ1850(ONE_108, 7);

    transmit_can_frame(&ONE_15A);
    transmit_can_frame(&ONE_1D7);
    transmit_can_frame(&ONE_175);
    transmit_can_frame(&ONE_108);
    //transmit_can_frame(&ONE_0F2);
    //transmit_can_frame(&ONE_0F0);
    //transmit_can_frame(&ONE_0B4);

    ONE_175.data.u8[3] = 0x2E;
  }

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;
    //transmit_can_frame(&ONE_212);
    //transmit_can_frame(&ONE_1D8);
    //transmit_can_frame(&ONE_160);
  }

  // Send 50ms CAN Message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;
    //transmit_can_frame(&ONE_242);
    //transmit_can_frame(&ONE_240);
    //transmit_can_frame(&ONE_234);
    //transmit_can_frame(&ONE_235);
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //transmit_can_frame(&ONE_3E0);
    //transmit_can_frame(&ONE_3E7);
    //transmit_can_frame(&ONE_3EB);
    //transmit_can_frame(&ONE_3EE);
    //transmit_can_frame(&ONE_320);
    //transmit_can_frame(&ONE_321);
    //transmit_can_frame(&ONE_322);
    //transmit_can_frame(&ONE_323);
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
