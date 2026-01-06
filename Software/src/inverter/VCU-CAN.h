#ifndef VCU_CAN_H
#define VCU_CAN_H

#include "CanInverterProtocol.h"

class VCUInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "VCU mode: Nissan LEAF battery";

 private:
  unsigned long previousMillis10ms = 0;
  unsigned long previousMillis100ms = 0;
  unsigned long previousMillis500ms = 0;
  uint16_t remining_gids = 281;
  uint8_t mprun10 = 0;
  uint8_t mprun100 = 0;
  uint8_t counter_55B = 0;
  CAN_frame LEAF_1DC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DC,
                        .data = {0x6E, 0x0C, 0x2F, 0xFD, 0x0C, 0x00, 0x00, 0xD8}};
  CAN_frame LEAF_1DB = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DB,
                        .data = {0xFF, 0xC0, 0xB1, 0xAB, 0x08, 0x00, 0x02, 0xB5}};
  CAN_frame LEAF_55B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x55B,
                        .data = {0x27, 0x00, 0xAA, 0x00, 0xE1, 0x00, 0x10, 0x79}};
  CAN_frame LEAF_5BC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5BC,
                        .data = {0x16, 0x00, 0x14, 0x50, 0xC8, 0x02, 0xA1, 0x68}};
  CAN_frame LEAF_59E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x59E,
                        .data = {0x00, 0x00, 0x0E, 0x60, 0x88, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_5C0 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5C0,
                        .data = {0x80, 0x60, 0x60, 0x00, 0xC7, 0xB4, 0x04, 0x0C}};
};

#endif
