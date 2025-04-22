#ifndef MG_5_BATTERY_H
#define MG_5_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class MG5Battery : public CanBattery {
 public:
  MG5Battery() : CanBattery(Mg5) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "MG 5 battery";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 4040; }
  virtual uint16_t min_pack_voltage_dv() { return 3100; }
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }
};

#endif
