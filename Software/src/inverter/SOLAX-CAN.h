#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-acan2515/ACAN2515.h"

#define INVERTER_SELECTED

extern ACAN2515 can;

extern uint16_t system_scaled_SOC_pptt;      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;       //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

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
