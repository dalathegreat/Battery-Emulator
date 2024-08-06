#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

// Timeout in milliseconds
#define SolaxTimeout 2000

//SOLAX BMS States Definition
#define BATTERY_ANNOUNCE 0
#define WAITING_FOR_CONTACTOR 1
#define CONTACTOR_CLOSED 2
#define FAULT_SOLAX 3
#define UPDATING_FW 4

#endif
