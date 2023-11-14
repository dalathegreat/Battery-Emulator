#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-acan2515/ACAN2515.h"

extern ACAN2515 can;

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
extern uint16_t min_voltage;
extern uint16_t max_voltage;
extern uint16_t cell_max_voltage;
extern uint16_t cell_min_voltage;
extern bool inverterAllowsContactorClosing;

// Timeout in milliseconds
#define SolaxTimeout 2000

//SOLAX BMS States Definition
#define BATTERY_ANNOUNCE 0
#define WAITING_FOR_CONTACTOR 1
#define CONTACTOR_CLOSED 2
#define FAULT_SOLAX 3
#define UPDATING_FW 4

void update_values_can_solax();
void receive_can_solax(CAN_frame_t rx_frame);
#endif
