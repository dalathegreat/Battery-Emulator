#pragma once

#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

void register_dump_can_route(AsyncWebServer& server);
void can_dump_drain_tick();
