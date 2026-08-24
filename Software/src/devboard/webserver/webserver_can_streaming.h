#pragma once

#include "../utils/types.h"

// Only register_dump_can_route() needs the web server, and a reference
// parameter needs no definition - so declaring it here keeps this header
// usable from the CAN layer, which wants stream_can_frame() and nothing else.
class AsyncWebServer;

void can_dump_drain_tick();
void register_dump_can_route(AsyncWebServer& server);
void stream_can_frame(const CAN_frame& frame, CAN_Interface interface, frameDirection msgDir);
