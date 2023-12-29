#ifndef WEBSERVER_H
#define WEBSERVER_H

// Load Wi-Fi library
#include <WiFi.h>
#include "../../../USER_SETTINGS.h"  // Needed for WiFi ssid and password
#include "../../lib/ayushsharma82-ElegantOTA/src/ElegantOTA.h"
#include "../../lib/me-no-dev-AsyncTCP/src/AsyncTCP.h"
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../config.h"  // Needed for LED defines

extern uint16_t SOC;                         //SOC%, 0-100.00 (0-10000)
extern uint16_t StateOfHealth;               //SOH%, 0-100.00 (0-10000)
extern uint16_t battery_voltage;             //V+1,  0-500.0 (0-5000)
extern uint16_t battery_current;             //A+1,  Goes thru convert2unsignedint16 function (5.0A = 50, -5.0A = 65485)
extern uint16_t capacity_Wh;                 //Wh,   0-60000
extern uint16_t remaining_capacity_Wh;       //Wh,   0-60000
extern uint16_t max_target_discharge_power;  //W,    0-60000
extern uint16_t max_target_charge_power;     //W,    0-60000
extern uint16_t bms_status;                  //Enum, 0-5
extern uint16_t bms_char_dis_status;         //Enum, 0-2
extern uint16_t stat_batt_power;             //W,    Goes thru convert2unsignedint16 function (5W = 5, -5W = 65530)
extern uint16_t temperature_min;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max;   //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;  //mV,   0-4350
extern uint16_t cell_min_voltage;  //mV,   0-4350
extern uint8_t LEDcolor;           //Enum, 0-10
extern bool batteryAllowsContactorClosing;  //Bool, 1=true, 0=false

extern const char* ssid;
extern const char* password;
extern const char* ssidAP;
extern const char* passwordAP;

/**
 * @brief Initialization function for the webserver.
 *
 * @param[in] void
 *
 * @return void
 */
void init_webserver();

/**
 * @brief Initialization function that creates a WiFi Access Point.
 *
 * @param[in] void
 * 
 * @return void
 */
void init_WiFi_AP();

/**
 * @brief Initialization function that connects to an existing network.
 *
 * @param[in] ssid WiFi network name
 * @param[in] password WiFi network password
 * 
 * @return void
 */
void init_WiFi_STA(const char* ssid, const char* password);

/**
 * @brief Initialization function for ElegantOTA.
 *
 * @param[in] void
 * 
 * @return void
 */
void init_ElegantOTA();

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String processor(const String& var);

/**
 * @brief Executes on OTA start 
 *
 * @param[in] void
 *
 * @return void
 */
void onOTAStart();

/**
 * @brief Executes on OTA progress 
 * 
 * @param[in] current Current bytes
 * @param[in] final Final bytes
 *
 * @return void
 */
void onOTAProgress(size_t current, size_t final);

/**
 * @brief Executes on OTA end 
 *
 * @param[in] void
 * 
 * @return bool success: success = true, failed = false
 */
void onOTAEnd(bool success);

#endif
