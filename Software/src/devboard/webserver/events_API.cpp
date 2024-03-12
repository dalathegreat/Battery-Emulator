#include "events_API.h"
#include <Arduino.h>
#include "../utils/events.h"

AsyncJsonResponse* eventsAPI_GET() {

  AsyncJsonResponse* response = new AsyncJsonResponse();
  JsonObject root = response->getRoot();

  const EVENTS_STRUCT_TYPE* event_pointer;
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    EVENTS_ENUM_TYPE event_handle = static_cast<EVENTS_ENUM_TYPE>(i);

    Serial.println("Event: " + String(get_event_enum_string(event_handle)) +
                   " count: " + String(event_pointer->occurences) + " seconds: " + String(event_pointer->timestamp) +
                   " data: " + String(event_pointer->data) + " level: " + String(get_event_level_string(event_handle)));

    if (event_pointer->occurences > 0) {
      JsonObject nested = root.createNestedObject(String(get_event_enum_string(event_handle)));
      nested["Level"] = String(get_event_level_string(event_handle));
      nested["Last event seconds ago"] = String(event_pointer->timestamp);
      nested["Occurances"] = String(event_pointer->occurences);
      nested["Data"] = String(event_pointer->data);
      nested["Message"] = String(get_event_message_string(event_handle));
      nested["Timestamp"] = String(millis() / 1000);
    }
  }
  return response;
}
