#pragma once

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../utils/events.h"

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
void print_events_html(AsyncResponseStream* response);
