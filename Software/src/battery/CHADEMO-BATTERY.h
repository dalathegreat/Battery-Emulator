#ifndef CHADEMO_BATTERY_H
#define CHADEMO_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 9999

//Contactor control is required for CHADEMO support
#define CONTACTOR_CONTROL

//ISA shunt is currently required for CHADEMO support
// other measurement sources may be added in the future
#define ISA_SHUNT

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
