#ifndef CHADEMO_BATTERY_H
#define CHADEMO_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

//Contactor control is required for CHADEMO support
#define CONTACTOR_CONTROL

//ISA shunt is currently required for CHADEMO support
// other measurement sources may be added in the future
#define ISA_SHUNT

class ChademoBattery : public CanBattery {
 public:
  ChademoBattery() : CanBattery(Chademo) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Chademo V2X mode";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
}

#endif
