#include "VCU-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../inverter/INVERTERS.h"

/*TODO once testing starts:
- Map more optional content into all message
- Add OBD2 responses for 79B /7BB polls
-*/

static uint8_t calculate_CRC_Nissan(CAN_frame* frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable_nissan_leaf[(crc ^ static_cast<uint8_t>(frame->data.u8[j])) % 256];
  }
  return crc;
}

void VCUInverter::update_values() {  //Called every 1s

  LEAF_1DC.data.u8[0] = ((datalayer.battery.status.max_discharge_power_W / 4) << 2);
  LEAF_1DC.data.u8[1] = ((datalayer.battery.status.max_discharge_power_W / 4) << 6);
  LEAF_1DC.data.u8[1] = (LEAF_1DC.data.u8[1] || (((datalayer.battery.status.max_charge_power_W / 4) >> 4) & 0x3F));
  LEAF_1DC.data.u8[2] = ((datalayer.battery.status.max_charge_power_W / 4) << 4);

  LEAF_55B.data.u8[0] = ((datalayer.battery.status.real_soc / 10) << 2);
  LEAF_55B.data.u8[1] = ((datalayer.battery.status.real_soc / 10) << 6);

  LEAF_1DB.data.u8[0] =
      ((datalayer.battery.status.current_dA / 10) / 2) >> 3;  //TODO: This is most likely handled wrong
  LEAF_1DB.data.u8[1] = ((((datalayer.battery.status.current_dA / 10) / 2) & 0x07) << 5);
  LEAF_1DB.data.u8[2] = ((datalayer.battery.status.voltage_dV / 10) / 2) >> 2;
  LEAF_1DB.data.u8[3] = (((datalayer.battery.status.voltage_dV / 10) / 2) << 6) | 0x2B;  //Lots of status flags here

  remining_gids = (datalayer.battery.status.real_soc / 10000.0) * 281;  //0-281 for 24kWh
  LEAF_5BC.data.u8[0] = remining_gids << 2;
  LEAF_5BC.data.u8[1] = remining_gids << 6;
  LEAF_5BC.data.u8[4] = (datalayer.battery.status.soh_pptt / 100) << 1;
}

void VCUInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1F2:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D4:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x50B:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x50C:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void VCUInverter::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    LEAF_1DC.data.u8[6] = mprun10;
    LEAF_1DC.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DC);
    transmit_can_frame(&LEAF_1DC);
    LEAF_1DB.data.u8[6] = mprun10;
    LEAF_1DB.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DB);
    transmit_can_frame(&LEAF_1DB);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    mprun100 = (mprun100 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    counter_55B = (counter_55B + 1) % 10;
    if (counter_55B < 5) {
      LEAF_55B.data.u8[2] = 0xAA;
    } else {
      LEAF_55B.data.u8[2] = 0x55;
    }
    LEAF_55B.data.u8[6] = (1 << 4) | mprun100;  //1 = RefuseToSleep 2 = ReadyToSleep
    LEAF_55B.data.u8[7] = calculate_CRC_Nissan(&LEAF_55B);
    transmit_can_frame(&LEAF_55B);
    transmit_can_frame(&LEAF_5BC);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&LEAF_59E);
    if (LEAF_5C0.data.u8[0] == 0x40) {
      LEAF_5C0.data.u8[0] = 0x80;
    } else if (LEAF_5C0.data.u8[0] == 0x80) {
      LEAF_5C0.data.u8[0] = 0xC0;
    } else if (LEAF_5C0.data.u8[0] == 0xC0) {
      LEAF_5C0.data.u8[0] = 0x40;
    }
    transmit_can_frame(&LEAF_5C0);
  }
}
