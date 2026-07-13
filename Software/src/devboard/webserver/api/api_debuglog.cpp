// GET /api/debug/log -> { text: string }
// Returns the in-RAM debug log buffer (same source as the legacy /log page,
// see debug_logging_html.cpp for the ring-buffer unwrap logic mirrored here).

#include "../../../datalayer/datalayer.h"
#include "api_common.h"
#include "api_routes.h"

// Like strchr, but bounded to the first 'n' bytes.
static const char* api_strnchr(const char* s, int c, size_t n) {
  if (s == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < n && s[i] != '\0'; ++i) {
    if (s[i] == c) {
      return &s[i];
    }
  }
  return NULL;
}

static void handle_api_debug_log(AsyncWebServerRequest* request) {
  String text;
  if (!text.reserve(sizeof(datalayer.system.info.logged_can_messages) + 16)) {
    request->send(500, "application/json", "{\"text\":\"Out of memory.\"}");
    return;
  }

  size_t offset = datalayer.system.info.logged_can_messages_offset;
  // If we're mid-buffer, output the older (wrapped) part first, starting at
  // the next full line after the current write offset.
  if (offset > 0 && offset < (sizeof(datalayer.system.info.logged_can_messages) - 1)) {
    const char* next_newline = api_strnchr(&datalayer.system.info.logged_can_messages[offset + 1], '\n',
                                           sizeof(datalayer.system.info.logged_can_messages) - offset - 1);
    if (next_newline != NULL) {
      text.concat(next_newline + 1,
                  strnlen(next_newline + 1, sizeof(datalayer.system.info.logged_can_messages) - offset - 2));
    } else {
      text.concat(&datalayer.system.info.logged_can_messages[offset + 1],
                  strnlen(&datalayer.system.info.logged_can_messages[offset + 1],
                          sizeof(datalayer.system.info.logged_can_messages) - offset - 1));
    }
  }
  text.concat(datalayer.system.info.logged_can_messages, offset);

  JsonDocument doc;
  doc["text"] = text;
  api_send_json(request, doc);
}

void init_api_debuglog_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/debug/log", server, HTTP_GET, handle_api_debug_log);
}
