#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#define MODBUS_INVERTER_SELECTED
#define SELECTED_INVERTER_CLASS BydModbusInverter

#define MB_RTU_NUM_VALUES 13100
#define MAX_POWER 40960  //BYD Modbus specific value

extern uint16_t mbPV[MB_RTU_NUM_VALUES];

class BydModbusInverter : public ModbusInverterProtocol {
 public:
  void setup();
  void update_modbus_registers();
  void handle_static_data();
};

#endif
