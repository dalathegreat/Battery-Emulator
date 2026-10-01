#pragma once

#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

void can_dump_drain_tick();
void register_dump_can_route(AsyncWebServer& server);
void stream_can_frame(const CAN_frame& frame, CAN_Interface interface, frameDirection msgDir);
