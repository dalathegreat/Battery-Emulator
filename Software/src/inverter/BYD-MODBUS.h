#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"

#define INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 30000
#define MAX_POWER 40960  //BYD Modbus specific value

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern int16_t system_battery_current_dA;      //A+1, -1000 - 1000
extern int16_t system_temperature_min_dC;      //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;      //C+1, -50.0 - 50.0
extern uint16_t system_max_design_voltage_dV;  //V+1,  0-1000.0 (0-10000)
extern uint16_t system_min_design_voltage_dV;  //V+1,  0-1000.0 (0-10000)
extern uint16_t system_scaled_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;     //V+1,  0-1000.0 (0-10000)
extern uint32_t system_max_discharge_power_W;  //W,    0-200000
extern uint32_t system_max_charge_power_W;     //W,    0-200000
extern uint8_t system_bms_status;              //Enum 0-5
extern bool batteryAllowsContactorClosing;     //Bool, true/false
extern bool inverterAllowsContactorClosing;    //Bool, true/false
extern bool system_LFP_Chemistry;              //Bool, true/false

void handle_static_data_modbus_byd();
void verify_temperature_modbus();
void handle_update_data_modbusp201_byd();
void handle_update_data_modbusp301_byd();
void update_modbus_registers_byd();
#endif
