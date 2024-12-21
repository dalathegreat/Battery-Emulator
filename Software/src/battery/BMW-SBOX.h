#ifndef BMW_SBOX_CONTROL_H
#define BMW_SBOX_CONTROL_H
#include "../include.h"
#define CAN_SHUNT_SELECTED
void transmit_can(CAN_frame* tx_frame, int interface);

/** Minimum input voltage required to enable relay control **/
#define MINIMUM_INPUT_VOLTAGE 250

/** Maximum allowable percentage of input voltage across the precharge resistor to engage the positive relay. **/
/** SAFETY FEATURE: If precharge resistor is faulty, positive contactor will not be engaged **/
#define MAX_PRECHARGE_RESISTOR_VOLTAGE_PERCENT 0.99

/* NOTE: modify the T2 time constant below to account for the resistance and capacitance of the target system.
 *      t=3RC at minimum, t=5RC ideally
 */

#define CONTACTOR_CONTROL_T1 5000 // Time before negative contactor engages and precharging starts
#define CONTACTOR_CONTROL_T2 5000 // Precharge time before precharge resistor is bypassed by positive contactor
#define CONTACTOR_CONTROL_T3 2000 // Precharge relay lead time after positive contactor has been engaged

#endif
