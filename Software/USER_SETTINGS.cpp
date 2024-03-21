#include "USER_SETTINGS.h"

/* This file contains all the battery settings and limits */
/* They can be defined here, or later on in the WebUI */

/* Battery settings */
volatile bool USE_SCALED_SOC =
    true;  //Increases battery life. If true will rescale SOC between the configured min/max-percentage
volatile uint32_t BATTERY_WH_MAX = 30000;  //Battery size in Wh
volatile uint16_t MAXPERCENTAGE =
    800;  //80.0% , Max percentage the battery will charge to (Inverter gets 100% when reached)
volatile uint16_t MINPERCENTAGE =
    200;  //20.0% , Min percentage the battery will discharge to (Inverter gets 0% when reached)
volatile uint16_t MAXCHARGEAMP =
    300;  //30.0A , BYD CAN specific setting, Max charge in Amp (Some inverters needs to be limited)
volatile uint16_t MAXDISCHARGEAMP =
    300;  //30.0A , BYD CAN specific setting, Max discharge in Amp (Some inverters needs to be limited)

/* Charger settings (Optional, when generator charging) */
/* Charger settings */
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete

#ifdef WEBSERVER
volatile uint8_t AccessPointEnabled =
    true;  //Set to either true or false incase you want the board to enable a direct wifi access point
const char* ssid = "ZTE_5G_VACCINE";          // Maximum of 63 characters;
const char* password = "secretpassword";  // Minimum of 8 characters;
const char* ssidAP = "Battery Emulator";              // Maximum of 63 characters;
const char* passwordAP = "123456789";  // Minimum of 8 characters; set to NULL if you want the access point to be open
const uint8_t wifi_channel = 0;        // set to 0 for automatic channel selection

// MQTT
#ifdef MQTT
const char* mqtt_user = "REDACTED";
const char* mqtt_password = "REDACTED";
#endif  // USE_MQTT

#endif
