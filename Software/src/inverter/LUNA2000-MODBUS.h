#ifndef LUNA2000_MODBUS_H
#define LUNA2000_MODBUS_H
#include "../include.h"

#define INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void update_modbus_registers_luna2000();
void handle_update_data_modbus32051();
void handle_update_data_modbus39500();
#endif
