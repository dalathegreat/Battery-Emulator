#ifndef SANTA_FE_PHEV_BATTERY_H
#define SANTA_FE_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4040  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 2880
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

uint8_t CalculateCRC8(CAN_frame rx_frame);

class SantaFePhevBattery : public CanBattery {
 public:
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Santa Fe PHEV";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
