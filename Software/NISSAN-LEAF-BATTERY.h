#ifndef NISSAN_LEAF_BATTERY_H
#define NISSAN_LEAF_BATTERY_H
#include <Arduino.h>
#include "ESP32CAN.h"

#define BATTERY_WH_MAX 30000 //Battery size in Wh (Maximum value Fronius accepts is 60000 [60kWh] you can use larger batteries but do set value over 60000
#define ABSOLUTE_MAX_VOLTAGE 4040 // 404.4V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 3100 // 310.0V if battery voltage goes under this, discharging further is disabled
#define MAXPERCENTAGE 800 //80.0% , Max percentage the battery will charge to (App will show 100% once this value is reached) - 96% is the maximum the battery reaches while in the car
#define MINPERCENTAGE 200 //20.0% , Min percentage the battery will discharge to (App will show 0% once this value is reached) - 6% is the minimum the battery reaches while in the car - turtle mode
//#define INTERLOCK_REQUIRED //Uncomment this line to skip requiring both high voltage connectors to be seated on the LEAF battery
static byte printValues = 0; //Should modbus values be printed to serial output? 

// These parameters need to be mapped for the Gen24
extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t battery_voltage;
extern uint16_t battery_current;
extern uint16_t capacity_Wh;
extern uint16_t remaining_capacity_Wh;
extern uint16_t max_target_discharge_power;
extern uint16_t max_target_charge_power;
extern uint16_t bms_status;
extern uint16_t bms_char_dis_status;
extern uint16_t stat_batt_power;
extern uint16_t temperature_min;
extern uint16_t temperature_max;
extern uint8_t batteryAllowsContactorClosing;
extern uint8_t LEDcolor;
// Definitions for BMS status
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

void update_values_leaf_battery();
void receive_can_leaf_battery(CAN_frame_t rx_frame);
void send_can_leaf_battery();
uint16_t convert2unsignedint16(uint16_t signed_value);
uint16_t Temp_fromRAW_to_F(uint16_t temperature);
bool is_message_corrupt(CAN_frame_t rx_frame);

#endif
