#pragma once

#include <Arduino.h>
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

void print_cellmonitor_html(AsyncResponseStream* response);
