#include "webserver.h"
#include <Preferences.h>
#include <base64.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <ctime>
#include <vector>

#include <WiFi.h>
#include <string>
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../battery/Shunt.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/safety/safety.h"
#include "../../inverter/INVERTERS.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../sdcard/sdcard.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"
#include "esp_task_wdt.h"
#include "html_escape.h"
#include "../../devboard/i2c/i2c_atecc.h"
#include "../mqtt/mqtt.h"

std::string http_username;
std::string http_password;

extern const char* version_number;

AsyncWebServer server(80);

// =========================================================================
// 🚀 Real-time CAN Stream Backend (ZERO Memory Fragmentation Mode)
// =========================================================================
SemaphoreHandle_t can_stream_mutex = NULL;

// (Raw Array) 4000 chars 100%!
#define CAN_BATCH_SIZE 4000
char can_batch_buffer[CAN_BATCH_SIZE];
size_t can_batch_len = 0;

void append_can_stream(const char* line) {
  if (!datalayer.system.info.can_logging_active)
    return;

  if (can_stream_mutex == NULL) {
    can_stream_mutex = xSemaphoreCreateMutex();
  }

  // Lock Task
  // if (xSemaphoreTake(can_stream_mutex, (TickType_t)10) == pdTRUE) {
  if (xSemaphoreTake(can_stream_mutex, 0) == pdTRUE) {
    size_t line_len = strlen(line);

    if (can_batch_len + line_len + 2 < CAN_BATCH_SIZE) {
      strcpy(can_batch_buffer + can_batch_len, line);
      can_batch_len += line_len;
      can_batch_buffer[can_batch_len++] = '\n';
      can_batch_buffer[can_batch_len] = '\0';
    }

    xSemaphoreGive(can_stream_mutex);
  }
}
// =========================================================================

unsigned long ota_progress_millis = 0;

#include "../../system_settings.h"
#include "advanced_battery_html.h"
#include "can_logging_html.h"
#include "can_replay_html.h"
#include "cellmonitor_html.h"
#include "debug_logging_html.h"
#include "events_html.h"
#include "index_html.h"
#include "settings_html.h"
#include "dashboard_html.h"

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

const char get_firmware_info_html[] = R"rawliteral(%X%)rawliteral";

String importedLogs = "";
bool isReplayRunning = false;
bool settingsUpdated = false;

CAN_frame currentFrame = {.FD = true, .ext_ID = false, .DLC = 64, .ID = 0x12F, .data = {0}};

String get_uptime();
String get_firmware_info_processor(const String& var);

