#include "USER_SETTINGS.h"
#include <string>
#include "USER_SECRETS.h"
#include "src/devboard/hal/hal.h"

/**
 * Configuration file for battery settings, WiFi, CAN, MQTT, and other device behaviors.
 * Adjust settings as needed, or manage them later in the WebUI.
 */

/* ------------------------- CAN Configuration ------------------------- */
/* CAN interface selection for each connected component. */
volatile CAN_Configuration can_config = {
    .battery = CAN_NATIVE,               // Battery CAN interface
    .inverter = CAN_NATIVE,              // Inverter CAN interface (use RS485 if not configured)
    .battery_double = CAN_ADDON_MCP2515, // Optional: Second battery CAN interface
    .charger = CAN_NATIVE                // Optional: Charger CAN interface
};

/* ------------------------- WiFi Settings ------------------------- */
std::string ssid = WIFI_SSID;             // WiFi SSID, defined in USER_SECRETS.h
std::string password = WIFI_PASSWORD;     // WiFi password, defined in USER_SECRETS.h
const char* ssidAP = "Battery Emulator";  // Max 63 chars, also used as device name in WebUI
const char* passwordAP = AP_PASSWORD;     // Access Point password, defined in USER_SECRETS.h
const uint8_t wifi_channel = 0;           // 0 for automatic channel selection

/* ------------------------- Web Server Settings ------------------------- */
#ifdef WEBSERVER
const char* http_username = HTTP_USERNAME;  // Web server username, defined in USER_SECRETS.h
const char* http_password = HTTP_PASSWORD;  // Web server password, defined in USER_SECRETS.h

// Static IP configuration (used only if WIFICONFIG is enabled in USER_SETTINGS.h)
IPAddress local_IP(192, 168, 10, 150);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);
#endif  // WEBSERVER

/* ------------------------- MQTT Settings ------------------------- */
#ifdef MQTT
const char* mqtt_user = MQTT_USER;          // MQTT username, defined in USER_SECRETS.h
const char* mqtt_password = MQTT_PASSWORD;  // MQTT password, defined in USER_SECRETS.h

#ifdef MQTT_MANUAL_TOPIC_OBJECT_NAME
const char* mqtt_topic_name = "BE";         // Custom MQTT topic name
const char* mqtt_object_id_prefix = "be_";  // Custom prefix for MQTT object IDs
const char* mqtt_device_name = "Battery Emulator";  // Custom device name in Home Assistant
#endif  // MQTT_MANUAL_TOPIC_OBJECT_NAME
#endif  // MQTT

/* ------------------------- Equipment Stop Button Settings ------------------------- */
#ifdef EQUIPMENT_STOP_BUTTON
// Define equipment stop button behavior for safety:
// LATCHING_SWITCH: Normally closed (NC) latching switch, activates e-stop when pressed.
// MOMENTARY_SWITCH: Short press activates e-stop; long (15s) press deactivates it. Persistent between reboots.
volatile STOP_BUTTON_BEHAVIOR equipment_stop_behavior = LATCHING_SWITCH;
#endif

/* ------------------------- Charger Settings ------------------------- */
/* Charger settings for optional generator charging. */
volatile float CHARGER_SET_HV = 384.0f;   // Default charging voltage (VDC) for a 96s pack
volatile float CHARGER_MAX_HV = 420.0f;   // Max permissible charger output voltage (VDC)
volatile float CHARGER_MIN_HV = 200.0f;   // Min permissible charger output voltage (VDC)
volatile float CHARGER_MAX_POWER = 3300;  // Max charger power (W)
volatile float CHARGER_MAX_A = 11.5f;     // Max charger current output (A)
volatile float CHARGER_END_A = 1.0f;      // Current threshold for charge completion (A)
