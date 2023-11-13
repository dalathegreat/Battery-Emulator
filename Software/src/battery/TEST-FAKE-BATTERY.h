#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define ABSOLUTE_MAX_VOLTAGE \
  4040  // 404.4V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 3100  // 310.0V if battery voltage goes under this, discharging further is disabled

// These parameters need to be mapped for the inverter
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
extern uint8_t batteryAllowsContactorClosing;  //Bool, 1=true, 0=false
extern uint8_t LEDcolor;                       //Enum, 0-2
// Definitions for bms_status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5
// LED colors
#define GREEN 0
#define YELLOW 1
#define RED 2
#define BLUE 3
#define TEST_ALL_COLORS 10

void update_values_test_battery();
void receive_can_test_battery(CAN_frame_t rx_frame);
void send_can_test_battery();

#endif