const char subpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS System Info</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #121212; color: #ffffff; margin: 0; padding: 20px; }
    button { background-color: #505E67; color: white; border: none; padding: 10px 15px; cursor: pointer; border-radius: 8px; margin-bottom: 20px; font-weight: bold; }
    button:hover { background-color: #3A4A52; }
    table { width: 100%; border-collapse: collapse; background-color: #2D3F2F; border-radius: 10px; overflow: hidden; margin-top: 10px; }
    th, td { border: 1px solid #444; padding: 10px; text-align: left; }
    th { background-color: #1A251B; color: #4CAF50; }
    h2, h3, h4 { color: #4CAF50; }
    a { color: #2196F3; }
  </style>
</head>
<body>
  <button onclick="window.location.href='/'">⬅️ Back to Dashboard</button>
  <div id="content">%X%</div>
</body>
</html>
)rawliteral";

bool is_scanning = false;
String scan_results = "[]";

void wifiScanTask(void* param) {
  is_scanning = true;
  int n = WiFi.scanNetworks(false, false, false, 300);
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; ++i) {
    JsonObject net = arr.add<JsonObject>();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
  }
  serializeJson(doc, scan_results);
  WiFi.scanDelete();
  is_scanning = false;
  vTaskDelete(NULL);
}

int wifi_apply_state = 0;
String apply_ssid = "";
String apply_pass = "";
String applied_ip = "";
bool keep_ap_mode = true;

void wifiApplyTask(void* param) {
  wifi_mode_t old_mode = WiFi.getMode();
  String old_ssid = WiFi.SSID();
  String old_pass = WiFi.psk();

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  vTaskDelay(500 / portTICK_PERIOD_MS);

  WiFi.begin(apply_ssid.c_str(), apply_pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_apply_state = 2;
    applied_ip = WiFi.localIP().toString();

    BatteryEmulatorSettingsStore settings;
    settings.saveString("SSID", apply_ssid.c_str());
    settings.saveString("PASSWORD", apply_pass.c_str());
    settings.saveBool("WIFIAPENABLED", keep_ap_mode);
    settingsUpdated = true;

    vTaskDelay(6000 / portTICK_PERIOD_MS);
    if (!keep_ap_mode)
      WiFi.mode(WIFI_STA);
  } else {
    wifi_apply_state = 3;
    WiFi.disconnect();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (old_ssid.length() > 0)
      WiFi.begin(old_ssid.c_str(), old_pass.c_str());
    WiFi.mode(old_mode);
  }
  vTaskDelete(NULL);
}

bool use_sd_for_replay = false;
File uploadReplayFile;
size_t current_upload_size = 0;
const size_t MAX_RAM_REPLAY_SIZE = 100 * 1024;

void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len,
                      bool final) {
  if (!index) {
    logging.printf("Receiving replay file: %s\n", filename.c_str());
    current_upload_size = 0;
    importedLogs = "";
    use_sd_for_replay = false;

    uploadReplayFile = SD_MMC.open("/replay_temp.txt", FILE_WRITE);
    if (uploadReplayFile) {
      logging.println("[Hybrid Replay] SD Card detected. Streaming directly to SD.");
      use_sd_for_replay = true;
    } else {
      logging.println("[Hybrid Replay] No SD Card available. Falling back to RAM (Max 100KB limit).");
    }
  }

  current_upload_size += len;

  if (use_sd_for_replay) {
    if (uploadReplayFile)
      uploadReplayFile.write(data, len);
  } else {
    if (current_upload_size > MAX_RAM_REPLAY_SIZE) {
      if (final) {
        logging.println("ERROR: File too large for RAM! Upload aborted to prevent crash.");
        request->send(400, "text/plain", "File too large! Please insert an SD Card to upload files larger than 100KB.");
      }
      return;
    }
    importedLogs += String((char*)data).substring(0, len);
  }

  if (final) {
    if (use_sd_for_replay && uploadReplayFile)
      uploadReplayFile.close();
    if (!use_sd_for_replay && current_upload_size > MAX_RAM_REPLAY_SIZE)
      return;

    logging.printf("Upload Complete! Total Size: %d bytes\n", current_upload_size);
    request->send(200, "text/plain", "File uploaded successfully");
  }
}

void canReplayTask(void* param) {
  bool hasData = false;

  if (use_sd_for_replay) {
    File f = SD_MMC.open("/replay_temp.txt", FILE_READ);
    if (f && f.size() > 0) {
      hasData = true;
      f.close();
    }
  } else {
    if (importedLogs.length() > 0)
      hasData = true;
  }

  if (!hasData) {
    logging.println("No replay data found.");
    isReplayRunning = false;
    vTaskDelete(NULL);
    return;
  }

  do {
    float firstTimestamp = -1.0f;
    float lastTimestamp = 0.0f;
    bool firstMessageSent = false;

    File replayFile;
    int ramIndex = 0;

    if (use_sd_for_replay) {
      replayFile = SD_MMC.open("/replay_temp.txt", FILE_READ);
      if (!replayFile)
        break;
    }

    while (true) {
      String line = "";

      if (use_sd_for_replay) {
        if (!replayFile.available())
          break;
        line = replayFile.readStringUntil('\n');
      } else {
        if (ramIndex >= importedLogs.length())
          break;
        int nextIndex = importedLogs.indexOf('\n', ramIndex);
        if (nextIndex == -1) {
          line = importedLogs.substring(ramIndex);
          ramIndex = importedLogs.length();
        } else {
          line = importedLogs.substring(ramIndex, nextIndex);
          ramIndex = nextIndex + 1;
        }
      }

      line.trim();
      if (line.length() == 0)
        continue;

      int timeStart = line.indexOf("(") + 1;
      int timeEnd = line.indexOf(")");
      if (timeStart == 0 || timeEnd == -1)
        continue;

      float currentTimestamp = line.substring(timeStart, timeEnd).toFloat();

      if (firstTimestamp < 0)
        firstTimestamp = currentTimestamp;

      if (!firstMessageSent) {
        firstMessageSent = true;
        firstTimestamp = currentTimestamp;
      } else {
        float deltaT = (currentTimestamp - lastTimestamp) * 1000.0f;
        if (deltaT > 0 && deltaT < 10000) {
          vTaskDelay((int)deltaT / portTICK_PERIOD_MS);
        }
      }

      lastTimestamp = currentTimestamp;

      int interfaceStart = timeEnd + 2;
      int interfaceEnd = line.indexOf(" ", interfaceStart);
      if (interfaceEnd == -1)
        continue;

      int idStart = interfaceEnd + 1;
      int idEnd = line.indexOf(" [", idStart);
      if (idStart == -1 || idEnd == -1)
        continue;

      String messageID = line.substring(idStart, idEnd);
      int dlcStart = idEnd + 2;
      int dlcEnd = line.indexOf("]", dlcStart);
      if (dlcEnd == -1)
        continue;

      String dlc = line.substring(dlcStart, dlcEnd);
      int dataStart = dlcEnd + 2;
      String dataBytes = line.substring(dataStart);

      currentFrame.ID = strtol(messageID.c_str(), NULL, 16);
      currentFrame.DLC = dlc.toInt();

      int byteIndex = 0;
      char* token = strtok((char*)dataBytes.c_str(), " ");
      while (token != NULL && byteIndex < currentFrame.DLC) {
        currentFrame.data.u8[byteIndex++] = strtol(token, NULL, 16);
        token = strtok(NULL, " ");
      }

      currentFrame.FD = (datalayer.system.info.can_replay_interface == CANFD_NATIVE) ||
                        (datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518);
      currentFrame.ext_ID = (currentFrame.ID > 0x7F0);

      transmit_can_frame_to_interface(&currentFrame, (CAN_Interface)datalayer.system.info.can_replay_interface);
    }

    if (use_sd_for_replay && replayFile) {
      replayFile.close();
    }

  } while (datalayer.system.info.loop_playback);

  importedLogs = "";
  isReplayRunning = false;
  vTaskDelete(NULL);
}

// Global variable Cache on RAM
static bool auth_cached = false;
static bool cached_auth_enabled = false;
static String cached_expected_auth = "";

void load_auth_cache() {
  BatteryEmulatorSettingsStore auth_settings(true);
  cached_auth_enabled = (auth_settings.getUInt("WEBAUTH", 0) == 1);

  String u = auth_settings.getString("WEBUSER", DEFAULT_WEB_USER);
  String p = auth_settings.getString("WEBPASS", DEFAULT_WEB_PASS);
  if (u.length() == 0) u = DEFAULT_WEB_USER;
  if (p.length() < 4) p = DEFAULT_WEB_PASS;

  cached_expected_auth = "Basic " + base64::encode(u + ":" + p);
  auth_cached = true;
}

bool checkAuth(AsyncWebServerRequest* request) {
  if (!auth_cached) load_auth_cache();

  if (!cached_auth_enabled) return true;

  if (request->hasHeader("Authorization")) {
    if (request->getHeader("Authorization")->value().equals(cached_expected_auth))
      return true;
  }
  return false;
}

void def_route_with_auth(const char* uri, AsyncWebServer& serv, WebRequestMethodComposite method,
                         std::function<void(AsyncWebServerRequest*)> handler) {
  serv.on(uri, method, [handler, uri](AsyncWebServerRequest* request) {
    if (!checkAuth(request)) {
      AsyncWebServerResponse* response = request->beginResponse(401, "text/plain", "Authentication Required");
      response->addHeader("WWW-Authenticate", "Basic realm=\"Battery_Emulator_Secure\"");
      return request->send(response);
    }
    handler(request);
  });
}

// =========================================================================
// WebServer Initialization
// =========================================================================

void init_webserver() {
  load_auth_cache();
  def_route_with_auth("/startScan", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!is_scanning)
      xTaskCreate(wifiScanTask, "WiFiScan", 4096, NULL, 1, NULL);
    request->send(200, "text/plain", "Started");
  });

  def_route_with_auth("/getScan", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (is_scanning)
      request->send(200, "application/json", "{\"status\":\"scanning\"}");
    else
      request->send(200, "application/json", scan_results);
  });

  def_route_with_auth("/applyWifi", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssid", true)) {
      apply_ssid = request->getParam("ssid", true)->value();
      apply_pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
      keep_ap_mode = request->hasParam("keep_ap", true) ? (request->getParam("keep_ap", true)->value() == "1") : true;
      wifi_apply_state = 1;
      xTaskCreate(wifiApplyTask, "WiFiApply", 4096, NULL, 1, NULL);
      request->send(200, "text/plain", "Started");
    } else {
      request->send(400, "text/plain", "No SSID");
    }
  });

  def_route_with_auth("/applyWifiStatus", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String resp = "{\"state\":" + String(wifi_apply_state) + ",\"ip\":\"" + applied_ip + "\"}";
    request->send(200, "application/json", resp);
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(401, "text/html",
                  "<h2 style='font-family:sans-serif; color:white; background:black; padding:20px;'>Logged Out "
                  "Successfully</h2><p style='color:white; background:black;'>Please close your browser window, or <a "
                  "href='/' style='color:#00ff00;'>click here to login again</a>.</p>");
  });

  server.on("/api/is_auth", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!auth_cached) load_auth_cache();
    request->send(200, "text/plain", cached_auth_enabled ? "1" : "0");
  });

  // 🌟 API MQTT status
  server.on("/api/mqtt/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!mqtt_enabled) {
      AsyncWebServerResponse *res = request->beginResponse(200, "application/json", "{\"enabled\":false,\"connected\":false}");
      res->addHeader("Connection", "close");
      return request->send(res);
    }

    String out;
    out.reserve(64);
    out = "{\"enabled\":true,\"connected\":";
    out += mqtt_is_connected ? "true" : "false";
    out += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", out);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  def_route_with_auth("/", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    ota_active = false;
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(FPSTR(full_dashboard_html));
    request->send(response);
  });

  def_route_with_auth("/advanced", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(FPSTR(index_html_header));
    print_advanced_battery_html(response);
    response->print(FPSTR(index_html_footer));
    request->send(response);
  });

  def_route_with_auth("/canlog", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", can_logger_full_html, can_logger_processor);
  });

  def_route_with_auth("/canreplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!datalayer.system.info.can_logging_active) {
      datalayer.system.info.logged_can_messages_offset = 0;
      datalayer.system.info.logged_can_messages[0] = '\0';
    }
    datalayer.system.info.can_logging_active = true;
    request->send(200, "text/html", can_replay_full_html, can_replay_template_processor);
  });

  def_route_with_auth("/startReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (isReplayRunning) {
      request->send(400, "text/plain", "Replay already running!");
      return;
    }
    datalayer.system.info.loop_playback = request->hasParam("loop") && request->getParam("loop")->value().toInt() == 1;
    isReplayRunning = true;
    xTaskCreatePinnedToCore(canReplayTask, "CAN_Replay", 8192, NULL, 1, NULL, 1);
    request->send(200, "text/plain", "CAN replay started!");
  });

  def_route_with_auth("/stopReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.loop_playback = false;
    request->send(200, "text/plain", "CAN replay stopped!");
  });

  def_route_with_auth("/setCANInterface", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("interface")) {
      datalayer.system.info.can_replay_interface = request->getParam("interface")->value().toInt();
      request->send(200, "text/plain", "New interface selected");
    } else {
      request->send(400, "text/plain", "Error: updating interface failed");
    }
  });

  def_route_with_auth("/log", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(FPSTR(index_html_header));
    print_debug_logger_html(response);
    response->print(FPSTR(index_html_footer));
    request->send(response);
  });

  // API : Start Stream
  def_route_with_auth("/start_can_logging", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = true;
    if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, (TickType_t)50) == pdTRUE) {
      can_batch_len = 0;
      can_batch_buffer[0] = '\0';
      xSemaphoreGive(can_stream_mutex);
    }
    request->send(200, "text/plain", "CAN Stream Started");
  });

  // 🌟 API : Stop Stream
  server.on("/stop_can_logging", HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = false;
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
    if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, (TickType_t)50) == pdTRUE) {
      can_batch_len = 0;
      can_batch_buffer[0] = '\0';
      xSemaphoreGive(can_stream_mutex);
    }
    request->send(200, "text/plain", "Logging stopped");
  });

  // API : High-Speed Poller
  def_route_with_auth("/api/can_poll", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String send_buffer = "";

    if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, 0) == pdTRUE) {
      if (can_batch_len > 0) {
        send_buffer = String(can_batch_buffer);
        can_batch_len = 0;
        can_batch_buffer[0] = '\0';
      }

      xSemaphoreGive(can_stream_mutex);
    }

    if (send_buffer.length() > 0) {
      request->send(200, "text/plain", send_buffer);
    } else {
      request->send(200, "text/plain", "");
    }
  });

  def_route_with_auth("/clear_log", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
    request->send(200, "text/plain", "Log cleared");
  });

  server.on(
      "/import_can_log", HTTP_POST,
      [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Ready to receive file."); },
      handleFileUpload);

  if (datalayer.system.info.CAN_SD_logging_active) {
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_can_writing();
      request->send(SD_MMC, CAN_LOG_FILE, String(), true);
      resume_can_writing();
    });
    server.on("/delete_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_can_log();
      request->send(200, "text/plain", "Log file deleted");
    });
  } else {
    server.on("/export_can_log", HTTP_GET,
              [](AsyncWebServerRequest* request) { request->send(400, "text/plain", "Use frontend Export."); });
  }

  if (datalayer.system.info.SD_logging_active) {
    server.on("/delete_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_log();
      request->send(200, "text/plain", "Log file deleted");
    });
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_log_writing();
      request->send(SD_MMC, LOG_FILE, String(), true);
      resume_log_writing();
    });
  } else {
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (!datalayer.system.info.web_logging_active) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "System Logging is DISABLED.");
        response->addHeader("Content-Disposition", "attachment; filename=\"System_Log_Disabled.txt\"");
        return request->send(response);
      }
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0)
        logs = "No logs available.";
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      char filename[32];
      if (!strftime(filename, sizeof(filename), "log_%H-%M-%S.txt", &timeinfo)) {
        strcpy(filename, "battery_emulator_log.txt");
      }
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", String("attachment; filename=\"") + String(filename) + "\"");
      request->send(response);
    });
  }

  extern const char full_cellmonitor_html[];

  def_route_with_auth("/cellmonitor", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", full_cellmonitor_html);
  });

  // Use AsyncResponseStream instead
  def_route_with_auth("/events", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("text/html");

    response->print(index_html_header);
    print_events_html(response);
    response->print(index_html_footer);
    request->send(response);
  });

  def_route_with_auth("/clearevents", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    reset_all_events();
    String response = "<html><body><script>window.location.href = '/events';</script></body></html>";
    request->send(200, "text/html", response);
  });

  def_route_with_auth("/factoryReset", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;
    settings.clearAll();
    request->send(200, "text/html", "OK");
  });

  auto serve_settings_page = [](AsyncWebServerRequest* request, const char* html_array) {
    auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);
    request->send(200, "text/html", (const uint8_t*)html_array, strlen(html_array),
                  [settings](const String& content) { return settings_processor(content, *settings); });
  };

  // auto serve_settings_page = [](AsyncWebServerRequest* request, const char* html_array) {
  //   auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);
  //   AsyncWebServerResponse* response =
  //       request->beginResponse(200, "text/html", html_array,
  //                              [settings](const String& content) { return settings_processor(content, *settings); });
  //   request->send(response);
  // };

  def_route_with_auth("/settings", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_batt_html);
  });
  def_route_with_auth("/set_network", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_net_html);
  });
  def_route_with_auth("/set_hardware", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_hw_html);
  });
  def_route_with_auth("/set_web", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_web_html);
  });
  def_route_with_auth("/set_overrides", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_overrides_html);
  });
  def_route_with_auth("/set_cloud", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) {
    serve_settings_page(request, settings_cloud_html);
  });

  //saveSettings, Use this API to receive values ​​via GET instead!
  def_route_with_auth("/api/saveBulk", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;

    if (request->hasParam("inverter")) settings.saveUInt("INVTYPE", request->getParam("inverter")->value().toInt());
    if (request->hasParam("battery")) settings.saveUInt("BATTTYPE", request->getParam("battery")->value().toInt());
    if (request->hasParam("charger")) settings.saveUInt("CHGTYPE", request->getParam("charger")->value().toInt());
    if (request->hasParam("shunttype")) settings.saveUInt("SHUNTTYPE", request->getParam("shunttype")->value().toInt());

    if (request->hasParam("BATTPVMAX")) settings.saveUInt("BATTPVMAX", (int)(request->getParam("BATTPVMAX")->value().toFloat() * 10.0f));
    if (request->hasParam("BATTPVMIN")) settings.saveUInt("BATTPVMIN", (int)(request->getParam("BATTPVMIN")->value().toFloat() * 10.0f));
    if (request->hasParam("MQTTPUBLISHMS")) settings.saveUInt("MQTTPUBLISHMS", request->getParam("MQTTPUBLISHMS")->value().toInt() * 1000);

    // 2. Group of number
    static const char* uintSettingNames[] = {
        "INVCOMM", "BATTCHEM", "BATTCOMM", "CHGCOMM", "EQSTOP", "BATT2COMM", "BATT3COMM", "SHUNTTYPE", "SHUNTCOMM",
        "BATTCVMAX",  "BATTCVMIN",  "MAXPRETIME", "MAXPREFREQ", "WIFICHANNEL", "DCHGPOWER",   "CHGPOWER",
        "LOCALIP1",   "LOCALIP2",   "LOCALIP3",   "LOCALIP4",   "GATEWAY1",    "GATEWAY2",    "GATEWAY3",
        "GATEWAY4",   "SUBNET1",    "SUBNET2",    "SUBNET3",    "SUBNET4",     "MQTTPORT",    "MQTTTIMEOUT",
        "SOFAR_ID",   "PYLONSEND",  "INVCELLS",   "INVMODULES", "INVCELLSPER", "INVVLEVEL",   "INVCAPACITY",
        "INVBTYPE",   "CANFREQ",    "CANFDFREQ",  "PRECHGMS",   "PWMFREQ",     "PWMHOLD",     "GTWCOUNTRY",
        "GTWMAPREG",  "GTWCHASSIS", "GTWPACK",    "LEDMODE",    "GPIOOPT1",    "GPIOOPT2",    "GPIOOPT3",
        "INVSUNTYPE", "GPIOOPT4",   "LEDTAIL",    "LEDCOUNT",   "WEBAUTH",     "DISPLAYTYPE", "CTVNOM",
        "CTANOM",     "CTATTEN",    "PYLONBAUD",  "PYLONBRAND", "MQTTFORMAT"
    };

    for (const char* uintSetting : uintSettingNames) {
      if (request->hasParam(uintSetting)) {
        settings.saveUInt(uintSetting, request->getParam(uintSetting)->value().toInt());
      }
    }

    // 3. Group of String
    static const char* stringSettingNames[] = {
        "APNAME", "APPASSWORD", "HOSTNAME", "MQTTSERVER", "MQTTUSER",
        "MQTTPASSWORD", "MQTTTOPIC", "MQTTOBJIDPREFIX", "MQTTDEVICENAME", "HADEVICEID",
        "SSID", "PASSWORD", "WEBUSER", "WEBPASS", "CTOFFSET"
    };

    for (const char* stringSetting : stringSettingNames) {
      if (request->hasParam(stringSetting)) {
        settings.saveString(stringSetting, request->getParam(stringSetting)->value().c_str());
      }
    }

    // group of Checkbox (use Static Array instead of std::vector)
    String pageId = request->hasParam("PAGE_ID") ? request->getParam("PAGE_ID")->value() : "";

    const char** activeBools = nullptr;
    size_t boolCount = 0;

    static const char* netBools[] = {"STATICIP",  "WIFIAPENABLED", "ESPNOWENABLED", "MQTTENABLED", "MQTTCELLV", "REMBMSRESET",   "MQTTTOPICS",    "HADISC"};
    static const char* setBools[] = {"INTERLOCKREQ", "DIGITALHVIL", "GTWRHD",  "SOCESTIMATED", "DBLBTR",    "TRIBTR", "PYLONOFFSET",  "PYLONORDER",  "DEYEBYD", "INVICNT",      "PRIMOGEN24"};
    static const char* hwBools[]  = {"CANFDASCAN",  "CNTCTRLDBL",   "CNTCTRLTRI", "CNTCTRL",        "NCCONTACTOR", "PWMCNTCTRL", "PERBMSRESET", "EXTPRECHARGE", "NOINVDISC",  "EPAPREFRESHBTN", "MULTII2C",    "I2C_SHT30", "I2C_ATECC",   "I2C_RTC",      "I2C_IO",     "CTINVERT"};
    static const char* webBools[] = {"PERFPROFILE", "CANLOGUSB", "USBENABLED", "WEBENABLED", "CANLOGSD", "SDLOGENABLED"};

    if (pageId == "/set_network") { activeBools = netBools; boolCount = sizeof(netBools) / sizeof(netBools[0]); }
    else if (pageId == "/settings") { activeBools = setBools; boolCount = sizeof(setBools) / sizeof(setBools[0]); }
    else if (pageId == "/set_hardware") { activeBools = hwBools; boolCount = sizeof(hwBools) / sizeof(hwBools[0]); }
    else if (pageId == "/set_web") { activeBools = webBools; boolCount = sizeof(webBools) / sizeof(webBools[0]); }

    for (size_t i = 0; i < boolCount; i++) {
      const char* boolSetting = activeBools[i];
      bool isChecked = request->hasParam(boolSetting) && request->getParam(boolSetting)->value() == "on";
      settings.saveBool(boolSetting, isChecked);
    }

    settingsUpdated = true;
    settings.were_settings_updated();
    request->send(200, "text/plain", "OK");
  });

  auto update_string = [](const char* route, std::function<void(String)> setter,
                          std::function<bool(String)> validator = nullptr) {
    def_route_with_auth(route, server, HTTP_GET, [=](AsyncWebServerRequest* request) {
      if (request->hasParam("value")) {
        String value = request->getParam("value")->value();
        if (validator && !validator(value)) {
          request->send(400, "text/plain", "Invalid value");
          return;
        }
        setter(value);
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });
  };

  auto update_string_setting = [=](const char* route, std::function<void(String)> setter,
                                   std::function<bool(String)> validator = nullptr) {
    update_string(
        route,
        [setter](String value) {
          setter(value);
          store_settings();
        },
        validator);
  };

  auto update_int_setting = [=](const char* route, std::function<void(int)> setter) {
    update_string_setting(route, [setter](String value) { setter(value.toInt()); });
  };

  update_int_setting("/updateBatterySize", [](int value) { datalayer.battery.info.total_capacity_Wh = value; });
  update_int_setting("/updateUseScaledSOC", [](int value) { datalayer.battery.settings.soc_scaling_active = value; });
  update_int_setting("/enableRecoveryMode",
                     [](int value) { datalayer.battery.settings.user_requests_forced_charging_recovery_mode = value; });
  update_string_setting("/updateSocMax", [](String value) {
    datalayer.battery.settings.max_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });
  update_int_setting("/set_can_id_cutoff", [](int value) { user_selected_CAN_ID_cutoff_filter = value; });

  update_string("/pause", [](String value) {
    if (value == "" || value == "toggle") {
      bool current_status = (get_emulator_pause_status() == "PAUSED" || get_emulator_pause_status() == "PAUSING");
      setBatteryPause(!current_status, false);
    } else {
      setBatteryPause(value == "true" || value == "1", false);
    }
  });

  update_string("/equipmentStop", [](String value) {
    if (value == "true" || value == "1")
      setBatteryPause(true, false, true);
    else
      setBatteryPause(false, false, false);
  });

  // Route for editing SOC Calibration BYD
  update_string_setting("/editCalTargetSOC", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetSOC = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing AH Calibration BYD
  update_string_setting("/editCalTargetAH", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetAH = static_cast<uint16_t>(value.toFloat());
  });

  update_string_setting("/updateSocMin", [](String value) {
    datalayer.battery.settings.min_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });
  update_string_setting("/updateMaxChargeA", [](String value) {
    datalayer.battery.settings.max_user_set_charge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });
  update_string_setting("/updateMaxDischargeA", [](String value) {
    datalayer.battery.settings.max_user_set_discharge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });

  for (const auto& cmd : battery_commands) {
    auto route = String("/") + cmd.identifier;
    server.on(
        route.c_str(), HTTP_PUT,
        [cmd](AsyncWebServerRequest* request) {
          BatteryEmulatorSettingsStore auth_settings(true);
          bool is_auth_enabled = (auth_settings.getUInt("WEBAUTH", 1) == 1);
          if (is_auth_enabled && !checkAuth(request))
            return request->requestAuthentication();
        },
        nullptr,
        [cmd](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          String battIndex = "";
          if (len > 0)
            battIndex += (char)data[0];
          Battery* batt = battery;
          if (battIndex == "1")
            batt = battery2;
          if (battIndex == "2")
            batt = battery3;
          if (batt)
            cmd.action(batt);
          request->send(200, "text/plain", "Command performed.");
        });
  }

  update_int_setting("/updateUseVoltageLimit",
                     [](int value) { datalayer.battery.settings.user_set_voltage_limits_active = value; });
  update_string_setting("/updateMaxChargeVoltage", [](String value) {
    datalayer.battery.settings.max_user_set_charge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });
  update_string_setting("/updateMaxDischargeVoltage", [](String value) {
    datalayer.battery.settings.max_user_set_discharge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });
  update_string_setting("/updateBMSresetDuration", [](String value) {
    datalayer.battery.settings.user_set_bms_reset_duration_ms = static_cast<uint16_t>(value.toFloat() * 1000);
  });
  update_string_setting("/updateFakeBatteryVoltage", [](String value) { battery->set_fake_voltage(value.toFloat()); });
  update_int_setting("/TeslaBalAct", [](int value) { datalayer.battery.settings.user_requests_balancing = value; });
  update_string_setting("/BalTime", [](String value) {
    datalayer.battery.settings.balancing_max_time_ms = static_cast<uint32_t>(value.toFloat() * 60000);
  });
  update_string_setting("/BalFloatPower", [](String value) {
    datalayer.battery.settings.balancing_float_power_W = static_cast<uint16_t>(value.toFloat());
  });
  update_string_setting("/BalMaxPackV", [](String value) {
    datalayer.battery.settings.balancing_max_pack_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });
  update_string_setting("/BalMaxCellV", [](String value) {
    datalayer.battery.settings.balancing_max_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });
  update_string_setting("/BalMaxDevCellV", [](String value) {
    datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });

  if (charger) {
    update_string_setting(
        "/updateChargeSetpointV", [](String value) { datalayer.charger.charger_setpoint_HV_VDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV) &&
                 (val * datalayer.charger.charger_setpoint_HV_IDC <= CHARGER_MAX_POWER);
        });
    update_string_setting(
        "/updateChargeSetpointA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_A) && (val <= datalayer.battery.settings.max_user_set_charge_dA) &&
                 (val * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER);
        });
    update_string_setting("/updateChargeEndA",
                          [](String value) { datalayer.charger.charger_setpoint_HV_IDC_END = value.toFloat(); });
    update_int_setting("/updateChargerHvEnabled",
                       [](int value) { datalayer.charger.charger_HV_enabled = (bool)value; });
    update_int_setting("/updateChargerAux12vEnabled",
                       [](int value) { datalayer.charger.charger_aux12V_enabled = (bool)value; });
  }

  // API: System & Battery Data
  def_route_with_auth("/api/data", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String out;
    out.reserve(3072);

    out += "{\"sys\":{";
    out += "\"uptime\":\"" + get_uptime() + "\",";
    out += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    out += "\"heap_size\":" + String(ESP.getHeapSize()) + ",";
    out += "\"psram\":" + String(ESP.getFreePsram()) + ",";
    out += "\"psram_size\":" + String(ESP.getPsramSize()) + ",";
    out += "\"status\":\"" + String(get_emulator_pause_status().c_str()) + "\",";
    out += "\"version\":\"" + String(version_number) + "\",";

    String inv_name = inverter ? String(inverter->name()) + " " + String(datalayer.system.info.inverter_brand) : "None";
    out += "\"inv\":\"" + inv_name + "\",";

    String bat_proto = "None";
    if (battery) {
      bat_proto = String(datalayer.system.info.battery_protocol);
      if (battery3) bat_proto += " (Triple)";
      else if (battery2) bat_proto += " (Double)";
      if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) bat_proto += " (LFP)";
    }
    out += "\"bat\":\"" + bat_proto + "\",";
    out += "\"inv_cont\":" + String(datalayer.system.status.inverter_allows_contactor_closing ? "true" : "false");
    out += "},";

    auto print_battery = [&](const char* key, Battery* bat, auto& dt_info, auto& dt_status) {
      out += String("\"") + key + "\":{";
      out += "\"en\":" + String(bat ? "true" : "false");
      if (bat) {
        out += ",\"fault\":" + String((dt_status.bms_status == FAULT) ? "true" : "false");
        out += ",\"soc\":\"" + String((float)dt_status.real_soc / 100.0f, 2) + "\"";
        out += ",\"soh\":\"" + String((float)dt_status.soh_pptt / 100.0f, 2) + "\"";
        out += ",\"v\":\"" + String((float)dt_status.voltage_dV / 10.0f, 1) + "\"";
        out += ",\"a\":\"" + String((float)dt_status.current_dA / 10.0f, 1) + "\"";
        out += ",\"p\":" + String((int)dt_status.active_power_W);
        out += ",\"cmin\":" + String((int)dt_status.cell_min_voltage_mV);
        out += ",\"cmax\":" + String((int)dt_status.cell_max_voltage_mV);
        out += ",\"tmin\":\"" + String((float)dt_status.temperature_min_dC / 10.0f, 1) + "\"";
        out += ",\"tmax\":\"" + String((float)dt_status.temperature_max_dC / 10.0f, 1) + "\"";

        const char* stat_str = "UNKNOWN";
        if (dt_status.bms_status == ACTIVE) stat_str = "OK";
        else if (dt_status.bms_status == UPDATING) stat_str = "UPDATING";
        else if (dt_status.bms_status == FAULT) stat_str = "FAULT";
        out += ",\"stat\":\"" + String(stat_str) + "\"";

        const char* act_str = "Idle 💤";
        if (dt_status.current_dA < 0) act_str = "Discharging 🔋⬇️";
        else if (dt_status.current_dA > 0) act_str = "Charging 🔋⬆️";
        out += ",\"act\":\"" + String(act_str) + "\"";

        out += ",\"mc\":" + String((int)dt_status.max_charge_power_W);
        out += ",\"md\":" + String((int)dt_status.max_discharge_power_W);
        out += ",\"rem\":" + String((int)dt_status.remaining_capacity_Wh);
        out += ",\"tot\":" + String((int)dt_info.total_capacity_Wh);
        out += ",\"b_cont\":" + String((dt_status.bms_status != FAULT) ? "true" : "false");
      }
      out += "}";
    };

    print_battery("b1", battery, datalayer.battery.info, datalayer.battery.status); out += ",";
    print_battery("b2", battery2, datalayer.battery2.info, datalayer.battery2.status); out += ",";
    print_battery("b3", battery3, datalayer.battery3.info, datalayer.battery3.status); out += ",";

    out += "\"chg\":{";
    out += "\"en\":" + String(charger ? "true" : "false");
    if (charger) {
      out += ",\"v\":\"" + String(charger->HVDC_output_voltage(), 2) + "\"";
      out += ",\"a\":\"" + String(charger->HVDC_output_current(), 2) + "\"";
    }
    out += "}";

    out += "}"; // end JSON

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", out);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  // API: JSON cell battery info (Dynamic RAM & Anti-Leak)
  def_route_with_auth("/api/cells", server, HTTP_GET, [](AsyncWebServerRequest* request) {

    int total_active_cells = 0;
    if (battery)  total_active_cells += datalayer.battery.info.number_of_cells;
    if (battery2) total_active_cells += datalayer.battery2.info.number_of_cells;
    if (battery3) total_active_cells += datalayer.battery3.info.number_of_cells;

    if (total_active_cells == 0) {
       AsyncWebServerResponse *res = request->beginResponse(200, "application/json", "{}");
       res->addHeader("Connection", "close");
       return request->send(res);
    }

    size_t required_ram = 128 + (total_active_cells * 15);

    String out;
    out.reserve(required_ram);
    out += "{";
    bool firstBat = true;

    auto add_cells = [&](const char* key, Battery* bat, DATALAYER_BATTERY_TYPE& layer) {
      if (!bat || layer.info.number_of_cells == 0) return;

      if (!firstBat) out += ",";
      firstBat = false;

      out += String("\"") + key + "\":{\"cv\":[";

      int cells = layer.info.number_of_cells;
      if (cells > MAX_AMOUNT_CELLS) cells = MAX_AMOUNT_CELLS;

      for (int i = 0; i < cells; i++) {
        out += String(layer.status.cell_voltages_mV[i]);
        if (i < cells - 1) out += ",";
      }

      out += "],\"cb\":[";

      for (int i = 0; i < cells; i++) {
        out += String(layer.status.cell_balancing_status[i] ? 1 : 0);
        if (i < cells - 1) out += ",";
      }
      out += "]}";
    };

    if (battery)  add_cells("b1", battery, datalayer.battery);
    if (battery2) add_cells("b2", battery2, datalayer.battery2);
    if (battery3) add_cells("b3", battery3, datalayer.battery3);

    out += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", out);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  def_route_with_auth("/debug", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  def_route_with_auth("/reboot", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Rebooting server...");
    setBatteryPause(true, true, true, false);

    // FreeRTOS Timer Create Task exec Restart, Non-block Network Thread!
    xTaskCreate(
        [](void* param) {
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          ESP.restart();
        },
        "RebootTask", 2048, NULL, 1, NULL);
  });

  // 🌟 API: Get Firmware Info for OTA
  def_route_with_auth("/GetFirmwareInfo", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String out;
    out.reserve(128);

    out += "{\"hardware\":\"";
    out += String(esp32hal->name());
    out += "\",\"firmware\":\"";
    out += String(version_number);
    out += "\"}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", out);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  def_route_with_auth("/ota", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    // Use Stream Less RAM (0% Heap Allocation)
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(index_html_header);
    response->print(R"rawliteral(
      <style>
        .ota-wrap { display: flex; flex-direction: column; height: 100vh; gap: 15px; }
        .ota-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #3498db; padding: 0; overflow: hidden; display: flex; flex-direction: column; height: 80vh; min-height: 600px; }
        .ota-header { padding: 15px 20px; border-bottom: 1px solid #eee; background: #fbfcfc; }
        .ota-header h3 { margin: 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; }
        .ota-header p { margin: 5px 0 0 0; color: #7f8c8d; font-size: 0.9rem; }
        .ota-iframe { align-self: stretch; border: none; flex: 1; }
      </style>
      <div class="ota-wrap">
        <div class="ota-card">
          <div class="ota-header">
            <h3>🔄 System OTA Update</h3>
            <p>Upload a new firmware (.bin) file. Please do not turn off the power during the update.</p>
          </div>
          <iframe src="/update" class="ota-iframe" title="OTA Update"></iframe>
        </div>
      </div>
    )rawliteral");
    response->print(index_html_footer);
    request->send(response);
  });

  // =======================================================
  // ☁️ ATECC608A Cloud Security APIs
  // =======================================================

  // Web page for checking safe status (retrieval of serial number, checking lock status).
  server.on("/api/atecc/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;

    doc["serial"] = atecc_get_serial();
    doc["config_locked"] = atecc_is_config_locked();
    doc["data_locked"] = atecc_is_data_locked();

    serializeJson(doc, *response);
    request->send(response);
  });

  // Web page for generating Private Key and CSR.
  server.on("/api/atecc/gencsr", HTTP_POST, [](AsyncWebServerRequest* request) {
    String cn = request->hasParam("cn", true) ? request->getParam("cn", true)->value() : "BatteryEmulator";
    String platform = request->hasParam("platform", true) ? request->getParam("platform", true)->value() : "AWS";

    // Instruct the ATECC chip to calculate the secret code (takes 1-2 seconds)
    String country = request->hasParam("country", true) ? request->getParam("country", true)->value() : "TH";
    String state = request->hasParam("state", true) ? request->getParam("state", true)->value() : "Bangkok";
    String locality = request->hasParam("locality", true) ? request->getParam("locality", true)->value() : "Bangkok";
    String org = request->hasParam("org", true) ? request->getParam("org", true)->value() : "BatteryEmulator";

    // Pass all variables to the function.
    String csr_pem = atecc_generate_csr(cn, platform, country, state, locality, org);

    if (csr_pem.length() > 0) {
      request->send(200, "text/plain", csr_pem);
    } else {
      request->send(500, "text/plain", "❌ Error generating CSR. Is Config Zone locked? Or missing chip?");
    }
  });

  // Lock Zone page
  server.on("/api/atecc/lock", HTTP_POST, [](AsyncWebServerRequest* request) {
    String zone = request->hasParam("zone", true) ? request->getParam("zone", true)->value() : "";

    bool success = false;
    String message = "";

    if (zone == "config") {
      success = atecc_lock_config_zone();
      message = success ? "✅ Config Zone Locked Successfully! Now you can generate CSR." : "❌ Failed to lock Config Zone.";
    } else if (zone == "data") {
      success = atecc_lock_data_zone();
      message = success ? "✅ Data Zone Locked Successfully! Device is fully secured." : "❌ Failed to lock Data Zone.";
    } else {
      message = "Invalid zone specified.";
    }

    request->send(success ? 200 : 500, "text/plain", message);
  });

  // =======================================================
  // 📜 Cloud Certificates APIs (NVM Storage)
  // =======================================================

  // API 4: Display previous certificates on the website.
  server.on("/api/cloud/getcerts", HTTP_GET, [](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc; // It uses a bit of RAM because the certificate has a long text.

    doc["cert"] = settings.getString("AWS_CERT", "");
    doc["ca"] = settings.getString("AWS_CA", "");

    serializeJson(doc, *response);
    request->send(response);
  });

  // API 5: Receive the new certificate and save it to NVM.
  server.on("/api/cloud/savecerts", HTTP_POST, [](AsyncWebServerRequest* request) {
    String cert = request->hasParam("cert", true) ? request->getParam("cert", true)->value() : "";
    String ca = request->hasParam("ca", true) ? request->getParam("ca", true)->value() : "";

    BatteryEmulatorSettingsStore settings;
    // Save NVM (ESP32's permanent memory)
    settings.saveString("AWS_CERT", cert.c_str());
    settings.saveString("AWS_CA", ca.c_str());

    request->send(200, "text/plain", "✅ Certificates saved successfully to ESP32 NVM!");
  });

  server.on("/settings.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/css", SETTINGS_CSS);
    response->addHeader("Cache-Control", "max-age=86400");
    request->send(response);
  });

  init_ElegantOTA();
  server.begin();
}

