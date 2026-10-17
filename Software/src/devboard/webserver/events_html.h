#ifndef EVENTS_H
#define EVENTS_H

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include "../utils/events.h"

class AsyncWebServerRequest;

/**
 * @brief Sends the event log page, or with a "since" parameter only its rows: none (204) while
 *        the events are unchanged since the version given, else the current rows.
 *
 * @param[in] request
 */
void send_events_page(AsyncWebServerRequest* request);

#endif
