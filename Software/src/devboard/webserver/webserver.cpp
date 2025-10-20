#include "webserver.h"
#include <Preferences.h>
#include <ctime>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../inverter/INVERTERS.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../sdcard/sdcard.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"
#include "esp_task_wdt.h"
#include "html_escape.h"

#include <string>
extern std::string http_username;
extern std::string http_password;

bool webserver_auth = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Measure OTA progress
unsigned long ota_progress_millis = 0;

#include "advanced_battery_html.h"
#include "can_logging_html.h"
#include "can_replay_html.h"
#include "cellmonitor_html.h"
#include "debug_logging_html.h"
#include "events_html.h"
#include "index_html.h"
#include "settings_html.h"

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

const char get_firmware_info_html[] = R"rawliteral(%X%)rawliteral";

String importedLogs = "";      // Store the uploaded logfile contents in RAM
bool isReplayRunning = false;  // Global flag to track replay state

// True when user has updated settings that need a reboot to be effective.
bool settingsUpdated = false;

CAN_frame currentFrame = {.FD = true, .ext_ID = false, .DLC = 64, .ID = 0x12F, .data = {0}};

void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len,
                      bool final) {
  if (!index) {
    importedLogs = "";  // Clear previous logs
    logging.printf("Receiving file: %s\n", filename.c_str());
  }

  // Append received data to the string (RAM storage)
  importedLogs += String((char*)data).substring(0, len);

  if (final) {
    logging.println("Upload Complete!");
    request->send(200, "text/plain", "File uploaded successfully");
  }
}

