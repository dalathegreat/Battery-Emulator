#ifndef BMW_SBOX_CONTROL_H
#define BMW_SBOX_CONTROL_H
#include "../include.h"
#define CAN_SHUNT_SELECTED
void transmit_can(CAN_frame* tx_frame, int interface);

/** Minimum input voltage required to enable relay control **/
#define MINIMUM_INPUT_VOLTAGE 250

/** Maximum allowable percentage of input voltage across the precharge resistor to close the positive relay **/
#define MAX_PRECHARGE_RESISTOR_VOLTAGE_PERCENT 96


#endif
