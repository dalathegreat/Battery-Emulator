#include "USER_SETTINGS.h"

/* This file contains all the battery settings and limits */
/* They can be defined here, or later on in the WebUI */

/* Battery settings */
volatile uint16_t BATTERY_WH_MAX =
    30000;  //Battery size in Wh (Maximum value for most inverters is 65000 [65kWh], you can use larger batteries but do not set value over 65000!
volatile uint16_t MAXPERCENTAGE =
    800;  //80.0% , Max percentage the battery will charge to (App will show 100% once this value is reached)
volatile uint16_t MINPERCENTAGE =
    200;  //20.0% , Min percentage the battery will discharge to (App will show 0% once this value is reached)
volatile uint16_t MAXCHARGEAMP =
    300;  //30.0A , BYD CAN specific setting, Max charge speed in Amp (Some inverters needs to be artificially limited)
volatile uint16_t MAXDISCHARGEAMP =
    300;  //30.0A , BYD CAN specific setting, Max discharge speed in Amp (Some inverters needs to be artificially limited)

// MQTT
// For more detailed settings, see mqtt.h
#ifdef MQTT
const char* mqtt_user = "REDACTED";
const char* mqtt_password = "REDACTED";
#endif  // USE_MQTT

#ifdef WEBSERVER
volatile uint8_t AccessPointEnabled =
    true;  //Set to either true or false incase you want the board to enable a direct wifi access point
const char* ssid = "REPLACE_WITH_YOUR_SSID";          // Maximum of 63 characters;
const char* password = "REPLACE_WITH_YOUR_PASSWORD";  // Minimum of 8 characters;
const char* ssidAP = "Battery Emulator";              // Maximum of 63 characters;
const char* passwordAP = "123456789";  // Minimum of 8 characters; set to NULL if you want the access point to be open
const char* versionNumber = "4.4.0";   // The current software version, shown on webserver
#endif
