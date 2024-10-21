#include "../include.h"
#ifdef SUNGROW_CAN
#include "../datalayer/datalayer.h"
#include "SUNGROW-CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1s = 0;  // will store last time a 1s CAN Message was send

static uint8_t mux = 0;
static uint8_t version_char[14] = {0};
static uint8_t manufacturer_char[14] = {0};
static uint8_t model_char[14] = {0};

//Actual content messages
CAN_frame SUNGROW_400 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x400,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_500 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x500,
                         .data = {0x01, 0x01, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x32}};
CAN_frame SUNGROW_501 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x501,
                         .data = {0xF0, 0x05, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_502 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x502,
                         .data = {0xA2, 0x05, 0x00, 0x00, 0x9B, 0x03, 0x00, 0x19}};
CAN_frame SUNGROW_503 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x503,
                         .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
CAN_frame SUNGROW_504 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x504,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_505 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x505,
                         .data = {0x00, 0x02, 0x01, 0xE6, 0x20, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_506 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x506,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_512 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x512,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_700 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x700,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_701 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x701,
                         .data = {0xF0, 0x05, 0x20, 0x03, 0x2C, 0x01, 0x2C, 0x01}};
CAN_frame SUNGROW_702 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x702,
                         .data = {0xA2, 0x05, 0x10, 0x27, 0x9B, 0x03, 0x00, 0x19}};
CAN_frame SUNGROW_703 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x703,
                         .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
CAN_frame SUNGROW_704 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x704,
                         .data = {0x27, 0x05, 0x00, 0x00, 0x24, 0x05, 0x08, 0x01}};
CAN_frame SUNGROW_705 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x705,
                         .data = {0x02, 0x00, 0x01, 0xE6, 0x20, 0x24, 0x05, 0x00}};
CAN_frame SUNGROW_706 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x706,
                         .data = {0x0E, 0x01, 0x01, 0x01, 0xDE, 0x0C, 0xD5, 0x0C}};
CAN_frame SUNGROW_713 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x713,
                         .data = {0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x0E, 0x01}};
CAN_frame SUNGROW_714 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x714,
                         .data = {0x05, 0x01, 0xAC, 0x80, 0x10, 0x02, 0x57, 0x80}};
CAN_frame SUNGROW_715 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x715,
                         .data = {0x93, 0x80, 0xAC, 0x80, 0x57, 0x80, 0x93, 0x80}};
CAN_frame SUNGROW_716 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x716,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_717 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x717,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_718 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x718,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_719 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x719,
                         .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71A = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71A,
                         .data = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71B = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71B,
                         .data = {0xBE, 0x8F, 0x61, 0x01, 0xBE, 0x8F, 0x61, 0x01}};