void canReplayTask(void* param) {
  std::vector<String> messages;
  messages.reserve(1000);  // Pre-allocate memory to reduce fragmentation

  if (!importedLogs.isEmpty()) {
    int lastIndex = 0;

    while (true) {
      int nextIndex = importedLogs.indexOf("\n", lastIndex);
      if (nextIndex == -1) {
        messages.push_back(importedLogs.substring(lastIndex));
        break;
      }
      messages.push_back(importedLogs.substring(lastIndex, nextIndex));
      lastIndex = nextIndex + 1;
    }

    do {
      float firstTimestamp = -1.0;
      float lastTimestamp = 0.0;
      bool firstMessageSent = false;  // Track first message

      for (size_t i = 0; i < messages.size(); i++) {
        String line = messages[i];
        line.trim();
        if (line.length() == 0)
          continue;

        int timeStart = line.indexOf("(") + 1;
        int timeEnd = line.indexOf(")");
        if (timeStart == 0 || timeEnd == -1)
          continue;

        float currentTimestamp = line.substring(timeStart, timeEnd).toFloat();

        if (firstTimestamp < 0) {
          firstTimestamp = currentTimestamp;
        }

        // Send first message immediately
        if (!firstMessageSent) {
          firstMessageSent = true;
          firstTimestamp = currentTimestamp;  // Adjust reference time
        } else {
          // Delay only if this isn't the first message
          float deltaT = (currentTimestamp - lastTimestamp) * 1000;
          vTaskDelay((int)deltaT / portTICK_PERIOD_MS);
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
    } while (datalayer.system.info.loop_playback);

    messages.clear();          // Free vector memory
    messages.shrink_to_fit();  // Release excess memory
  }

  isReplayRunning = false;  // Mark replay as stopped
  vTaskDelete(NULL);
}

void def_route_with_auth(const char* uri, AsyncWebServer& serv, WebRequestMethodComposite method,
                         std::function<void(AsyncWebServerRequest*)> handler) {
  serv.on(uri, method, [handler](AsyncWebServerRequest* request) {
    if (webserver_auth && !request->authenticate(http_username.c_str(), http_password.c_str())) {
      return request->requestAuthentication();
    }
    handler(request);
  });
}

void init_webserver() {

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(401); });

  // Route for firmware info from ota update page
  def_route_with_auth("/GetFirmwareInfo", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  // Route for root / web page
  def_route_with_auth("/", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { request->send(200, "text/html", index_html, processor); });

  // Route for going to settings web page
  def_route_with_auth("/settings", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    // Using make_shared to ensure lifetime for the settings object during send() lambda execution
    auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);

    request->send(200, "text/html", settings_html,
                  [settings](const String& content) { return settings_processor(content, *settings); });
  });

  // Route for going to advanced battery info web page
  def_route_with_auth("/advanced", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html, advanced_battery_processor);
  });

  // Route for going to CAN logging web page
  def_route_with_auth("/canlog", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", can_logger_processor()));
  });

  // Route for going to CAN replay web page
  def_route_with_auth("/canreplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", can_replay_processor()));
  });

  def_route_with_auth("/startReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    // Prevent multiple replay tasks from being created
    if (isReplayRunning) {
      request->send(400, "text/plain", "Replay already running!");
      return;
    }

    datalayer.system.info.loop_playback = request->hasParam("loop") && request->getParam("loop")->value().toInt() == 1;
    isReplayRunning = true;  // Set flag before starting task

    xTaskCreatePinnedToCore(canReplayTask, "CAN_Replay", 8192, NULL, 1, NULL, 1);

    request->send(200, "text/plain", "CAN replay started!");
  });

  // Route for stopping the CAN replay
  def_route_with_auth("/stopReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.loop_playback = false;

    request->send(200, "text/plain", "CAN replay stopped!");
  });

  // Route to handle setting the CAN interface for CAN replay
  def_route_with_auth("/setCANInterface", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("interface")) {
      String canInterface = request->getParam("interface")->value();

      // Convert the received value to an integer
      int interfaceValue = canInterface.toInt();

      // Update the datalayer with the selected interface
      datalayer.system.info.can_replay_interface = interfaceValue;

      // Respond with success message
      request->send(200, "text/plain", "New interface selected");
    } else {
      request->send(400, "text/plain", "Error: updating interface failed");
    }
  });

  if (datalayer.system.info.web_logging_active || datalayer.system.info.SD_logging_active) {
    // Route for going to debug logging web page
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/html", debug_logger_processor());
      request->send(response);
    });
  }

  // Define the handler to stop can logging
  server.on("/stop_can_logging", HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = false;
    request->send(200, "text/plain", "Logging stopped");
  });

  // Define the handler to import can log
  server.on(
      "/import_can_log", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Ready to receive file.");  // Response when request is made
      },
      handleFileUpload);

  if (datalayer.system.info.CAN_SD_logging_active) {
    // Define the handler to export can log
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_can_writing();
      request->send(SD_MMC, CAN_LOG_FILE, String(), true);
      resume_can_writing();
    });

    // Define the handler to delete can log
    server.on("/delete_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_can_log();
      request->send(200, "text/plain", "Log file deleted");
    });
  } else {
    // Define the handler to export can log
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0) {
        logs = "No logs available.";
      }

      // Get the current time
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);

      // Ensure time retrieval was successful
      char filename[32];
      if (strftime(filename, sizeof(filename), "canlog_%H-%M-%S.txt", &timeinfo)) {
        // Valid filename created
      } else {
        // Fallback filename if automatic timestamping failed
        strcpy(filename, "battery_emulator_can_log.txt");
      }

      // Use request->send with dynamic headers
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", String("attachment; filename=\"") + String(filename) + "\"");
      request->send(response);
    });
  }

  if (datalayer.system.info.SD_logging_active) {
    // Define the handler to delete log file
    server.on("/delete_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_log();
      request->send(200, "text/plain", "Log file deleted");
    });

    // Define the handler to export debug log
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_log_writing();
      request->send(SD_MMC, LOG_FILE, String(), true);
      resume_log_writing();
    });
  } else {
    // Define the handler to export debug log
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0) {
        logs = "No logs available.";
      }

      // Get the current time
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);

      // Ensure time retrieval was successful
      char filename[32];
      if (strftime(filename, sizeof(filename), "log_%H-%M-%S.txt", &timeinfo)) {
        // Valid filename created
      } else {
        // Fallback filename if automatic timestamping failed
        strcpy(filename, "battery_emulator_log.txt");
      }

      // Use request->send with dynamic headers
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", String("attachment; filename=\"") + String(filename) + "\"");
      request->send(response);
    });
  }

  // Route for going to cellmonitor web page
  def_route_with_auth("/cellmonitor", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html, cellmonitor_processor);
  });

  // Route for going to event log web page
  def_route_with_auth("/events", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html, events_processor);
  });

  // Route for clearing all events
  def_route_with_auth("/clearevents", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    reset_all_events();
    // Send back a response that includes an instant redirect to /events
    String response = "<html><body>";
    response += "<script>window.location.href = '/events';</script>";  // Instant redirect
    response += "</body></html>";
    request->send(200, "text/html", response);
  });

  def_route_with_auth("/factoryReset", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    // Reset all settings to factory defaults
    BatteryEmulatorSettingsStore settings;
    settings.clearAll();

    request->send(200, "text/html", "OK");
  });

  struct BoolSetting {
    const char* name;
    bool existingValue;
    bool newValue;
  };

  const char* boolSettingNames[] = {
      "DBLBTR",       "CNTCTRL",      "CNTCTRLDBL",    "PWMCNTCTRL",  "PERBMSRESET", "SDLOGENABLED",
      "STATICIP",     "REMBMSRESET",  "EXTPRECHARGE",  "USBENABLED",  "CANLOGUSB",   "WEBENABLED",
      "CANFDASCAN",   "CANLOGSD",     "WIFIAPENABLED", "MQTTENABLED", "NOINVDISC",   "HADISC",
      "MQTTTOPICS",   "MQTTCELLV",    "INVICNT",       "GTWRHD",      "DIGITALHVIL", "PERFPROFILE",
      "INTERLOCKREQ", "SOCESTIMATED", "PYLONOFFSET",   "PYLONORDER",  "DEYEBYD",
  };

  // Handles the form POST from UI to save settings of the common image
  server.on("/saveSettings", HTTP_POST, [boolSettingNames](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;

    std::vector<BoolSetting> boolSettings;

    for (auto& name : boolSettingNames) {
      boolSettings.push_back({name, settings.getBool(name, name == std::string("WIFIAPENABLED")), false});
    }

    int numParams = request->params();
    for (int i = 0; i < numParams; i++) {
      auto p = request->getParam(i);
      if (p->name() == "inverter") {
        auto type = static_cast<InverterProtocolType>(atoi(p->value().c_str()));
        settings.saveUInt("INVTYPE", (int)type);
      } else if (p->name() == "INVCOMM") {
        auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
        settings.saveUInt("INVCOMM", (int)type);
      } else if (p->name() == "battery") {
        auto type = static_cast<BatteryType>(atoi(p->value().c_str()));
        settings.saveUInt("BATTTYPE", (int)type);
      } else if (p->name() == "BATTCHEM") {
        auto type = static_cast<battery_chemistry_enum>(atoi(p->value().c_str()));
        settings.saveUInt("BATTCHEM", (int)type);
      } else if (p->name() == "BATTCOMM") {
        auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
        settings.saveUInt("BATTCOMM", (int)type);
      } else if (p->name() == "BATTPVMAX") {
        auto type = p->value().toFloat() * 10.0;
        settings.saveUInt("BATTPVMAX", (int)type);
      } else if (p->name() == "BATTPVMIN") {
        auto type = p->value().toFloat() * 10.0;
        settings.saveUInt("BATTPVMIN", (int)type);
      } else if (p->name() == "BATTCVMAX") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("BATTCVMAX", type);
      } else if (p->name() == "BATTCVMIN") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("BATTCVMIN", type);
      } else if (p->name() == "charger") {
        auto type = static_cast<ChargerType>(atoi(p->value().c_str()));
        settings.saveUInt("CHGTYPE", (int)type);
      } else if (p->name() == "CHGCOMM") {
        auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
        settings.saveUInt("CHGCOMM", (int)type);
      } else if (p->name() == "EQSTOP") {
        auto type = static_cast<STOP_BUTTON_BEHAVIOR>(atoi(p->value().c_str()));
        settings.saveUInt("EQSTOP", (int)type);
      } else if (p->name() == "BATT2COMM") {
        auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
        settings.saveUInt("BATT2COMM", (int)type);
      } else if (p->name() == "shunt") {
        auto type = static_cast<ShuntType>(atoi(p->value().c_str()));
        settings.saveUInt("SHUNTTYPE", (int)type);
      } else if (p->name() == "SHUNTCOMM") {
        auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
        settings.saveUInt("SHUNTCOMM", (int)type);
      } else if (p->name() == "MAXPRETIME") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("MAXPRETIME", type);
      } else if (p->name() == "WIFICHANNEL") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("WIFICHANNEL", type);
      } else if (p->name() == "DCHGPOWER") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("DCHGPOWER", type);
      } else if (p->name() == "CHGPOWER") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("CHGPOWER", type);
      } else if (p->name() == "LOCALIP1") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("LOCALIP1", type);
      } else if (p->name() == "LOCALIP2") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("LOCALIP2", type);
      } else if (p->name() == "LOCALIP3") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("LOCALIP3", type);
      } else if (p->name() == "LOCALIP4") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("LOCALIP4", type);
      } else if (p->name() == "GATEWAY1") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GATEWAY1", type);
      } else if (p->name() == "GATEWAY2") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GATEWAY2", type);
      } else if (p->name() == "GATEWAY3") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GATEWAY3", type);
      } else if (p->name() == "GATEWAY4") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GATEWAY4", type);
      } else if (p->name() == "SUBNET1") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("SUBNET1", type);
      } else if (p->name() == "SUBNET2") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("SUBNET2", type);
      } else if (p->name() == "SUBNET3") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("SUBNET3", type);
      } else if (p->name() == "SUBNET4") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("SUBNET4", type);
      } else if (p->name() == "SSID") {
        settings.saveString("SSID", p->value().c_str());
        ssid = settings.getString("SSID", "").c_str();
      } else if (p->name() == "PASSWORD") {
        settings.saveString("PASSWORD", p->value().c_str());
        password = settings.getString("PASSWORD", "").c_str();
      } else if (p->name() == "APNAME") {
        settings.saveString("APNAME", p->value().c_str());
      } else if (p->name() == "APPASSWORD") {
        settings.saveString("APPASSWORD", p->value().c_str());
      } else if (p->name() == "HOSTNAME") {
        settings.saveString("HOSTNAME", p->value().c_str());
      } else if (p->name() == "MQTTSERVER") {
        settings.saveString("MQTTSERVER", p->value().c_str());
      } else if (p->name() == "MQTTPORT") {
        auto port = atoi(p->value().c_str());
        settings.saveUInt("MQTTPORT", port);
      } else if (p->name() == "MQTTUSER") {
        settings.saveString("MQTTUSER", p->value().c_str());
      } else if (p->name() == "MQTTPASSWORD") {
        settings.saveString("MQTTPASSWORD", p->value().c_str());
      } else if (p->name() == "MQTTTOPIC") {
        settings.saveString("MQTTTOPIC", p->value().c_str());
      } else if (p->name() == "MQTTTIMEOUT") {
        auto port = atoi(p->value().c_str());
        settings.saveUInt("MQTTTIMEOUT", port);
      } else if (p->name() == "MQTTOBJIDPREFIX") {
        settings.saveString("MQTTOBJIDPREFIX", p->value().c_str());
      } else if (p->name() == "MQTTDEVICENAME") {
        settings.saveString("MQTTDEVICENAME", p->value().c_str());
      } else if (p->name() == "HADEVICEID") {
        settings.saveString("HADEVICEID", p->value().c_str());
      } else if (p->name() == "SOFAR_ID") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("SOFAR_ID", type);
      } else if (p->name() == "PYLONSEND") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("PYLONSEND", type);
      } else if (p->name() == "INVCELLS") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVCELLS", type);
      } else if (p->name() == "INVMODULES") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVMODULES", type);
      } else if (p->name() == "INVCELLSPER") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVCELLSPER", type);
      } else if (p->name() == "INVVLEVEL") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVVLEVEL", type);
      } else if (p->name() == "INVCAPACITY") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVCAPACITY", type);
      } else if (p->name() == "INVBTYPE") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("INVBTYPE", (int)type);
      } else if (p->name() == "CANFREQ") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("CANFREQ", type);
      } else if (p->name() == "CANFDFREQ") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("CANFDFREQ", type);
      } else if (p->name() == "PRECHGMS") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("PRECHGMS", type);
      } else if (p->name() == "PWMFREQ") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("PWMFREQ", type);
      } else if (p->name() == "PWMHOLD") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("PWMHOLD", type);
      } else if (p->name() == "GTWCOUNTRY") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GTWCOUNTRY", type);
      } else if (p->name() == "GTWMAPREG") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GTWMAPREG", type);
      } else if (p->name() == "GTWCHASSIS") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GTWCHASSIS", type);
      } else if (p->name() == "GTWPACK") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("GTWPACK", type);
      } else if (p->name() == "LEDMODE") {
        auto type = atoi(p->value().c_str());
        settings.saveUInt("LEDMODE", type);
      }

      for (auto& boolSetting : boolSettings) {
        if (p->name() == boolSetting.name) {
          boolSetting.newValue = p->value() == "on";
        }
      }
    }

    for (auto& boolSetting : boolSettings) {
      if (boolSetting.existingValue != boolSetting.newValue) {
        settings.saveBool(boolSetting.name, boolSetting.newValue);
      }
    }

    settingsUpdated = settings.were_settings_updated();
    request->redirect("/settings");
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

  // Route for editing Wh
  update_int_setting("/updateBatterySize", [](int value) { datalayer.battery.info.total_capacity_Wh = value; });

  // Route for editing USE_SCALED_SOC
  update_int_setting("/updateUseScaledSOC", [](int value) { datalayer.battery.settings.soc_scaling_active = value; });

  // Route for editing SOCMax
  update_string_setting("/updateSocMax", [](String value) {
    datalayer.battery.settings.max_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });

  // Route for editing CAN ID cutoff filter
  update_int_setting("/set_can_id_cutoff", [](int value) { user_selected_CAN_ID_cutoff_filter = value; });

  // Route for pause/resume Battery emulator
  update_string("/pause", [](String value) { setBatteryPause(value == "true" || value == "1", false); });

  // Route for equipment stop/resume
  update_string("/equipmentStop", [](String value) {
    if (value == "true" || value == "1") {
      setBatteryPause(true, false, true);  //Pause battery, do not pause CAN, equipment stop on (store to flash)
    } else {
      setBatteryPause(false, false, false);
    }
  });

  // Route for editing SOCMin
  update_string_setting("/updateSocMin", [](String value) {
    datalayer.battery.settings.min_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });

  // Route for editing MaxChargeA
  update_string_setting("/updateMaxChargeA", [](String value) {
    datalayer.battery.settings.max_user_set_charge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing MaxDischargeA
  update_string_setting("/updateMaxDischargeA", [](String value) {
    datalayer.battery.settings.max_user_set_discharge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });

  for (const auto& cmd : battery_commands) {
    auto route = String("/") + cmd.identifier;
    server.on(
        route.c_str(), HTTP_PUT,
        [cmd](AsyncWebServerRequest* request) {
          if (webserver_auth && !request->authenticate(http_username.c_str(), http_password.c_str())) {
            return request->requestAuthentication();
          }
        },
        nullptr,
        [cmd](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          String battIndex = "";
          if (len > 0) {
            battIndex += (char)data[0];
          }
          Battery* batt = battery;
          if (battIndex == "1") {
            batt = battery2;
          }
          if (batt) {
            cmd.action(batt);
          }
          request->send(200, "text/plain", "Command performed.");
        });
  }

  // Route for editing BATTERY_USE_VOLTAGE_LIMITS
  update_int_setting("/updateUseVoltageLimit",
                     [](int value) { datalayer.battery.settings.user_set_voltage_limits_active = value; });

  // Route for editing MaxChargeVoltage
  update_string_setting("/updateMaxChargeVoltage", [](String value) {
    datalayer.battery.settings.max_user_set_charge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing MaxDischargeVoltage
  update_string_setting("/updateMaxDischargeVoltage", [](String value) {
    datalayer.battery.settings.max_user_set_discharge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing BMSresetDuration
  update_string_setting("/updateBMSresetDuration", [](String value) {
    datalayer.battery.settings.user_set_bms_reset_duration_ms = static_cast<uint16_t>(value.toFloat() * 1000);
  });

  // Route for editing FakeBatteryVoltage
  update_string_setting("/updateFakeBatteryVoltage", [](String value) { battery->set_fake_voltage(value.toFloat()); });

  // Route for editing balancing enabled
  update_int_setting("/TeslaBalAct", [](int value) { datalayer.battery.settings.user_requests_balancing = value; });

  // Route for editing balancing max time
  update_string_setting("/BalTime", [](String value) {
    datalayer.battery.settings.balancing_time_ms = static_cast<uint32_t>(value.toFloat() * 60000);
  });

  // Route for editing balancing max power
  update_string_setting("/BalFloatPower", [](String value) {
    datalayer.battery.settings.balancing_float_power_W = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing balancing max pack voltage
  update_string_setting("/BalMaxPackV", [](String value) {
    datalayer.battery.settings.balancing_max_pack_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing balancing max cell voltage
  update_string_setting("/BalMaxCellV", [](String value) {
    datalayer.battery.settings.balancing_max_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing balancing max cell voltage deviation
  update_string_setting("/BalMaxDevCellV", [](String value) {
    datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });

  if (charger) {
    // Route for editing ChargerTargetV
    update_string_setting(
        "/updateChargeSetpointV", [](String value) { datalayer.charger.charger_setpoint_HV_VDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV) &&
                 (val * datalayer.charger.charger_setpoint_HV_IDC <= CHARGER_MAX_POWER);
        });

    // Route for editing ChargerTargetA
    update_string_setting(
        "/updateChargeSetpointA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_A) && (val <= datalayer.battery.settings.max_user_set_charge_dA) &&
                 (val * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER);
        });

    // Route for editing ChargerEndA
    update_string_setting("/updateChargeEndA",
                          [](String value) { datalayer.charger.charger_setpoint_HV_IDC_END = value.toFloat(); });

    // Route for enabling/disabling HV charger
    update_int_setting("/updateChargerHvEnabled",
                       [](int value) { datalayer.charger.charger_HV_enabled = (bool)value; });

    // Route for enabling/disabling aux12v charger
    update_int_setting("/updateChargerAux12vEnabled",
                       [](int value) { datalayer.charger.charger_aux12V_enabled = (bool)value; });
  }

  // Send a GET request to <ESP_IP>/update
  def_route_with_auth("/debug", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  // Route to handle reboot command
  def_route_with_auth("/reboot", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Rebooting server...");

    //Equipment STOP without persisting the equipment state before restart
    // Max Charge/Discharge = 0; CAN = stop; contactors = open
    setBatteryPause(true, true, true, false);
    delay(1000);
    ESP.restart();
  });

  // Initialize ElegantOTA
  init_ElegantOTA();

  // Start server
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
  if (ota_active && ota_timeout_timer.elapsed()) {
    // OTA timeout, try to restore can and clear the update event
    set_event(EVENT_OTA_UPDATE_TIMEOUT, 0);
    onOTAEnd(false);
  }
}

// Function to initialize ElegantOTA
void init_ElegantOTA() {
  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
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
  uint64_t milliseconds;
  uint32_t remaining_seconds_in_day;
  uint32_t remaining_seconds;
  uint32_t remaining_minutes;
  uint32_t remaining_hours;
  uint16_t total_days;

  milliseconds = millis64();

  //convert passed millis to days, hours, minutes, seconds
  total_days = milliseconds / (1000 * 60 * 60 * 24);
  remaining_seconds_in_day = (milliseconds / 1000) % (60 * 60 * 24);
  remaining_hours = remaining_seconds_in_day / (60 * 60);
  remaining_minutes = (remaining_seconds_in_day % (60 * 60)) / 60;
  remaining_seconds = remaining_seconds_in_day % 60;

  return (String)total_days + " days, " + (String)remaining_hours + " hours, " + (String)remaining_minutes +
         " minutes, " + (String)remaining_seconds + " seconds";
}

String processor(const String& var) {
  if (var == "X") {
    String content = "";
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content +=
        "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
        "cursor: pointer; border-radius: 10px; }";
    content += "button:hover { background-color: #3A4A52; }";
    content += "h2 { font-size: 1.2em; margin: 0.3em 0 0.5em 0; }";
    content += "h4 { margin: 0.6em 0; line-height: 1.2; }";
    //content += ".tooltip { position: relative; display: inline-block; }";
    content += ".tooltip .tooltiptext {";
    content += "  visibility: hidden;";
    content += "  width: 200px;";
    content += "  background-color: #3A4A52;";  // Matching your button hover color
    content += "  color: white;";
    content += "  text-align: center;";
    content += "  border-radius: 6px;";
    content += "  padding: 8px;";
    content += "  position: absolute;";
    content += "  z-index: 1;";
    content += "  bottom: 125%;";
    content += "  left: 50%;";
    content += "  margin-left: -100px;";
    content += "  opacity: 0;";
    content += "  transition: opacity 0.3s;";
    content += "  font-size: 0.9em;";
    content += "  font-weight: normal;";
    content += "  line-height: 1.4;";
    content += "}";
    content += ".tooltip:hover .tooltiptext { visibility: visible; opacity: 1; }";
    content += ".tooltip-icon { color: #505E67; cursor: help; }";  // Matching your button color
    content += "</style>";

    // Compact header
    content += "<h2>Battery Emulator</h2>";

    // Start content block
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
    content += "<h4>Software: " + String(version_number);

// Show hardware used:
#ifdef HW_LILYGO
    content += " Hardware: LilyGo T-CAN485";
#endif  // HW_LILYGO
#ifdef HW_LILYGO2CAN
    content += " Hardware: LilyGo T_2CAN";
#endif  // HW_LILYGO2CAN
#ifdef HW_STARK
    content += " Hardware: Stark CMR Module";
#endif  // HW_STARK
    content += " @ " + String(datalayer.system.info.CPU_temperature, 1) + " &deg;C</h4>";
    content += "<h4>Uptime: " + get_uptime() + "</h4>";
    if (datalayer.system.info.performance_measurement_active) {
      // Load information
      content += "<h4>Core task max load: " + String(datalayer.system.status.core_task_max_us) + " us</h4>";
      content +=
          "<h4>Core task max load last 10 s: " + String(datalayer.system.status.core_task_10s_max_us) + " us</h4>";
      content +=
          "<h4>MQTT function (MQTT task) max load last 10 s: " + String(datalayer.system.status.mqtt_task_10s_max_us) +
          " us</h4>";
      content +=
          "<h4>WIFI function (MQTT task) max load last 10 s: " + String(datalayer.system.status.wifi_task_10s_max_us) +
          " us</h4>";
      content += "<h4>Max load @ worst case execution of core task:</h4>";
      content += "<h4>10ms function timing: " + String(datalayer.system.status.time_snap_10ms_us) + " us</h4>";
      content += "<h4>Values function timing: " + String(datalayer.system.status.time_snap_values_us) + " us</h4>";
      content += "<h4>CAN/serial RX function timing: " + String(datalayer.system.status.time_snap_comm_us) + " us</h4>";
      content += "<h4>CAN TX function timing: " + String(datalayer.system.status.time_snap_cantx_us) + " us</h4>";
      content += "<h4>OTA function timing: " + String(datalayer.system.status.time_snap_ota_us) + " us</h4>";
    }

    wl_status_t status = WiFi.status();
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + html_escape(ssid.c_str());
    if (status == WL_CONNECTED) {
      // Get and display the signal strength (RSSI) and channel
      content += " RSSI:" + String(WiFi.RSSI()) + " dBm Ch: " + String(WiFi.channel());
    }
    content += "</h4>";
    if (status == WL_CONNECTED) {
      content += "<h4>Hostname: " + html_escape(WiFi.getHostname()) + "</h4>";
      content += "<h4>IP: " + WiFi.localIP().toString() + "</h4>";
    } else {
      content += "<h4>Wifi state: " + getConnectResultString(status) + "</h4>";
    }
    // Close the block
    content += "</div>";

    if (inverter || battery || shunt || charger) {
      // Start a new block with a specific background color
      content += "<div style='background-color: #333; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";

      // Display which components are used
      if (inverter) {
        content += "<h4 style='color: white;'>Inverter protocol: ";
        content += inverter->name();
        content += " ";
        content += datalayer.system.info.inverter_brand;
        content += "</h4>";
      }

      if (battery) {
        content += "<h4 style='color: white;'>Battery protocol: ";
        content += datalayer.system.info.battery_protocol;
        if (battery2) {
          content += " (Double battery)";
        }
        if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
          content += " (LFP)";
        }
        content += "</h4>";
      }

      if (shunt) {
        content += "<h4 style='color: white;'>Shunt protocol: ";
        content += datalayer.system.info.shunt_protocol;
        content += "</h4>";
      }

      if (charger) {
        content += "<h4 style='color: white;'>Charger protocol: ";
        content += charger->name();
        content += "</h4>";
      }

      // Close the block
      content += "</div>";
    }

    if (battery) {
      if (battery2) {
        // Start a new block with a specific background color. Color changes depending on BMS status
        content += "<div style='display: flex; width: 100%;'>";
        content += "<div style='flex: 1; background-color: ";
      } else {
        // Start a new block with a specific background color. Color changes depending on system status
        content += "<div style='background-color: ";
      }

      switch (get_emulator_status()) {
        case EMULATOR_STATUS::STATUS_OK:
          content += "#2D3F2F;";
          break;
        case EMULATOR_STATUS::STATUS_WARNING:
          content += "#F5CC00;";
          break;
        case EMULATOR_STATUS::STATUS_ERROR:
          content += "#A70107;";
          break;
        case EMULATOR_STATUS::STATUS_UPDATING:
          content += "#2B35AF;";  // Blue in test mode
          break;
      }

      // Add the common style properties
      content += "padding: 10px; margin-bottom: 10px; border-radius: 50px;'>";

      // Display battery statistics within this block
      float socRealFloat =
          static_cast<float>(datalayer.battery.status.real_soc) / 100.0;  // Convert to float and divide by 100
      float socScaledFloat =
          static_cast<float>(datalayer.battery.status.reported_soc) / 100.0;  // Convert to float and divide by 100
      float sohFloat =
          static_cast<float>(datalayer.battery.status.soh_pptt) / 100.0;  // Convert to float and divide by 100
      float voltageFloat =
          static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0;  // Convert to float and divide by 10
      float currentFloat =
          static_cast<float>(datalayer.battery.status.current_dA) / 10.0;  // Convert to float and divide by 10
      float powerFloat = static_cast<float>(datalayer.battery.status.active_power_W);               // Convert to float
      float tempMaxFloat = static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0;  // Convert to float
      float tempMinFloat = static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0;  // Convert to float
      float maxCurrentChargeFloat =
          static_cast<float>(datalayer.battery.status.max_charge_current_dA) / 10.0;  // Convert to float
      float maxCurrentDischargeFloat =
          static_cast<float>(datalayer.battery.status.max_discharge_current_dA) / 10.0;  // Convert to float
      uint16_t cell_delta_mv =
          datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV;

      if (datalayer.battery.settings.soc_scaling_active)
        content += "<h4 style='color: white;'>Scaled SOC: " + String(socScaledFloat, 2) +
                   "&percnt; (real: " + String(socRealFloat, 2) + "&percnt;)</h4>";
      else
        content += "<h4 style='color: white;'>SOC: " + String(socRealFloat, 2) + "&percnt;</h4>";

      content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "&percnt;</h4>";
      content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) +
                 " V &nbsp; Current: " + String(currentFloat, 1) + " A</h4>";
      content += formatPowerValue("Power", powerFloat, "", 1);

      if (datalayer.battery.settings.soc_scaling_active)
        content += "<h4 style='color: white;'>Scaled total capacity: " +
                   formatPowerValue(datalayer.battery.info.reported_total_capacity_Wh, "h", 1) +
                   " (real: " + formatPowerValue(datalayer.battery.info.total_capacity_Wh, "h", 1) + ")</h4>";
      else
        content += formatPowerValue("Total capacity", datalayer.battery.info.total_capacity_Wh, "h", 1);

      if (datalayer.battery.settings.soc_scaling_active)
        content += "<h4 style='color: white;'>Scaled remaining capacity: " +
                   formatPowerValue(datalayer.battery.status.reported_remaining_capacity_Wh, "h", 1) +
                   " (real: " + formatPowerValue(datalayer.battery.status.remaining_capacity_Wh, "h", 1) + ")</h4>";
      else
        content += formatPowerValue("Remaining capacity", datalayer.battery.status.remaining_capacity_Wh, "h", 1);

      if (datalayer.system.settings.equipment_stop_active) {
        content +=
            formatPowerValue("Max discharge power", datalayer.battery.status.max_discharge_power_W, "", 1, "red");
        content += formatPowerValue("Max charge power", datalayer.battery.status.max_charge_power_W, "", 1, "red");
        content += "<h4 style='color: red;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A</h4>";
        content += "<h4 style='color: red;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A</h4>";
      } else {
        content += formatPowerValue("Max discharge power", datalayer.battery.status.max_discharge_power_W, "", 1);
        content += formatPowerValue("Max charge power", datalayer.battery.status.max_charge_power_W, "", 1);
        content += "<h4 style='color: white;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A";
        if (datalayer.battery.settings.remote_settings_limit_discharge) {
          content += " (Remote)</h4>";
        } else if (datalayer.battery.settings.user_settings_limit_discharge) {
          content += " (Manual)</h4>";
        } else {
          content += " (BMS)</h4>";
        }
        content += "<h4 style='color: white;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A";
        if (datalayer.battery.settings.remote_settings_limit_charge) {
          content += " (Remote)</h4>";
        } else if (datalayer.battery.settings.user_settings_limit_charge) {
          content += " (Manual)</h4>";
        } else {
          content += " (BMS)</h4>";
        }
      }

      content += "<h4>Cell min/max: " + String(datalayer.battery.status.cell_min_voltage_mV) + " mV / " +
                 String(datalayer.battery.status.cell_max_voltage_mV) + " mV</h4>";
      if (cell_delta_mv > datalayer.battery.info.max_cell_voltage_deviation_mV) {
        content += "<h4 style='color: red;'>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
      } else {
        content += "<h4>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
      }
      content += "<h4>Temperature min/max: " + String(tempMinFloat, 1) + " &deg;C / " + String(tempMaxFloat, 1) +
                 " &deg;C</h4>";

      content += "<h4>System status: ";
      switch (datalayer.battery.status.bms_status) {
        case ACTIVE:
          content += String("OK");
          break;
        case UPDATING:
          content += String("UPDATING");
          break;
        case FAULT:
          content += String("FAULT");
          break;
        case INACTIVE:
          content += String("INACTIVE");
          break;
        case STANDBY:
          content += String("STANDBY");
          break;
        default:
          content += String("??");
          break;
      }
      content += "</h4>";

      if (battery && battery->supports_real_BMS_status()) {
        content += "<h4>Battery BMS status: ";
        switch (datalayer.battery.status.real_bms_status) {
          case BMS_ACTIVE:
            content += String("OK");
            break;
          case BMS_FAULT:
            content += String("FAULT");
            break;
          case BMS_DISCONNECTED:
            content += String("DISCONNECTED");
            break;
          case BMS_STANDBY:
            content += String("STANDBY");
            break;
          default:
            content += String("??");
            break;
        }
        content += "</h4>";
      }

      if (datalayer.battery.status.current_dA == 0) {
        content += "<h4>Battery idle</h4>";
      } else if (datalayer.battery.status.current_dA < 0) {
        content += "<h4>Battery discharging!";
        if (datalayer.battery.settings.inverter_limits_discharge) {
          content += " (Inverter limiting)</h4>";
        } else {
          if (datalayer.battery.settings.user_settings_limit_discharge) {
            content += " (Settings limiting)</h4>";
          } else {
            content += " (Battery limiting)</h4>";
          }
        }
        content += "</h4>";
      } else {  // > 0 , positive current
        content += "<h4>Battery charging!";
        if (datalayer.battery.settings.inverter_limits_charge) {
          content += " (Inverter limiting)</h4>";
        } else {
          if (datalayer.battery.settings.user_settings_limit_charge) {
            content += " (Settings limiting)</h4>";
          } else {
            content += " (Battery limiting)</h4>";
          }
        }
      }

      // Close the block
      content += "</div>";

      if (battery2) {
        content += "<div style='flex: 1; background-color: ";
        switch (datalayer.battery.status.bms_status) {
          case ACTIVE:
            content += "#2D3F2F;";
            break;
          case FAULT:
            content += "#A70107;";
            break;
          default:
            content += "#2D3F2F;";
            break;
        }
        // Add the common style properties
        content += "padding: 10px; margin-bottom: 10px; border-radius: 50px;'>";

        // Display battery statistics within this block
        socRealFloat =
            static_cast<float>(datalayer.battery2.status.real_soc) / 100.0;  // Convert to float and divide by 100
        //socScaledFloat; // Same value used for bat2
        sohFloat =
            static_cast<float>(datalayer.battery2.status.soh_pptt) / 100.0;  // Convert to float and divide by 100
        voltageFloat =
            static_cast<float>(datalayer.battery2.status.voltage_dV) / 10.0;  // Convert to float and divide by 10
        currentFloat =
            static_cast<float>(datalayer.battery2.status.current_dA) / 10.0;        // Convert to float and divide by 10
        powerFloat = static_cast<float>(datalayer.battery2.status.active_power_W);  // Convert to float
        tempMaxFloat = static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0;  // Convert to float
        tempMinFloat = static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0;  // Convert to float
        cell_delta_mv = datalayer.battery2.status.cell_max_voltage_mV - datalayer.battery2.status.cell_min_voltage_mV;

        if (datalayer.battery.settings.soc_scaling_active)
          content += "<h4 style='color: white;'>Scaled SOC: " + String(socScaledFloat, 2) +
                     "&percnt; (real: " + String(socRealFloat, 2) + "&percnt;)</h4>";
        else
          content += "<h4 style='color: white;'>SOC: " + String(socRealFloat, 2) + "&percnt;</h4>";

        content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "&percnt;</h4>";
        content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) +
                   " V &nbsp; Current: " + String(currentFloat, 1) + " A</h4>";
        content += formatPowerValue("Power", powerFloat, "", 1);

        if (datalayer.battery.settings.soc_scaling_active)
          content += "<h4 style='color: white;'>Scaled total capacity: " +
                     formatPowerValue(datalayer.battery2.info.reported_total_capacity_Wh, "h", 1) +
                     " (real: " + formatPowerValue(datalayer.battery2.info.total_capacity_Wh, "h", 1) + ")</h4>";
        else
          content += formatPowerValue("Total capacity", datalayer.battery2.info.total_capacity_Wh, "h", 1);

        if (datalayer.battery.settings.soc_scaling_active)
          content += "<h4 style='color: white;'>Scaled remaining capacity: " +
                     formatPowerValue(datalayer.battery2.status.reported_remaining_capacity_Wh, "h", 1) +
                     " (real: " + formatPowerValue(datalayer.battery2.status.remaining_capacity_Wh, "h", 1) + ")</h4>";
        else
          content += formatPowerValue("Remaining capacity", datalayer.battery2.status.remaining_capacity_Wh, "h", 1);

        if (datalayer.system.settings.equipment_stop_active) {
          content +=
              formatPowerValue("Max discharge power", datalayer.battery2.status.max_discharge_power_W, "", 1, "red");
          content += formatPowerValue("Max charge power", datalayer.battery2.status.max_charge_power_W, "", 1, "red");
          content +=
              "<h4 style='color: red;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A</h4>";
          content += "<h4 style='color: red;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A</h4>";
        } else {
          content += formatPowerValue("Max discharge power", datalayer.battery2.status.max_discharge_power_W, "", 1);
          content += formatPowerValue("Max charge power", datalayer.battery2.status.max_charge_power_W, "", 1);
          content +=
              "<h4 style='color: white;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A</h4>";
          content += "<h4 style='color: white;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A</h4>";
        }

        content += "<h4>Cell min/max: " + String(datalayer.battery2.status.cell_min_voltage_mV) + " mV / " +
                   String(datalayer.battery2.status.cell_max_voltage_mV) + " mV</h4>";
        if (cell_delta_mv > datalayer.battery2.info.max_cell_voltage_deviation_mV) {
          content += "<h4 style='color: red;'>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
        } else {
          content += "<h4>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
        }
        content += "<h4>Temperature min/max: " + String(tempMinFloat, 1) + " &deg;C / " + String(tempMaxFloat, 1) +
                   " &deg;C</h4>";
        if (datalayer.battery.status.bms_status == ACTIVE) {
          content += "<h4>System status: OK </h4>";
        } else if (datalayer.battery.status.bms_status == UPDATING) {
          content += "<h4>System status: UPDATING </h4>";
        } else {
          content += "<h4>System status: FAULT </h4>";
        }
        if (datalayer.battery2.status.current_dA == 0) {
          content += "<h4>Battery idle</h4>";
        } else if (datalayer.battery2.status.current_dA < 0) {
          content += "<h4>Battery discharging!</h4>";
        } else {  // > 0
          content += "<h4>Battery charging!</h4>";
        }
        content += "</div>";
        content += "</div>";
      }
    }
    // Block for Contactor status and component request status
    // Start a new block with gray background color
    content += "<div style='background-color: #333; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    if (emulator_pause_status == NORMAL) {
      content += "<h4>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
    } else {
      content += "<h4 style='color: red;'>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
    }

    content += "<h4>Emulator allows contactor closing: ";
    if (datalayer.battery.status.bms_status == FAULT) {
      content += "<span style='color: red;'>&#10005;</span>";
    } else {
      content += "<span>&#10003;</span>";
    }
    content += " Inverter allows contactor closing: ";
    if (datalayer.system.status.inverter_allows_contactor_closing == true) {
      content += "<span>&#10003;</span></h4>";
    } else {
      content += "<span style='color: red;'>&#10005;</span></h4>";
    }
    if (battery2) {
      content += "<h4>Secondary battery allowed to join ";
      if (datalayer.system.status.battery2_allowed_contactor_closing == true) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005; (voltage mismatch)</span>";
      }
    }

    if (!contactor_control_enabled) {
      content += "<div class=\"tooltip\">";
      content += "<h4>Contactors not fully controlled via emulator <span style=\"color:orange\">[?]</span></h4>";
      content +=
          "<span class=\"tooltiptext\">This means you are either running CAN controlled contactors OR manually "
          "powering the contactors. Battery-Emulator will have limited amount of control over the contactors!</span>";
      content += "</div>";
    } else {  //contactor_control_enabled TRUE
      content += "<div class=\"tooltip\"><h4>Contactors controlled by emulator, state: ";
      if (datalayer.system.status.contactors_engaged == 0) {
        content += "<span style='color: green;'>PRECHARGE</span>";
      } else if (datalayer.system.status.contactors_engaged == 1) {
        content += "<span style='color: green;'>ON</span>";
      } else if (datalayer.system.status.contactors_engaged == 2) {
        content += "<span style='color: red;'>OFF</span>";
        content += "<span class=\"tooltip-icon\"> [!]</span>";
        content +=
            "<span class=\"tooltiptext\">Emulator spent too much time in critical FAULT event. Investigate event "
            "causing this via Events page. Reboot required to resume operation!</span>";
      }
      content += "</h4></div>";
      if (contactor_control_enabled_double_battery && battery2) {
        content += "<h4>Secondary battery contactor, state: ";
        if (pwm_contactor_control) {
          if (datalayer.system.status.contactors_battery2_engaged) {
            content += "<span style='color: green;'>Economized</span>";
          } else {
            content += "<span style='color: red;'>OFF</span>";
          }
        } else if (
            esp32hal->SECOND_BATTERY_CONTACTORS_PIN() !=
            GPIO_NUM_NC) {  // No PWM_CONTACTOR_CONTROL , we can read the pin and see feedback. Helpful if channel overloaded
          if (digitalRead(esp32hal->SECOND_BATTERY_CONTACTORS_PIN()) == HIGH) {
            content += "<span style='color: green;'>ON</span>";
          } else {
            content += "<span style='color: red;'>OFF</span>";
          }
        }  //no PWM_CONTACTOR_CONTROL
        content += "</h4>";
      }
    }

    // Close the block
    content += "</div>";

    if (charger) {
      // Start a new block with orange background color
      content += "<div style='background-color: #FF6E00; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

      content += "<h4>Charger HV Enabled: ";
      if (datalayer.charger.charger_HV_enabled) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005;</span>";
      }
      content += "</h4>";

      content += "<h4>Charger Aux12v Enabled: ";
      if (datalayer.charger.charger_aux12V_enabled) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005;</span>";
      }
      content += "</h4>";

      auto chgPwrDC = charger->outputPowerDC();
      auto chgEff = charger->efficiency();

      content += formatPowerValue("Charger Output Power", chgPwrDC, "", 1);
      if (charger->efficiencySupported()) {
        content += "<h4 style='color: white;'>Charger Efficiency: " + String(chgEff) + "%</h4>";
      }

      float HVvol = charger->HVDC_output_voltage();
      float HVcur = charger->HVDC_output_current();
      float LVvol = charger->LVDC_output_voltage();
      float LVcur = charger->LVDC_output_current();

      content += "<h4 style='color: white;'>Charger HVDC Output V: " + String(HVvol, 2) + " V</h4>";
      content += "<h4 style='color: white;'>Charger HVDC Output I: " + String(HVcur, 2) + " A</h4>";
      content += "<h4 style='color: white;'>Charger LVDC Output I: " + String(LVcur, 2) + "</h4>";
      content += "<h4 style='color: white;'>Charger LVDC Output V: " + String(LVvol, 2) + "</h4>";

      float ACcur = charger->AC_input_current();
      float ACvol = charger->AC_input_voltage();

      content += "<h4 style='color: white;'>Charger AC Input V: " + String(ACvol, 2) + " VAC</h4>";
      content += "<h4 style='color: white;'>Charger AC Input I: " + String(ACcur, 2) + " A</h4>";

      content += "</div>";
    }

    if (emulator_pause_request_ON)
      content += "<button onclick='PauseBattery(false)'>Resume charge/discharge</button> ";
    else
      content +=
          "<button onclick=\"if(confirm('Are you sure you want to pause charging and discharging? This will set the "
          "maximum charge and discharge values to zero, preventing any further power flow.')) { PauseBattery(true); "
          "}\">Pause charge/discharge</button> ";

    content += "<button onclick='OTA()'>Perform OTA update</button> ";
    content += "<button onclick='Settings()'>Change Settings</button> ";
    content += "<button onclick='Advanced()'>More Battery Info</button> ";
    content += "<button onclick='CANlog()'>CAN logger</button> ";
    content += "<button onclick='CANreplay()'>CAN replay</button> ";
    if (datalayer.system.info.web_logging_active || datalayer.system.info.SD_logging_active) {
      content += "<button onclick='Log()'>Log</button> ";
    }
    content += "<button onclick='Cellmon()'>Cellmonitor</button> ";
    content += "<button onclick='Events()'>Events</button> ";
    content += "<button onclick='askReboot()'>Reboot Emulator</button>";
    if (webserver_auth)
      content += "<button onclick='logout()'>Logout</button>";
    if (!datalayer.system.settings.equipment_stop_active)
      content +=
          "<br/><button style=\"background:red;color:white;cursor:pointer;\""
          " onclick=\""
          "if(confirm('This action will attempt to open contactors on the battery. Are you "
          "sure?')) { estop(true); }\""
          ">Open Contactors</button><br/>";
    else
      content +=
          "<br/><button style=\"background:green;color:white;cursor:pointer;\""
          "20px;font-size:16px;font-weight:bold;cursor:pointer;border-radius:5px; margin:10px;"
          " onclick=\""
          "if(confirm('This action will attempt to close contactors and enable power transfer. Are you sure?')) { "
          "estop(false); }\""
          ">Close Contactors</button><br/>";
    content += "<script>";
    content += "function OTA() { window.location.href = '/update'; }";
    content += "function Cellmon() { window.location.href = '/cellmonitor'; }";
    content += "function Settings() { window.location.href = '/settings'; }";
    content += "function Advanced() { window.location.href = '/advanced'; }";
    content += "function CANlog() { window.location.href = '/canlog'; }";
    content += "function CANreplay() { window.location.href = '/canreplay'; }";
    content += "function Log() { window.location.href = '/log'; }";
    content += "function Events() { window.location.href = '/events'; }";
    if (webserver_auth) {
      content += "function logout() {";
      content += "  var xhr = new XMLHttpRequest();";
      content += "  xhr.open('GET', '/logout', true);";
      content += "  xhr.send();";
      content += "  setTimeout(function(){ window.open(\"/\",\"_self\"); }, 1000);";
      content += "}";
    }
    content += "function PauseBattery(pause){";
    content +=
        "var xhr=new "
        "XMLHttpRequest();xhr.onload=function() { "
        "window.location.reload();};xhr.open('GET','/pause?value='+pause,true);xhr.send();";
    content += "}";
    content += "function estop(stop){";
    content +=
        "var xhr=new "
        "XMLHttpRequest();xhr.onload=function() { "
        "window.location.reload();};xhr.open('GET','/equipmentStop?value='+stop,true);xhr.send();";
    content += "}";
    content += "</script>";

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 15000);";
    content += "</script>";

    return content;
  }
  return String();
}

void onOTAStart() {
  //try to Pause the battery
  setBatteryPause(true, true);

  // Log when OTA has started
  set_event(EVENT_OTA_UPDATE, 0);

  // If already set, make a new attempt
  clear_event(EVENT_OTA_UPDATE_TIMEOUT);
  ota_active = true;

  ota_timeout_timer.reset();
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    logging.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    // Reset the "watchdog"
    ota_timeout_timer.reset();
  }
}

void onOTAEnd(bool success) {

  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);

  // Log when OTA has finished
  if (success) {
    //Equipment STOP without persisting the equipment state before restart
    // Max Charge/Discharge = 0; CAN = stop; contactors = open
    setBatteryPause(true, true, true, false);
    // a reboot will be done by the OTA library. no need to do anything here
    logging.println("OTA update finished successfully!");
  } else {
    logging.println("There was an error during OTA update!");
    //try to Resume the battery pause and CAN communication
    setBatteryPause(false, false);
  }
}

template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(String label, T value, String unit, int precision, String color) {
  String result = "<h4 style='color: " + color + ";'>" + label + ": ";
  result += formatPowerValue(value, unit, precision);
  result += "</h4>";
  return result;
}
template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(T value, String unit, int precision) {
  String result = "";

  if (std::is_same<T, float>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) {
    float convertedValue = static_cast<float>(value);

    if (convertedValue >= 1000.0 || convertedValue <= -1000.0) {
      result += String(convertedValue / 1000.0, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }

  result += unit;
  return result;
}
