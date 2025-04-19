#ifndef DALY_BMS_H
#define DALY_BMS_H

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

class DalyBms : public RS485Battery {
 public:
  DalyBms() : RS485Battery(Daly) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "DALY RS485";

  virtual void setup();
  virtual void update_values();

  virtual int rs485_baudrate() { return 9600; }
  virtual void transmit_RS485();
  virtual void receive_RS485();
};

#endif
