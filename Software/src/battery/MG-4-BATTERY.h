#ifndef MG_4_BATTERY_H
#define MG_4_BATTERY_H

#include "UdsCanBattery.h"

class Mg4Battery : public UdsCanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MG4 battery";

 private:
  static const uint16_t MAX_CELL_DEVIATION_MV = 150;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send
  unsigned long four_seven_counter = 0;

  CAN_frame MG4_4F3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4F3,
                       .data = {0xF3, 0x10, 0x44, 0x00, 0xFF, 0xFF, 0x00, 0x11}};
  CAN_frame MG4_047_E9 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x047,
                          .data = {0xE9, 0x0A, 0x45, 0x7D, 0x7F, 0xFF, 0xFF, 0xFE}};
  CAN_frame MG4_047_3B = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x047,
                          .data = {0x3B, 0x0B, 0x45, 0x7D, 0x7F, 0xFF, 0xFF, 0xFE}};
};

#endif
