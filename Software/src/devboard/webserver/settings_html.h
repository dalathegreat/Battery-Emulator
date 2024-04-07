#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

#include "../../../USER_SETTINGS.h"         // Needed for WiFi ssid and password
extern uint16_t system_battery_voltage_dV;  //V+1,  0-1000.0 (0-10000)

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String settings_processor(const String& var);

#endif
