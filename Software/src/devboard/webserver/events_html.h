#ifndef EVENTS_H
#define EVENTS_H

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include "../utils/events.h"

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String events_processor(const String& var);
// Define a struct to hold event data
struct EventData {
  EVENTS_ENUM_TYPE event_handle;
  const EVENTS_STRUCT_TYPE* event_pointer;
};

#endif
