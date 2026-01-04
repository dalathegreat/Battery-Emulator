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
  CAN_frame LEAF_1DC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DC,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_1DB = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DB,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_55B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x55B,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_5BC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5BC,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_59E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x59E,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_5C0 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5C0,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
