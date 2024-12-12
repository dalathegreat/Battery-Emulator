#ifndef BYD_KOSTAL_RS485_H
#define BYD_KOSTAL_RS485_H
#include <Arduino.h>
#include "../include.h"

#define RS485_INVERTER_SELECTED
//#define DEBUG_KOSTAL_RS485_DATA  // Enable this line to get TX / RX printed out via serial

#ifdef DEBUG_KOSTAL_RS485_DATA
#ifndef DEBUG_VIA_USB
#define DEBUG_VIA_USB
#endif
#endif

#endif
