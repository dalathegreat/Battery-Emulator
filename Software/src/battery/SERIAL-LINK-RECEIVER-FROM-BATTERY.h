// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

//  https://github.com/mackelec/SerialDataLink

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
extern uint8_t bms_status;                   //Enum, 0-5
extern uint8_t bms_char_dis_status;          //Enum, 0-2
extern uint16_t stat_batt_power;             //W,    Goes thru convert2unsignedint16 function (5W = 5, -5W = 65530)
extern uint16_t temperature_min;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;  //mV,   0-4350
extern uint16_t cell_min_voltage;  //mV,   0-4350
extern uint8_t LEDcolor;           //Enum, 0-10

extern bool LFP_Chemistry;
extern uint16_t CANerror;

extern bool batteryAllowsContactorClosing;  //Bool, 1=true, 0=false

// Parameters to send to the transmitter
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void manageSerialLinkReceiver();
void update_values_serial_link();

#endif