String getConnectResultString(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return "Connected";
    case WL_NO_SHIELD:
      return "No shield";
    case WL_IDLE_STATUS:
      return "Idle status";
    case WL_NO_SSID_AVAIL:
      return "No SSID available";
    case WL_SCAN_COMPLETED:
      return "Scan completed";
    case WL_CONNECT_FAILED:
      return "Connect failed";
    case WL_CONNECTION_LOST:
      return "Connection lost";
    case WL_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown";
  }
}

void ota_monitor() {
  ElegantOTA.loop();
  if (ota_active && ota_timeout_timer.elapsed()) {
    set_event(EVENT_OTA_UPDATE_TIMEOUT, 0);
    onOTAEnd(false);
  }
}

void init_ElegantOTA() {
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}

String get_firmware_info_processor(const String& var) {
  if (var == "X") {
    String content = "";
    static JsonDocument doc;
    doc["hardware"] = esp32hal->name();
    doc["firmware"] = String(version_number);
    serializeJson(doc, content);
    return content;
  }
  return String();
}

String get_uptime() {
  uint64_t milliseconds = millis64();
  uint16_t total_days = milliseconds / (1000 * 60 * 60 * 24);
  uint32_t remaining_seconds_in_day = (milliseconds / 1000) % (60 * 60 * 24);
  uint32_t remaining_hours = remaining_seconds_in_day / (60 * 60);
  uint32_t remaining_minutes = (remaining_seconds_in_day % (60 * 60)) / 60;
  uint32_t remaining_seconds = remaining_seconds_in_day % 60;

  char buf[64];
  snprintf(buf, sizeof(buf), "%u days, %lu hours, %lu mins, %lu secs",
           total_days, remaining_hours, remaining_minutes, remaining_seconds);
  return String(buf);
}

void onOTAStart() {
  setBatteryPause(true, true);
  set_event(EVENT_OTA_UPDATE, 0);
  clear_event(EVENT_OTA_UPDATE_TIMEOUT);
  ota_active = true;
  ota_timeout_timer.reset();
}

void onOTAProgress(size_t current, size_t final) {
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    logging.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    ota_timeout_timer.reset();
  }
}

void onOTAEnd(bool success) {
  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);
  if (success) {
    setBatteryPause(true, true, true, false);
    logging.println("OTA update finished successfully!");
  } else {
    logging.println("There was an error during OTA update!");
    setBatteryPause(false, false);
  }
}

template <typename T>
String formatPowerValue(String label, T value, String unit, int precision, String color) {
  String result = "<h4 style='color: " + color + ";'>" + label + ": ";
  result += formatPowerValue(value, unit, precision);
  result += "</h4>";
  return result;
}

template <typename T>
String formatPowerValue(T value, String unit, int precision) {
  String result = "";
  if (std::is_same<T, float>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) {
    float convertedValue = static_cast<float>(value);
    if (convertedValue >= 1000.0f || convertedValue <= -1000.0f) {
      result += String(convertedValue / 1000.0f, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }
  result += unit;
  return result;
}
