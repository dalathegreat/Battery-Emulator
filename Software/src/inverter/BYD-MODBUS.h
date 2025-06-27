#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#ifdef BYD_MODBUS
#define SELECTED_INVERTER_CLASS BydModbusInverter
#endif

#include "ModbusInverterProtocol.h"

class BydModbusInverter : public ModbusInverterProtocol {
 public:
  void setup();
  void update_values();
  static constexpr char* Name = "BYD 11kWh HVM battery over Modbus RTU";

 private:
  void handle_static_data();
  void verify_temperature();
  void verify_inverter_modbus();
  void handle_update_data_modbusp201_byd();
  void handle_update_data_modbusp301_byd();

  //BYD Modbus specific value
  const uint16_t MAX_POWER = 40960;
};

#endif
