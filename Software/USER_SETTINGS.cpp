#include "USER_SETTINGS.h"
#include <string>
#include "src/devboard/hal/hal.h"

/* This file contains all the battery settings and limits */
/* They can be defined here, or later on in the WebUI */
/* Most important is to select which CAN interface each component is connected to */
/* 
CAN_NATIVE = Native CAN port on the LilyGo & Stark hardware
CANFD_NATIVE = Native CANFD port on the Stark CMR hardware
CAN_ADDON_MCP2515 = Add-on CAN MCP2515 connected to GPIO pins
CAN_ADDON_FD_MCP2518 = Add-on CAN-FD MCP2518 connected to GPIO pins
*/

volatile CAN_Configuration can_config = {
    .battery = CAN_NATIVE,   // Which CAN is your battery connected to?
    .inverter = CAN_NATIVE,  // Which CAN is your inverter connected to? (No need to configure incase you use RS485)
    .battery_double = CAN_ADDON_MCP2515,  // (OPTIONAL) Which CAN is your second battery connected to?
    .charger = CAN_NATIVE                 // (OPTIONAL) Which CAN is your charger connected to?
};

#ifdef WIFI

volatile uint8_t AccessPointEnabled = true;           //Set to either true/false to enable direct wifi access point
std::string ssid = "REPLACE_WITH_YOUR_SSID";          // Maximum of 63 characters
std::string password = "REPLACE_WITH_YOUR_PASSWORD";  // Minimum of 8 characters
const char* ssidAP = "Battery Emulator";  // Maximum of 63 characters, also used for device name on web interface
const char* passwordAP = "123456789";  // Minimum of 8 characters; set to NULL if you want the access point to be open
const uint8_t wifi_channel = 0;        // Set to 0 for automatic channel selection

#ifdef WIFICONFIG
// Set your Static IP address
IPAddress local_IP(192, 168, 10, 150);
// Set your Gateway IP address
IPAddress gateway(192, 168, 10, 1);
// Set your Subnet IP address
IPAddress subnet(255, 255, 255, 0);
#endif
#ifdef WEBSERVER
const char* http_username = "admin";  // username to webserver authentication;
const char* http_password = "admin";  // password to webserver authentication;

#endif  // WEBSERVER
// MQTT
#ifdef MQTT
const char* mqtt_user = "REDACTED";      // Set NULL for no username
const char* mqtt_password = "REDACTED";  // Set NULL for no password
#endif                                   // USE_MQTT
#endif                                   // WIFI

#ifdef EQUIPMENT_STOP_BUTTON
// Equipment stop button behavior. Use NC button for safety reasons.
//TOGGLE_SWITCH                - activated while pressed, deactivated when released, button state is reflected in the emulator state
//PERSISTENT_ACTIVATION_SWITCH - short press to activate, long press to deactivate, state is persisted between reboots
volatile STOP_BUTTON_BEHAVIOR equipment_stop_behavior = TOGGLE_SWITCH;
#endif

/* Charger settings (Optional, when using generator charging) */
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete
