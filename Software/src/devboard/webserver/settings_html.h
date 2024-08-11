#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <string>

extern std::string ssid;
extern std::string password;

#include "../../../USER_SETTINGS.h"  // Needed for WiFi ssid and password

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String settings_processor(const String& var);
/**
 * @brief Maps the value to a string of characters
 *
 * @param[in] char
 *
 * @return String
 */
const char* getCANInterfaceName(CAN_Interface interface);

#endif
