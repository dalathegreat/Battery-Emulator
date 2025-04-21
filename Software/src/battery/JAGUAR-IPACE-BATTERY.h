#ifndef JAGUAR_IPACE_BATTERY_H
#define JAGUAR_IPACE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4546  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 3370
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

class JaguarIpaceBattery : public CanBattery {
 public:
  JaguarIpaceBattery() : CanBattery(JaguarIpace) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Jaguar I-PACE";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
