#ifndef LUNA2000_MODBUS_H
#define LUNA2000_MODBUS_H
#include "../include.h"

#define INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern uint16_t system_scaled_SOC_pptt;      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;       //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void update_modbus_registers_luna2000();
void handle_update_data_modbus32051();
void handle_update_data_modbus39500();
#endif
