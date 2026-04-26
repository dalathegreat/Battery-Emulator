#ifndef DALY_BMS_H
#define DALY_BMS_H

#include "RS485Battery.h"

class DalyBms : public RS485Battery {
 public:
  void setup();
  void update_values();
  void transmit_rs485(unsigned long currentMillis);
  void receive();
  static constexpr const char* Name = "DALY RS485";

 private:
  int baud_rate() { return 9600; }
};

#endif
