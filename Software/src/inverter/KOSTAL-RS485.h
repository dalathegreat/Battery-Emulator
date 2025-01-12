#ifndef BYD_KOSTAL_RS485_H
#define BYD_KOSTAL_RS485_H
#include <Arduino.h>
#include "../include.h"

#define RS485_INVERTER_SELECTED
//#define DEBUG_KOSTAL_RS485_DATA  // Enable this line to get TX / RX printed out via logging

#if defined(DEBUG_KOSTAL_RS485_DATA) && !defined(DEBUG_LOG)
#error "enable LOG_TO_SD, DEBUG_VIA_USB or DEBUG_VIA_WEB in order to use DEBUG_KOSTAL_RS485_DATA"
#endif

#endif
