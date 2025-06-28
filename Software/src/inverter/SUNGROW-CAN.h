#ifndef SUNGROW_CAN_H
#define SUNGROW_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef SUNGROW_CAN
#define SELECTED_INVERTER_CLASS SungrowInverter
#endif

class SungrowInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "Sungrow SBR064 battery over CAN bus";

 private:
  unsigned long previousMillis500ms = 0;
  bool alternate = false;
  uint8_t mux = 0;
  uint8_t version_char[14] = {0};
  uint8_t manufacturer_char[14] = {0};
  uint8_t model_char[14] = {0};
  bool inverter_sends_000 = false;

  //Actual content messages
  CAN_frame SUNGROW_000 = {.FD = false,  // Sent by inv or BMS?
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x000,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_001 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x001,
                           .data = {0xF0, 0x05, 0x20, 0x03, 0x2C, 0x01, 0x2C, 0x01}};
  CAN_frame SUNGROW_002 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x002,
                           .data = {0xA2, 0x05, 0x10, 0x27, 0x9B, 0x03, 0x00, 0x19}};
  CAN_frame SUNGROW_003 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x003,
                           .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
  CAN_frame SUNGROW_004 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x004,
                           .data = {0x27, 0x05, 0x00, 0x00, 0x24, 0x05, 0x08, 0x01}};
  CAN_frame SUNGROW_005 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x005,
                           .data = {0x02, 0x00, 0x01, 0xE6, 0x20, 0x24, 0x05, 0x00}};
  CAN_frame SUNGROW_006 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x006,
                           .data = {0x0E, 0x01, 0x01, 0x01, 0xDE, 0x0C, 0xD5, 0x0C}};
  CAN_frame SUNGROW_013 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x013,
                           .data = {0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x0E, 0x01}};
  CAN_frame SUNGROW_014 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x014,
                           .data = {0x05, 0x01, 0xAC, 0x80, 0x10, 0x02, 0x57, 0x80}};
  CAN_frame SUNGROW_015 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x015,
                           .data = {0x93, 0x80, 0xAC, 0x80, 0x57, 0x80, 0x93, 0x80}};
  CAN_frame SUNGROW_016 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x016,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_017 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x017,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_018 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x018,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_019 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x019,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01A,
                           .data = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01B,
                           .data = {0xBE, 0x8F, 0x61, 0x01, 0xBE, 0x8F, 0x61, 0x01}};
  CAN_frame SUNGROW_01C = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01C,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
};

#endif
