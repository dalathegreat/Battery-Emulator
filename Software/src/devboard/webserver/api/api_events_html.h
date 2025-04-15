#ifndef API_EVENTS_HTML_H
#define API_EVENTS_HTML_H

#ifndef EVENT_MAX
#define EVENT_MAX 50  // Adjust this to match your project's event count
#endif

#include <Arduino.h>

/**
 * @brief Creates JSON string with event data
 *
 * @return String JSON-formatted string with event data
 */
String api_events_processor();

// Helper function to add an event entry to the JSON
void addEventEntry(String& json, int id, const String& description, unsigned long timestamp, bool active, bool addComma);

#endif