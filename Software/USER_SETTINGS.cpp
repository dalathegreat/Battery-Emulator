#include "USER_SETTINGS.h"
#include <string>
#include "USER_SECRETS.h"
#include "src/devboard/hal/hal.h"

/* This file contains all the battery settings and limits */
/* They can be defined here, or later on in the WebUI */
/* Most important is to select which CAN interface each component is connected to */
/* 
CAN_NATIVE = Native CAN port on the LilyGo & Stark hardware
CANFD_NATIVE = Native CANFD port on the Stark CMR hardware
CAN_ADDON_MCP2515 = Add-on CAN MCP2515 connected to GPIO pins
CANFD_ADDON_MCP2518 = Add-on CAN-FD MCP2518 connected to GPIO pins
*/

volatile CAN_Configuration can_config = {
    .battery = CAN_NATIVE,   // Which CAN is your battery connected to?
    .inverter = CAN_NATIVE,  // Which CAN is your inverter connected to? (No need to configure incase you use RS485)
    .battery_double = CAN_ADDON_MCP2515,  // (OPTIONAL) Which CAN is your second battery connected to?
    .charger = CAN_NATIVE,                // (OPTIONAL) Which CAN is your charger connected to?
    .shunt = CAN_NATIVE                   // (OPTIONAL) Which CAN is your shunt connected to?
};

#ifdef COMMON_IMAGE
std::string ssid;
std::string password;
std::string passwordAP;
#else
std::string ssid = WIFI_SSID;          // Set in USER_SECRETS.h
std::string password = WIFI_PASSWORD;  // Set in USER_SECRETS.h
std::string passwordAP = AP_PASSWORD;  // Set in USER_SECRETS.h
#endif

const uint8_t wifi_channel = 0;  // Set to 0 for automatic channel selection

#ifdef COMMON_IMAGE
std::string http_username;
std::string http_password;
#else
std::string http_username = HTTP_USERNAME;  // Set in USER_SECRETS.h
std::string http_password = HTTP_PASSWORD;  // Set in USER_SECRETS.h
#endif

// Set your Static IP address. Only used incase WIFICONFIG is set in USER_SETTINGS.h
IPAddress local_IP(192, 168, 10, 150);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

// MQTT
#ifdef COMMON_IMAGE
std::string mqtt_user;
std::string mqtt_password;
#else
std::string mqtt_user = MQTT_USER;          // Set in USER_SECRETS.h
std::string mqtt_password = MQTT_PASSWORD;  // Set in USER_SECRETS.h
#endif

const char* mqtt_topic_name =
    "BE";  // Custom MQTT topic name. Previously, the name was automatically set to "battery-emulator_esp32-XXXXXX"
const char* mqtt_object_id_prefix =
    "be_";  // Custom prefix for MQTT object ID. Previously, the prefix was automatically set to "esp32-XXXXXX_"
const char* mqtt_device_name =
    "Battery Emulator";  // Custom device name in Home Assistant. Previously, the name was automatically set to "BatteryEmulator_esp32-XXXXXX"
const char* ha_device_id =
    "battery-emulator";  // Custom device ID in Home Assistant. Previously, the ID was always "battery-emulator"

/* Charger settings (Optional, when using generator charging) */
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete

#ifdef PERIODIC_BMS_RESET_AT
// A list of rules for your zone can be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
const char* time_zone =
    "GMT0BST,M3.5.0/1,M10.5.0";  // TimeZone rule for Europe/London including daylight adjustment rules (optional)
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
#endif
