#ifndef DALY_BMS_H
#define DALY_BMS_H

/* Tweak these according to your battery build */
#define POWER_PER_PERCENT 50     // below 20% and above 80% limit power to 50W * SOC (i.e. 150W at 3%, 500W at 10%, ...)
#define POWER_PER_DEGREE_C 60    // max power added/removed per degree above/below 0°C
#define POWER_AT_0_DEGREE_C 800  // power at 0°C

/* Do not modify any rows below*/
#define BATTERY_SELECTED

class DalyBms : public RS485Battery {
 public:
  DalyBms() : RS485Battery(Daly) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "DALY RS485";

  virtual void setup();
  virtual void update_values();

  virtual int rs485_baudrate() { return 9600; }
  virtual void transmit_RS485();
  virtual void receive_RS485();

  virtual uint16_t max_pack_voltage_dv() { return 580; }
  virtual uint16_t min_pack_voltage_dv() { return 460; }
  virtual uint16_t max_cell_deviation_mv() { return 100; }
  virtual uint16_t max_cell_voltage_mv() { return 4200; }
  virtual uint16_t min_cell_voltage_mv() { return 3200; }
  virtual uint8_t number_of_cells() { return 14; }
};

#endif
