#ifndef SUNGROW_CAN_H
#define SUNGROW_CAN_H

#include "CanInverterProtocol.h"

class SungrowInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Sungrow SBRXXX battery over CAN bus";

 private:
  unsigned long previousMillis500ms = 0;
  unsigned long previousMillis1s = 0;
  unsigned long previousMillis10s = 0;
  unsigned long previousMillis60s = 0;
  bool alternate = false;
  uint8_t mux = 0;
  uint8_t version_char[14] = {0};
  uint8_t manufacturer_char[14] = {0};
  uint8_t model_char[14] = {0};
  uint32_t remaining_wh = 0;
  uint32_t capacity_wh = 0;
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
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_002 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x002,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_003 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x003,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_004 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x004,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_005 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x005,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_006 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x006,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_013 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x013,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_014 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x014,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_015 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x015,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
  CAN_frame SUNGROW_401 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x401,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_500 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x500,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_501 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x501,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_502 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x502,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_503 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x503,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_504 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x504,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_505 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x505,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_702 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x702,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_703 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x703,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_704 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x704,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_705 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x705,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_706 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x706,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_713 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x713,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_714 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x714,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_715 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x715,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
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
