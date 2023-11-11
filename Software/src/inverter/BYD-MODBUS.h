#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include <Arduino.h>

#define MB_RTU_NUM_VALUES 30000
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
extern uint16_t max_voltage;
extern uint16_t min_voltage;

void handle_static_data_modbus_byd();
void handle_update_data_modbusp201_byd();
void handle_update_data_modbusp301_byd();
void update_modbus_registers_byd();
#endif
