#ifndef SOL_ARK_LV_CAN_H
#define SOL_ARK_LV_CAN_H

#include "CanInverterProtocol.h"

class SolArkLvInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Sol-Ark LV protocol over CAN bus";

 private:
  unsigned long previousMillis1000ms = 0;

  const uint8_t MODULE_NUMBER = 1;  //8-bit integer representing quantity of parallel connected batteries

  CAN_frame SOLARK_359 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x359,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOLARK_351 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x351,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOLARK_355 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x355,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOLARK_356 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x356,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOLARK_35C = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x35C,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SOLARK_35E = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x35E,  //BAT-EMU
                          .data = {0x42, 0x41, 0x54, 0x2D, 0x45, 0x4D, 0x55, 0x20}};
};

#endif
