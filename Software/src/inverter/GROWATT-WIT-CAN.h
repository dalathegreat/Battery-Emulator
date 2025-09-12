#ifndef GROWATT_WIT_CAN_H
#define GROWATT_WIT_CAN_H

#include "CanInverterProtocol.h"

class GrowattWitInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Growatt WIT compatible battery via CAN";

 private:
  /* Do not change code below unless you are sure what you are doing */

  //Actual content messages
  CAN_frame GROWATT_1AC3XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC3,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC4XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC4,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC5XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC5,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC6XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC6,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC7XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC7,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC0XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC0,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC2XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC2,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC8XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC8,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AC9XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AC9,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1ACAXXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1ACA,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1ACCXXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1ACC,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1ACDXXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1ACD,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1ACEXXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1ACE,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1ACFXXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1ACF,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AD0XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AD0,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AD1XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AD1,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AD8XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AD8,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1AD9XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1AD9,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1A80XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1A80,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_1A82XXXX = {.FD = false,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x1A82,  //TODO
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  unsigned long previousMillis100ms = 0;
  unsigned long previousMillis500ms = 0;
};

#endif
