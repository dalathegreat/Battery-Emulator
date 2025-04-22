#ifndef JAGUAR_IPACE_BATTERY_H
#define JAGUAR_IPACE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED

class JaguarIpaceBattery : public CanBattery {
 public:
  JaguarIpaceBattery() : CanBattery(JaguarIpace) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Jaguar I-PACE";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 4546; }
  virtual uint16_t min_pack_voltage_dv() { return 3370; }
  virtual uint16_t max_cell_deviation_mv() { return 250; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }
};

#endif
