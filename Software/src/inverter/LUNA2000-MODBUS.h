#ifndef LUNA2000_MODBUS_H
#define LUNA2000_MODBUS_H
#include "../include.h"

#define MODBUS_INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000

extern uint16_t mbPV[MB_RTU_NUM_VALUES];

void handle_update_data_modbus32051();
void handle_update_data_modbus39500();
#endif
