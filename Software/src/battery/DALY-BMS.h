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
  /* Tweak these according to your battery build */
  static const int POWER_PER_PERCENT =
      50;  // below 20% and above 80% limit power to 50W * SOC (i.e. 150W at 3%, 500W at 10%, ...)
  static const int POWER_PER_DEGREE_C = 60;    // max power added/removed per degree above/below 0°C
  static const int POWER_AT_0_DEGREE_C = 800;  // power at 0°C

  int baud_rate() { return 9600; }
};

#endif
