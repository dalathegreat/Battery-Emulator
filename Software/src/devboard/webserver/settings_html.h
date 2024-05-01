#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

#include "../../../USER_SETTINGS.h"  // Needed for WiFi ssid and password

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String settings_processor(const String& var);

#endif
