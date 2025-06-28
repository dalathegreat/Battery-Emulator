#ifndef SOFAR_CAN_H
#define SOFAR_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef SOFAR_CAN
#define SELECTED_INVERTER_CLASS SofarInverter
#endif

class SofarInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "Sofar BMS (Extended Frame) over CAN bus";

 private:
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send

  //Actual content messages
  //Note that these are technically extended frames. If more batteries are put in parallel,the first battery sends 0x351 the next battery sends 0x1351 etc. 16 batteries in parallel supported
  CAN_frame SOFAR_351 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x351,
                         .data = {0xC6, 0x08, 0xFA, 0x00, 0xFA, 0x00, 0x80, 0x07}};
  CAN_frame SOFAR_355 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x355,
                         .data = {0x31, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xF6, 0x00}};
  CAN_frame SOFAR_356 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x356,
                         .data = {0x36, 0x08, 0x10, 0x00, 0xD0, 0x00, 0x01, 0x00}};
  CAN_frame SOFAR_30F = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x30F,
                         .data = {0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_359 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x359,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x04, 0x10, 0x27, 0x10}};
  CAN_frame SOFAR_35E = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x35E,
                         .data = {0x41, 0x4D, 0x41, 0x53, 0x53, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_35F = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x35F,
                         .data = {0x00, 0x00, 0x24, 0x4E, 0x32, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_35A = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x35A,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame SOFAR_670 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x670,
                         .data = {0x00, 0x8A, 0x33, 0x11, 0x59, 0x1A, 0x00, 0x00}};
  CAN_frame SOFAR_671 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x671,
                         .data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
  CAN_frame SOFAR_672 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x672,
                         .data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
  CAN_frame SOFAR_673 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x673,
                         .data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
  CAN_frame SOFAR_680 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x680,
                         .data = {0x00, 0xB7, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
  CAN_frame SOFAR_681 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x681,
                         .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
  CAN_frame SOFAR_682 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x682,
                         .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
  CAN_frame SOFAR_683 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x683,
                         .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
  CAN_frame SOFAR_684 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x684,
                         .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
  CAN_frame SOFAR_685 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x685,
                         .data = {0x00, 0xB3, 0x0C, 0xBB, 0x0C, 0xB3, 0x0C, 0x00}};
  CAN_frame SOFAR_690 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x690,
                         .data = {0x00, 0xD7, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_691 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x691,
                         .data = {0x00, 0xD4, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_6A0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x6A0,
                         .data = {0x00, 0xFA, 0x00, 0xDD, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_6B0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x6B0,
                         .data = {0x00, 0xF6, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00}};
  CAN_frame SOFAR_6C0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x6C0,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_770 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x770,
                         .data = {0x00, 0x56, 0x0B, 0xF0, 0x58, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_771 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x771,
                         .data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
  CAN_frame SOFAR_772 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x772,
                         .data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
  CAN_frame SOFAR_773 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x773,
                         .data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
  CAN_frame SOFAR_780 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x780,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_781 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x781,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_782 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x782,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_783 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x783,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_784 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x784,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_785 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x785,
                         .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
  CAN_frame SOFAR_790 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x790,
                         .data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_791 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x791,
                         .data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_7A0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x7A0,
                         .data = {0x00, 0xFA, 0x00, 0xE1, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOFAR_7B0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x7B0,
                         .data = {0x00, 0xF9, 0x00, 0x06, 0x02, 0xE9, 0x5D, 0x00}};
  CAN_frame SOFAR_7C0 = {.FD = false,
                         .ext_ID = true,
                         .DLC = 8,
                         .ID = 0x7C0,
                         .data = {0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x80, 0x00}};
};

#endif
