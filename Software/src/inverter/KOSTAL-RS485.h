#ifndef BYD_KOSTAL_RS485_H
#define BYD_KOSTAL_RS485_H
#include <Arduino.h>
#include "../include.h"

#define RS485_INVERTER_SELECTED
#define RS485_BAUDRATE 57600
//#define DEBUG_KOSTAL_RS485_DATA  // Enable this line to get TX / RX printed out via logging
//#define DEBUG_KOSTAL_RS485_DATA_USB  // Enable this line to get TX / RX printed out via USB

#if defined(DEBUG_KOSTAL_RS485_DATA) && !defined(DEBUG_LOG)
#error "enable LOG_TO_SD, DEBUG_VIA_USB or DEBUG_VIA_WEB in order to use DEBUG_KOSTAL_RS485_DATA"
#endif

#endif
