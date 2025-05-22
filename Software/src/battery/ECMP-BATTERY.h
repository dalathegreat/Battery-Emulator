#ifndef STELLANTIS_ECMP_BATTERY_H
#define STELLANTIS_ECMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS EcmpBattery

class EcmpBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

 private:
  static const int MAX_CELL_DEVIATION_MV = 250;

  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

  //Actual content messages
  CAN_frame ECMP_XXX = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x301,
                        .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  uint16_t battery_voltage = 37000;
  uint16_t battery_soc = 0;
  uint16_t cellvoltages[108];
};

#endif
