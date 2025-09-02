#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__
#include <WiFi.h>
#include <stdint.h>
#include "src/devboard/utils/types.h"

/* Shunt/Contactor settings (Optional) */
//#define BMW_SBOX  // SBOX relay control & battery current/voltage measurement

/* Connectivity options */
//#define WIFICONFIG  //Enable this line to set a static IP address / gateway /subnet mask for the device. see USER_SETTINGS.cpp for the settings

/* Do not change any code below this line */
/* Only change battery specific settings above and in "USER_SETTINGS.cpp" */
typedef struct {
  CAN_Interface battery;
  CAN_Interface inverter;
  CAN_Interface battery_double;
  CAN_Interface charger;
  CAN_Interface shunt;
} CAN_Configuration;
extern volatile CAN_Configuration can_config;
extern volatile uint8_t AccessPointEnabled;
extern const uint8_t wifi_channel;
extern volatile float CHARGER_SET_HV;
extern volatile float CHARGER_MAX_HV;
extern volatile float CHARGER_MIN_HV;
extern volatile float CHARGER_MAX_POWER;
extern volatile float CHARGER_MAX_A;
extern volatile float CHARGER_END_A;

#include "src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"

// Equipment stop button behavior. Use NC button for safety reasons.
//LATCHING_SWITCH  - Normally closed (NC), latching switch. When pressed it activates e-stop
//MOMENTARY_SWITCH - Short press to activate e-stop, long 15s press to deactivate. E-stop is persistent between reboots

#ifdef EQUIPMENT_STOP_BUTTON
const STOP_BUTTON_BEHAVIOR stop_button_default_behavior = STOP_BUTTON_BEHAVIOR::MOMENTARY_SWITCH;
#else
const STOP_BUTTON_BEHAVIOR stop_button_default_behavior = STOP_BUTTON_BEHAVIOR::NOT_CONNECTED;
#endif

#ifdef WIFICONFIG
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
#endif

#if defined(MEB_BATTERY)
#define PRECHARGE_CONTROL
#endif

#endif  // __USER_SETTINGS_H__
