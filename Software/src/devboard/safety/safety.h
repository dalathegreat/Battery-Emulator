#ifndef SAFETY_H
#define SAFETY_H
#include <Arduino.h>

#define MAX_CAN_FAILURES 50

#define MAX_CHARGE_DISCHARGE_LIMIT_FAILURES 1

void update_machineryprotection();

#endif
