#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-acan2515/ACAN2515.h"

#define CAN_INVERTER_SELECTED

extern ACAN2515 can;

// Timeout in milliseconds
#define SolaxTimeout 2000

//SOLAX BMS States Definition
#define BATTERY_ANNOUNCE 0
#define WAITING_FOR_CONTACTOR 1
#define CONTACTOR_CLOSED 2
#define FAULT_SOLAX 3
#define UPDATING_FW 4

void receive_can_solax(CAN_frame_t rx_frame);
#endif
