#ifndef MODBUS_LUNA2000_H
#define MODBUS_LUNA2000_H
#include <Arduino.h>

#define MB_RTU_NUM_VALUES 50000
// Definitions for BMS status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

extern uint16_t mbPV[MB_RTU_NUM_VALUES];
extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t battery_voltage;
extern uint16_t battery_current;
extern uint16_t capacity_Wh;
extern uint16_t remaining_capacity_Wh;
extern uint16_t max_target_discharge_power;
extern uint16_t max_target_charge_power;
extern uint16_t bms_status;
extern uint16_t bms_char_dis_status;
extern uint16_t stat_batt_power;
extern uint16_t temperature_min;
extern uint16_t temperature_max;

extern uint16_t capacity_Wh_startup;
extern uint16_t max_power;
extern uint16_t max_volt_modbus_byd;
extern uint16_t min_volt_modbus_byd;

void update_modbus_registers_luna2000();
void handle_update_data_modbus32051();
void handle_update_data_modbus39500();
#endif