#ifndef API_STATUS_HTML_H
#define API_STATUS_HTML_H

#include <Arduino.h>

/**
 * @brief Creates JSON string with battery and system status
 *
 * @return String JSON-formatted string with system data
 */
String api_status_processor();

#endif