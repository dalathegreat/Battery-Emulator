#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#define INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000
#define MAX_POWER 40960  //BYD Modbus specific value

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern uint16_t system_scaled_SOC_pptt;      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false
extern bool system_LFP_Chemistry;            //Bool, true/false

void handle_static_data_modbus_byd();
void verify_temperature_modbus();
void handle_update_data_modbusp201_byd();
void handle_update_data_modbusp301_byd();
void update_modbus_registers_byd();
#endif
