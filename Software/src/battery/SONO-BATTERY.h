#ifndef SONO_BATTERY_H
#define SONO_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

uint8_t CalculateCRC8(CAN_frame rx_frame);

class SonoBattery : public CanBattery {
 public:
  SonoBattery() : CanBattery(Sono) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Sono Motors Sion 64kWh LFP ";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 5000; }
  virtual uint16_t min_pack_voltage_dv() { return 2500; }
  virtual uint16_t max_cell_deviation_mv() { return 250; }
  virtual uint16_t max_cell_voltage_mv() { return 3800; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }
};

#endif
