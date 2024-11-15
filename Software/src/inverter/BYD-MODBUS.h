#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#define MODBUS_INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000
#define MAX_POWER 40960  //BYD Modbus specific value

extern uint16_t mbPV[MB_RTU_NUM_VALUES];

void handle_static_data_modbus_byd();
void verify_temperature_modbus();
void verify_inverter_modbus();
void handle_update_data_modbusp201_byd();
void handle_update_data_modbusp301_byd();
void setup_inverter(void);
#endif
