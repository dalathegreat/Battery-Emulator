#ifndef RENAULT_TWIZY_BATTERY_H
#define RENAULT_TWIZY_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED

class RenaultTwizyBattery : public CanBattery {
 public:
  RenaultTwizyBattery() : CanBattery(RenaultTwizy) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Renault Twizy";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 579; }  // 57.9V at 100% SOC (with 70% SOH, new one might be higher)
  virtual uint16_t min_pack_voltage_dv() { return 480; }  // 48.4V at 13.76% SOC
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 4200; }
  virtual uint16_t min_cell_voltage_mv() { return 3400; }
};

#endif
