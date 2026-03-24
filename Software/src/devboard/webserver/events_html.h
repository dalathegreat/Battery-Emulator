#ifndef EVENTS_H
#define EVENTS_H

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include "../utils/events.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String events_processor(const String& var);
void print_events_html(AsyncResponseStream *response);

#endif
