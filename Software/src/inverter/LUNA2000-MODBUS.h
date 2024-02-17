#ifndef LUNA2000_MODBUS_H
#define LUNA2000_MODBUS_H
#include <Arduino.h>
#include "../devboard/config.h"  // Needed for all defines

#define MB_RTU_NUM_VALUES 30000

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t battery_voltage;
extern uint16_t battery_current;
extern uint16_t capacity_Wh;
extern uint16_t remaining_capacity_Wh;
extern uint16_t max_target_discharge_power;
extern uint16_t max_target_charge_power;
extern uint16_t stat_batt_power;
extern uint16_t temperature_min;
extern uint16_t temperature_max;
extern uint16_t max_power;
extern uint16_t max_voltage;
extern uint16_t min_voltage;
extern bool batteryAllowsContactorClosing;   //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void update_modbus_registers_luna2000();
void handle_update_data_modbus32051();
void handle_update_data_modbus39500();
#endif