CAN_frame SUNGROW_71C = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71C,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71D = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71D,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71E = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71E,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //TODO: Update all the SUNGROW_### content here
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //In here we need to respond to the inverter
    case 0x000:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x100:  // SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:  // Both SH10RS / SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x102:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Version number byte1-7 (e.g. @23A229)
        version_char[0] = rx_frame.data.u8[1];
        version_char[1] = rx_frame.data.u8[2];
        version_char[2] = rx_frame.data.u8[3];
        version_char[3] = rx_frame.data.u8[4];
        version_char[4] = rx_frame.data.u8[5];
        version_char[5] = rx_frame.data.u8[6];
        version_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Version number byte1-7 continued (e.g 2795)
        version_char[7] = rx_frame.data.u8[1];
        version_char[8] = rx_frame.data.u8[2];
        version_char[9] = rx_frame.data.u8[3];
        version_char[10] = rx_frame.data.u8[4];
        version_char[11] = rx_frame.data.u8[5];
        version_char[12] = rx_frame.data.u8[6];
        version_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x104:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Manufacturer byte1-7 (e.g. SUNGROW)
        manufacturer_char[0] = rx_frame.data.u8[1];
        manufacturer_char[1] = rx_frame.data.u8[2];
        manufacturer_char[2] = rx_frame.data.u8[3];
        manufacturer_char[3] = rx_frame.data.u8[4];
        manufacturer_char[4] = rx_frame.data.u8[5];
        manufacturer_char[5] = rx_frame.data.u8[6];
        manufacturer_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Manufacturer byte1-7 continued (e.g )
        manufacturer_char[7] = rx_frame.data.u8[1];
        manufacturer_char[8] = rx_frame.data.u8[2];
        manufacturer_char[9] = rx_frame.data.u8[3];
        manufacturer_char[10] = rx_frame.data.u8[4];
        manufacturer_char[11] = rx_frame.data.u8[5];
        manufacturer_char[12] = rx_frame.data.u8[6];
        manufacturer_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x105:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Model byte1-7 (e.g. SH10RT)
        model_char[0] = rx_frame.data.u8[1];
        model_char[1] = rx_frame.data.u8[2];
        model_char[2] = rx_frame.data.u8[3];
        model_char[3] = rx_frame.data.u8[4];
        model_char[4] = rx_frame.data.u8[5];
        model_char[5] = rx_frame.data.u8[6];
        model_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Model byte1-7 continued (e.g )
        model_char[7] = rx_frame.data.u8[1];
        model_char[8] = rx_frame.data.u8[2];
        model_char[9] = rx_frame.data.u8[3];
        model_char[10] = rx_frame.data.u8[4];
        model_char[11] = rx_frame.data.u8[5];
        model_char[12] = rx_frame.data.u8[6];
        model_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x106:  // 250ms - SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x151:  //Only sent by SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Manufacturer byte1-7 (e.g. SUNGROW)
        manufacturer_char[0] = rx_frame.data.u8[1];
        manufacturer_char[1] = rx_frame.data.u8[2];
        manufacturer_char[2] = rx_frame.data.u8[3];
        manufacturer_char[3] = rx_frame.data.u8[4];
        manufacturer_char[4] = rx_frame.data.u8[5];
        manufacturer_char[5] = rx_frame.data.u8[6];
        manufacturer_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Manufacturer byte1-7 continued (e.g )
        manufacturer_char[7] = rx_frame.data.u8[1];
        manufacturer_char[8] = rx_frame.data.u8[2];
        manufacturer_char[9] = rx_frame.data.u8[3];
        manufacturer_char[10] = rx_frame.data.u8[4];
        manufacturer_char[11] = rx_frame.data.u8[5];
        manufacturer_char[12] = rx_frame.data.u8[6];
        manufacturer_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x191:  //Only sent by SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x00004200:  //Only sent by SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02007F00:  //Only sent by SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();
  // Send 1s CAN Message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    //TODO: This will overload the buffer most likely. Split up and delay with a few ms inbetween
    transmit_can(&SUNGROW_512, can_config.inverter);
    transmit_can(&SUNGROW_501, can_config.inverter);
    transmit_can(&SUNGROW_502, can_config.inverter);
    transmit_can(&SUNGROW_503, can_config.inverter);
    transmit_can(&SUNGROW_504, can_config.inverter);
    transmit_can(&SUNGROW_505, can_config.inverter);
    transmit_can(&SUNGROW_506, can_config.inverter);
    transmit_can(&SUNGROW_500, can_config.inverter);
    transmit_can(&SUNGROW_400, can_config.inverter);
    transmit_can(&SUNGROW_700, can_config.inverter);
    transmit_can(&SUNGROW_701, can_config.inverter);
    transmit_can(&SUNGROW_702, can_config.inverter);
    transmit_can(&SUNGROW_703, can_config.inverter);
    transmit_can(&SUNGROW_704, can_config.inverter);
    transmit_can(&SUNGROW_705, can_config.inverter);
    transmit_can(&SUNGROW_706, can_config.inverter);
    transmit_can(&SUNGROW_713, can_config.inverter);
    transmit_can(&SUNGROW_714, can_config.inverter);
    transmit_can(&SUNGROW_715, can_config.inverter);
    transmit_can(&SUNGROW_716, can_config.inverter);
    transmit_can(&SUNGROW_717, can_config.inverter);
    transmit_can(&SUNGROW_718, can_config.inverter);
    transmit_can(&SUNGROW_719, can_config.inverter);
    transmit_can(&SUNGROW_71A, can_config.inverter);
    transmit_can(&SUNGROW_71B, can_config.inverter);
    transmit_can(&SUNGROW_71C, can_config.inverter);
    transmit_can(&SUNGROW_71D, can_config.inverter);
    transmit_can(&SUNGROW_71E, can_config.inverter);
  }
}
#endif
