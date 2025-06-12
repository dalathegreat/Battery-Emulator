#ifndef DALY_BMS_H
#define DALY_BMS_H

#include "RS485Battery.h"

/* Tweak these according to your battery build */
#define CELL_COUNT 14
#define MAX_PACK_VOLTAGE_DV 580   //580 = 58.0V
#define MIN_PACK_VOLTAGE_DV 460   //480 = 48.0V
#define MAX_CELL_VOLTAGE_MV 4200  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 3200  //Battery is put into emergency stop if one cell goes below this value
#define POWER_PER_PERCENT 50     // below 20% and above 80% limit power to 50W * SOC (i.e. 150W at 3%, 500W at 10%, ...)
#define POWER_PER_DEGREE_C 60    // max power added/removed per degree above/below 0°C
#define POWER_AT_0_DEGREE_C 800  // power at 0°C

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define RS485_BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS DalyBms

class DalyBms : public RS485Battery {
 public:
  void setup();
  void update_values();
  void transmit_rs485(unsigned long currentMillis);
  void receive();

 private:
  int baud_rate() { return 9600; }
};

#endif
