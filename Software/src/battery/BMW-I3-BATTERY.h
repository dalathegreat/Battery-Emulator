#ifndef BMW_I3_BATTERY_H
#define BMW_I3_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-acan2515/ACAN2515.h"

extern ACAN2515 can;

#define BATTERY_SELECTED

#define WUP_PIN 25
#define MAX_CELL_VOLTAGE_60AH 4104   // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_60AH 3000   // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_VOLTAGE_94AH 4177   // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_94AH 3000   // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_VOLTAGE_120AH 4210  // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_120AH 3000  // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 150    // LED turns yellow on the board if mv delta exceeds this value
#define MAX_PACK_VOLTAGE_60AH 3920   // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_60AH 3300   // Discharge stops if pack voltage exceeds this value
#define MAX_PACK_VOLTAGE_94AH 3980   // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_94AH 3300   // Discharge stops if pack voltage exceeds this value
#define MAX_PACK_VOLTAGE_120AH 4030  // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_120AH 3300  // Discharge stops if pack voltage exceeds this value
void setup_battery(void);

#endif
