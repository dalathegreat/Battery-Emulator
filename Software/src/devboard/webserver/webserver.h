#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Preferences.h>
#include <WiFi.h>
#include "../../include.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/YiannisBourkelis-Uptime-Library/src/uptime_formatter.h"
#include "../../lib/ayushsharma82-ElegantOTA/src/ElegantOTA.h"
#include "../../lib/mathieucarbou-AsyncTCPSock/src/AsyncTCP.h"

extern const char* version_number;  // The current software version, shown on webserver

#include <string>
extern const char* http_username;
extern const char* http_password;

extern const char* ssidAP;

// Common charger parameters
extern float charger_stat_HVcur;
extern float charger_stat_HVvol;
extern float charger_stat_ACcur;
extern float charger_stat_ACvol;
extern float charger_stat_LVcur;
extern float charger_stat_LVvol;

//LEAF charger
extern uint16_t OBC_Charge_Power;

/**
 * @brief Initialization function for the webserver.
 *
 * @param[in] void
 *
 * @return void
 */
void init_webserver();

// /**
//  * @brief Function to handle WiFi reconnection.
//  *
//  * @param[in] void
//  *
//  * @return void
//  */
// void handle_WiFi_reconnection(unsigned long currentMillis);

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
String get_firmware_info_processor(const String& var);

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

/**
 * @brief Formats power values 
 *
 * @param[in] float or uint16_t 
 * 
 * @return string: values
 */
template <typename T>
String formatPowerValue(String label, T value, String unit, int precision, String color = "white");

extern void store_settings();

void ota_monitor();

#endif
