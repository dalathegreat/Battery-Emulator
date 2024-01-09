#ifndef TESLA_MODEL_3_BATTERY_H
#define TESLA_MODEL_3_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define ABSOLUTE_MAX_VOLTAGE \
  4030  // 403.0V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 2450    // 245.0V if battery voltage goes under this, discharging further is disabled
#define MAXCHARGEPOWERALLOWED 15000  // 15000W we use a define since the value supplied by Tesla is always 0
#define MAXDISCHARGEPOWERALLOWED \
  60000  // 60000W we need to cap this value to max 60kW, most inverters overflow otherwise

// These parameters need to be mapped for the Inverter
extern uint16_t SOC;                         //SOC%, 0-100.00 (0-10000)
extern uint16_t StateOfHealth;               //SOH%, 0-100.00 (0-10000)
extern uint16_t battery_voltage;             //V+1,  0-500.0 (0-5000)
extern uint16_t battery_current;             //A+1,  Goes thru convert2unsignedint16 function (5.0A = 50, -5.0A = 65485)
extern uint16_t capacity_Wh;                 //Wh,   0-60000
extern uint16_t remaining_capacity_Wh;       //Wh,   0-60000
extern uint16_t max_target_discharge_power;  //W,    0-60000
extern uint16_t max_target_charge_power;     //W,    0-60000
extern uint8_t bms_status;                   //Enum, 0-5
extern uint8_t bms_char_dis_status;          //Enum, 0-2
extern uint16_t stat_batt_power;             //W,    Goes thru convert2unsignedint16 function (5W = 5, -5W = 65530)
extern uint16_t temperature_min;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;  //mV,   0-4350
extern uint16_t cell_min_voltage;  //mV,   0-4350
extern uint8_t LEDcolor;           //Enum, 0-10
extern bool batteryAllowsContactorClosing;   //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false
extern bool LFP_Chemistry;

void update_values_tesla_model_3_battery();
void receive_can_tesla_model_3_battery(CAN_frame_t rx_frame);
void send_can_tesla_model_3_battery();
void printFaultCodesIfActive();
void printDebugIfActive(uint8_t symbol, const char* message);
void print_int_with_units(char* header, int value, char* units);
void print_SOC(char* header, int SOC);
uint16_t convert2unsignedInt16(int16_t signed_value);

#endif
