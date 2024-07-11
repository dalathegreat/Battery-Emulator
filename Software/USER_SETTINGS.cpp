#include "USER_SETTINGS.h"
#include <string>
/* This file contains all the battery settings and limits */
/* They can be defined here, or later on in the WebUI */

/* Charger settings (Optional, when using generator charging) */
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete

#ifdef WEBSERVER
volatile uint8_t AccessPointEnabled = true;  //Set to either true/false incase you want to enable direct wifi access point
std::string ssid = "REPLACE_WITH_YOUR_SSID";          // Maximum of 63 characters;
std::string password = "REPLACE_WITH_YOUR_PASSWORD";  // Minimum of 8 characters;
const char* ssidAP = "Battery Emulator";              // Maximum of 63 characters;
const char* passwordAP = "123456789";  // Minimum of 8 characters; set to NULL if you want the access point to be open
const uint8_t wifi_channel = 0;        // set to 0 for automatic channel selection

// MQTT
#ifdef MQTT
const char* mqtt_user = "REDACTED";
const char* mqtt_password = "REDACTED";
#endif  // USE_MQTT

#endif
