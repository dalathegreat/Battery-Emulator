#ifndef STELLANTIS_ECMP_BATTERY_H
#define STELLANTIS_ECMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class StellantisEcmpBattery : public CanBattery {
 public:
  StellantisEcmpBattery() : CanBattery(Ecmp) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "ECMP battery";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_cell_deviation_mv() { return 250; }
};

#endif
