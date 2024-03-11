#ifndef EVENTS_A
#define EVENTS_A

#include <Arduino.h>
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/AsyncJson.h"

/**
 * @brief Maps Event data to their JSON equivalent  to be returned as part of an api endpoint response
 *
 * @param[in] void
 *
 * @return AsyncJsonResponse pointer
 */
AsyncJsonResponse* eventsAPI_GET();

#endif
