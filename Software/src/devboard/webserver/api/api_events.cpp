// Event endpoints:
//   GET  /api/events       -> EventsResponse (sorted newest first)
//   POST /api/events/clear -> { ok: true }
// Shape: frontend/src/types/events.types.ts

#include <algorithm>
#include <vector>
#include "../../utils/events.h"
#include "../../utils/millis64.h"
#include "api_common.h"
#include "api_routes.h"

static void handle_api_events(AsyncWebServerRequest* request) {
  std::vector<EventData> ordered_events;
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (event_pointer->occurences > 0) {
      ordered_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }
  std::sort(ordered_events.begin(), ordered_events.end(), compareEventsByTimestampDesc);

  uint64_t current_timestamp = millis64();

  JsonDocument doc;
  JsonArray events = doc["events"].to<JsonArray>();
  for (const auto& event : ordered_events) {
    JsonObject o = events.add<JsonObject>();
    o["type"] = get_event_enum_string(event.event_handle);
    o["severity"] = get_event_level_string(event.event_handle);
    o["age_ms"] = current_timestamp - event.event_pointer->timestamp;
    o["count"] = event.event_pointer->occurences;
    o["data"] = event.event_pointer->data;
    o["message"] = get_event_message_string(event.event_handle);
  }

  api_send_json(request, doc);
}

static void handle_api_events_clear(AsyncWebServerRequest* request) {
  reset_all_events();
  JsonDocument doc;
  doc["ok"] = true;
  api_send_json(request, doc);
}

void init_api_events_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/events", server, HTTP_GET, handle_api_events);
  def_route_with_auth("/api/events/clear", server, HTTP_POST, handle_api_events_clear);
}
