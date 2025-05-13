#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#define MODBUS_INVERTER_SELECTED
#define SELECTED_INVERTER_CLASS BydModbusInverter

#include "ModbusInverterProtocol.h"

class BydModbusInverter : public ModbusInverterProtocol {
 public:
  void setup();
  void update_values();

 private:
  void handle_static_data();
  void verify_temperature();
  void verify_inverter_modbus();
  void handle_update_data_modbusp201_byd();
  void handle_update_data_modbusp301_byd();

  //BYD Modbus specific value
  const int MAX_POWER = 40960;
};

#endif
