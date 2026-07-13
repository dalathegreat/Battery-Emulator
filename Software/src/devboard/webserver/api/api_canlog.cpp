// CAN log endpoints:
//   GET  /api/canlog/status      -> CanlogStatusResponse
//   POST /api/can/viewer/enable  -> { ok: true } (starts web CAN logging)
//   GET  /api/can/messages       -> CanMessagesResponse
// Shapes: frontend/src/types/system.types.ts / canlog.types.ts

#include "../../../communication/can/comm_can.h"
#include "../../../datalayer/datalayer.h"
#include "api_common.h"
#include "api_routes.h"

// Defined in webserver.cpp (CAN replay state)
extern String importedLogs;
extern bool isReplayRunning;

static void handle_api_canlog_status(AsyncWebServerRequest* request) {
  JsonDocument doc;
  doc["can_logging_active"] = datalayer.system.info.can_logging_active;
  doc["can_usb_logging"] = datalayer.system.info.CAN_usb_logging_active;
  doc["can_sd_logging"] = datalayer.system.info.CAN_SD_logging_active;
  doc["loop_playback"] = datalayer.system.info.loop_playback;
  doc["replay_running"] = isReplayRunning;
  doc["imported_log_chars"] = importedLogs.length();
  doc["can_replay_interface"] = datalayer.system.info.can_replay_interface;
  api_send_json(request, doc);
}

static void handle_api_can_viewer_enable(AsyncWebServerRequest* request) {
  // Same behavior as opening the legacy /canlog page: clear the buffer when
  // logging was inactive and signal the main loop to start logging.
  if (!datalayer.system.info.can_logging_active) {
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
  }
  datalayer.system.info.can_logging_active = true;

  JsonDocument doc;
  doc["ok"] = true;
  api_send_json(request, doc);
}

static void handle_api_can_messages(AsyncWebServerRequest* request) {
  JsonDocument doc;
  doc["text"] = datalayer.system.info.logged_can_messages;
  doc["can_id_cutoff"] = user_selected_CAN_ID_cutoff_filter;
  doc["can_logging_active"] = datalayer.system.info.can_logging_active;
  api_send_json(request, doc);
}

void init_api_canlog_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/canlog/status", server, HTTP_GET, handle_api_canlog_status);
  def_route_with_auth("/api/can/viewer/enable", server, HTTP_POST, handle_api_can_viewer_enable);
  def_route_with_auth("/api/can/messages", server, HTTP_GET, handle_api_can_messages);
}
