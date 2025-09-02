#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__
#include <WiFi.h>
#include <stdint.h>
#include "src/devboard/utils/types.h"

/* Shunt/Contactor settings (Optional) */
//#define BMW_SBOX  // SBOX relay control & battery current/voltage measurement

/* Connectivity options */
//#define WIFICONFIG  //Enable this line to set a static IP address / gateway /subnet mask for the device. see USER_SETTINGS.cpp for the settings

extern volatile float CHARGER_SET_HV;
extern volatile float CHARGER_MAX_HV;
extern volatile float CHARGER_MIN_HV;
extern volatile float CHARGER_MAX_POWER;
extern volatile float CHARGER_MAX_A;
extern volatile float CHARGER_END_A;

#ifdef WIFICONFIG
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
#endif

#endif  // __USER_SETTINGS_H__
