#ifndef MG_5_BATTERY_H
#define MG_5_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef MG_5_BATTERY
#define SELECTED_BATTERY_CLASS Mg5Battery
#endif

class Mg5Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "MG 5 battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3100;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send

  int BMS_SOC = 0;

  CAN_frame MG_5_100 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x100,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};
};

#endif
