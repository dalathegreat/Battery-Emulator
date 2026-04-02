#pragma once

#include <Arduino.h>
#include <string>
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

void print_debug_logger_html(AsyncResponseStream* response);
