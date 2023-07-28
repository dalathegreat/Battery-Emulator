#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include <Arduino.h>
#include "ESP32CAN.h"

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
extern uint16_t CANerror;
extern uint16_t min_volt_solax_can;
extern uint16_t max_volt_solax_can;
extern uint16_t cell_max_voltage;
extern uint16_t cell_min_voltage;

// Definitions for BMS status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

void update_values_can_solax();
void send_can_solax();
void receive_can_solax(CAN_frame_t rx_frame);
#endif