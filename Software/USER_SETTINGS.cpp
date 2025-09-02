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
CANFD_ADDON_MCP2518 = Add-on CAN-FD MCP2518 connected to GPIO pins
*/

volatile CAN_Configuration can_config = {
    .battery = CAN_NATIVE,   // Which CAN is your battery connected to?
    .inverter = CAN_NATIVE,  // Which CAN is your inverter connected to? (No need to configure incase you use RS485)
    .battery_double = CAN_ADDON_MCP2515,  // (OPTIONAL) Which CAN is your second battery connected to?
    .charger = CAN_NATIVE,                // (OPTIONAL) Which CAN is your charger connected to?
    .shunt = CAN_NATIVE                   // (OPTIONAL) Which CAN is your shunt connected to?
};

// Set your Static IP address. Only used incase WIFICONFIG is set in USER_SETTINGS.h
IPAddress local_IP(192, 168, 10, 150);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

std::string ssid;
std::string password;
std::string passwordAP;

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
