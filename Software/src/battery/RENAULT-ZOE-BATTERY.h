#ifndef RENAULT_ZOE_BATTERY_H
#define RENAULT_ZOE_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for LED defines
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define ABSOLUTE_MAX_VOLTAGE \
  4040  // 404.4V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 3100  // 310.0V if battery voltage goes under this, discharging further is disabled
#define ABSOLUTE_CELL_MAX_VOLTAGE \
  4100  // Max Cell Voltage mV! if voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_CELL_MIN_VOLTAGE \
  3000                             // Min Cell Voltage mV! if voltage goes under this, discharging further is disabled
#define MAX_CELL_DEVIATION_MV 500  //LED turns yellow on the board if mv delta exceeds this value

// These parameters need to be mapped for the Gen24
extern uint16_t SOC;                         //SOC%, 0-100.00 (0-10000)
extern uint16_t StateOfHealth;               //SOH%, 0-100.00 (0-10000)
extern uint16_t battery_voltage;             //V+1,  0-500.0 (0-5000)
extern uint16_t battery_current;             //A+1,  Goes thru convert2unsignedint16 function (5.0A = 50, -5.0A = 65485)
extern uint16_t capacity_Wh;                 //Wh,   0-60000
extern uint16_t remaining_capacity_Wh;       //Wh,   0-60000
extern uint16_t max_target_discharge_power;  //W,    0-60000
extern uint16_t max_target_charge_power;     //W,    0-60000
extern uint16_t bms_status;                  //Enum, 0-5
extern uint16_t bms_char_dis_status;         //Enum, 0-2
extern uint16_t stat_batt_power;             //W,    Goes thru convert2unsignedint16 function (5W = 5, -5W = 65530)
extern uint16_t temperature_min;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;  //mV,   0-4350
extern uint16_t cell_min_voltage;  //mV,   0-4350
extern uint8_t LEDcolor;           //Enum, 0-10
extern bool batteryAllowsContactorClosing;  //Bool, 1=true, 0=false
// Definitions for BMS status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

void update_values_zoe_battery();
void receive_can_zoe_battery(CAN_frame_t rx_frame);
void send_can_zoe_battery();
uint16_t convert2uint16(int16_t signed_value);

#endif
