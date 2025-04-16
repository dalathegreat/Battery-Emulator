#include "api_events_html.h"
#include "../../../datalayer/datalayer.h"
#include "../../utils/events.h"
#include "api_helpers.h"

// Helper function to add an event entry to the JSON
void addEventEntry(String& json, EVENTS_ENUM_TYPE eventId, bool addComma) {
  json += "{";
  addJsonNumber(json, "id", eventId, true);

  const char* eventDesc = get_event_message_string(eventId);
  addJsonField(json, "description", eventDesc, true);

  const EVENTS_STRUCT_TYPE* eventPtr = get_event_pointer(eventId);

  // Calculate absolute timestamp
  unsigned long timestamp = eventPtr->timestamp;
  if (eventPtr->millisrolloverCount > 0) {
    // Handle rollover if needed
    timestamp += (unsigned long)eventPtr->millisrolloverCount * 0xFFFFFFFF;
  }

  addJsonNumber(json, "timestamp", timestamp, true);
  addJsonField(json, "level", get_event_level_string(eventId), true);
  addJsonNumber(json, "occurences", eventPtr->occurences, true);

  bool isActive = (eventPtr->state == EVENT_STATE_ACTIVE || eventPtr->state == EVENT_STATE_ACTIVE_LATCHED);
  addJsonBool(json, "active", isActive, false);

  json += "}";
  if (addComma)
    json += ",";
}

String api_events_processor() {
  String json = "{\"events\":[";

  // Counter for active events
  int eventCount = 0;
  int totalEvents = 0;

  // First count the total events to handle the commas correctly
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* eventPtr = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (eventPtr->occurences > 0) {
      totalEvents++;
    }
  }

  // Then add each event to the JSON
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* eventPtr = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (eventPtr->occurences > 0) {
      addEventEntry(json, (EVENTS_ENUM_TYPE)i, eventCount < totalEvents - 1);
      eventCount++;
    }
  }

  json += "]}";
  return json;
}
