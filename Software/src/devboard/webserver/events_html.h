#pragma once

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../utils/events.h"

void print_events_html(AsyncResponseStream* response);
