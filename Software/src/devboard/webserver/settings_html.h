#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../../USER_SETTINGS.h"  // Needed for WiFi ssid and password
extern uint16_t battery_voltage;     //V+1,  0-500.0 (0-5000)

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String settings_processor(const String& var);

#endif
