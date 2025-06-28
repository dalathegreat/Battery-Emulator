#include "webserver.h"
#include <Preferences.h>
#include <ctime>
#include <vector>
#include "../../../USER_SECRETS.h"
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../sdcard/sdcard.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"
#include "esp_task_wdt.h"

void transmit_can_frame(CAN_frame* tx_frame, int interface);

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

        transmit_can_frame(&currentFrame, datalayer.system.info.can_replay_interface);
      }
    } while (datalayer.system.info.loop_playback);

    messages.clear();          // Free vector memory
    messages.shrink_to_fit();  // Release excess memory
  }

  isReplayRunning = false;  // Mark replay as stopped
  vTaskDelete(NULL);
}

void init_webserver() {

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(401); });

  // Route for firmware info from ota update page
  server.on("/GetFirmwareInfo", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/html", index_html, processor);
  });

  // Route for going to settings web page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/html", index_html, settings_processor);
  });

  // Route for going to advanced battery info web page
  server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html, advanced_battery_processor);
  });

  // Route for going to CAN logging web page
  server.on("/canlog", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", can_logger_processor());
    request->send(response);
  });

  // Route for going to CAN replay web page
  server.on("/canreplay", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", can_replay_processor());
    request->send(response);
  });

  server.on("/startReplay", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }

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
  server.on("/stopReplay", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }

    datalayer.system.info.loop_playback = false;

    request->send(200, "text/plain", "CAN replay stopped!");
  });

  // Route to handle setting the CAN interface for CAN replay
  server.on("/setCANInterface", HTTP_GET, [](AsyncWebServerRequest* request) {
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

#if defined(DEBUG_VIA_WEB) || defined(LOG_TO_SD)
  // Route for going to debug logging web page
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", debug_logger_processor());
    request->send(response);
  });
#endif  // DEBUG_VIA_WEB

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

#ifndef LOG_CAN_TO_SD
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
#endif

#ifdef LOG_CAN_TO_SD
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
#endif

#ifdef LOG_TO_SD
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
#endif

#ifndef LOG_TO_SD
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
#endif

  // Route for going to cellmonitor web page
  server.on("/cellmonitor", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/html", index_html, cellmonitor_processor);
  });

  // Route for going to event log web page
  server.on("/events", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/html", index_html, events_processor);
  });

  // Route for clearing all events
  server.on("/clearevents", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    reset_all_events();
    // Send back a response that includes an instant redirect to /events
    String response = "<html><body>";
    response += "<script>window.location.href = '/events';</script>";  // Instant redirect
    response += "</body></html>";
    request->send(200, "text/html", response);
  });

  struct BoolSetting {
    const char* name;
    bool existingValue;
    bool newValue;
  };

  const char* boolSettingNames[] = {"DBLBTR", "CNTCTRL", "CNTCTRLDBL", "PWMCNTCTRL", "PERBMSRESET", "REMBMSRESET"};

#ifdef COMMON_IMAGE
  // Handles the form POST from UI to save certain settings: battery/inverter type and double battery on/off
  server.on("/saveSettings", HTTP_POST, [boolSettingNames](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;

    std::vector<BoolSetting> boolSettings;

    for (auto& name : boolSettingNames) {
      boolSettings.push_back({name, settings.getBool(name), false});
    }

    int numParams = request->params();
    for (int i = 0; i < numParams; i++) {
      auto p = request->getParam(i);
      if (p->name() == "inverter") {
        auto type = static_cast<InverterProtocolType>(atoi(p->value().c_str()));
        settings.saveUInt("INVTYPE", (int)type);
      } else if (p->name() == "battery") {
        auto type = static_cast<BatteryType>(atoi(p->value().c_str()));
        settings.saveUInt("BATTTYPE", (int)type);
      } else if (p->name() == "charger") {
        auto type = static_cast<ChargerType>(atoi(p->value().c_str()));
        settings.saveUInt("CHGTYPE", (int)type);
      } /*else if (p->name() == "dblbtr") {
        newDoubleBattery = p->value() == "on";
      } else if (p->name() == "contctrl") {
        settings.saveBool("CNTCTRL", p->value() == "on");
      } else if (p->name() == "contctrldbl") {
        settings.saveBool("CNTCTRLDBL", p->value() == "on");
      } else if (p->name() == "pwmcontctrl") {
        settings.saveBool("PWMCNTCTRL", p->value() == "on");
      } else if (p->name() == "PERBMSRESET") {
        settings.saveBool("PERBMSRESET", p->value() == "on");
      } else if (p->name() == "REMBMSRESET") {
        settings.saveBool("REMBMSRESET", p->value() == "on");
      }*/

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

    settingsUpdated = true;
    request->redirect("/settings");
  });
#endif

  // Route for editing SSID
  server.on("/updateSSID", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      if (value.length() <= 63) {  // Check if SSID is within the allowable length
        ssid = value.c_str();
        store_settings();
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "SSID must be 63 characters or less");
      }
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });
  // Route for editing Password
  server.on("/updatePassword", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      if (value.length() > 8) {  // Check if password is within the allowable length
        password = value.c_str();
        store_settings();
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Password must be atleast 8 characters");
      }
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing Wh
  server.on("/updateBatterySize", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.info.total_capacity_Wh = value.toInt();
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing USE_SCALED_SOC
  server.on("/updateUseScaledSOC", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.soc_scaling_active = value.toInt();
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing SOCMax
  server.on("/updateSocMax", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_percentage = static_cast<uint16_t>(value.toFloat() * 100);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for pause/resume Battery emulator
  server.on("/pause", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("p")) {
      String valueStr = request->getParam("p")->value();
      setBatteryPause(valueStr == "true" || valueStr == "1", false);
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for equipment stop/resume
  server.on("/equipmentStop", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("stop")) {
      String valueStr = request->getParam("stop")->value();
      if (valueStr == "true" || valueStr == "1") {
        setBatteryPause(true, false, true);  //Pause battery, do not pause CAN, equipment stop on (store to flash)
      } else {
        setBatteryPause(false, false, false);
      }
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing SOCMin
  server.on("/updateSocMin", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.min_percentage = static_cast<uint16_t>(value.toFloat() * 100);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxChargeA
  server.on("/updateMaxChargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_user_set_charge_dA = static_cast<uint16_t>(value.toFloat() * 10);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxDischargeA
  server.on("/updateMaxDischargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_user_set_discharge_dA = static_cast<uint16_t>(value.toFloat() * 10);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  for (const auto& cmd : battery_commands) {
    auto route = String("/") + cmd.identifier;
    server.on(
        route.c_str(), HTTP_PUT,
        [cmd](AsyncWebServerRequest* request) {
          if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password)) {
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
  server.on("/updateUseVoltageLimit", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.user_set_voltage_limits_active = value.toInt();
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxChargeVoltage
  server.on("/updateMaxChargeVoltage", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_user_set_charge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxDischargeVoltage
  server.on("/updateMaxDischargeVoltage", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_user_set_discharge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing FakeBatteryVoltage
  server.on("/updateFakeBatteryVoltage", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "Bad Request");
    }

    String value = request->getParam("value")->value();
    float val = value.toFloat();

    battery->set_fake_voltage(val);

    request->send(200, "text/plain", "Updated successfully");
  });

  // Route for editing balancing enabled
  server.on("/TeslaBalAct", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.user_requests_balancing = value.toInt();
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing balancing max time
  server.on("/BalTime", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.balancing_time_ms = static_cast<uint32_t>(value.toFloat() * 60000);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing balancing max power
  server.on("/BalFloatPower", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.balancing_float_power_W = static_cast<uint16_t>(value.toFloat());
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing balancing max pack voltage
  server.on("/BalMaxPackV", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.balancing_max_pack_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing balancing max cell voltage
  server.on("/BalMaxCellV", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.balancing_max_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing balancing max cell voltage deviation
  server.on("/BalMaxDevCellV", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
      store_settings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  if (charger) {
    // Route for editing ChargerTargetV
    server.on("/updateChargeSetpointV", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      if (!request->hasParam("value")) {
        request->send(400, "text/plain", "Bad Request");
      }

      String value = request->getParam("value")->value();
      float val = value.toFloat();

      if (!(val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV)) {
        request->send(400, "text/plain", "Bad Request");
      }

      if (!(val * datalayer.charger.charger_setpoint_HV_IDC <= CHARGER_MAX_POWER)) {
        request->send(400, "text/plain", "Bad Request");
      }

      datalayer.charger.charger_setpoint_HV_VDC = val;

      request->send(200, "text/plain", "Updated successfully");
    });

    // Route for editing ChargerTargetA
    server.on("/updateChargeSetpointA", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      if (!request->hasParam("value")) {
        request->send(400, "text/plain", "Bad Request");
      }

      String value = request->getParam("value")->value();
      float val = value.toFloat();

      if (!(val <= datalayer.battery.settings.max_user_set_charge_dA && val <= CHARGER_MAX_A)) {
        request->send(400, "text/plain", "Bad Request");
      }

      if (!(val * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER)) {
        request->send(400, "text/plain", "Bad Request");
      }

      datalayer.charger.charger_setpoint_HV_IDC = value.toFloat();

      request->send(200, "text/plain", "Updated successfully");
    });

    // Route for editing ChargerEndA
    server.on("/updateChargeEndA", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      if (request->hasParam("value")) {
        String value = request->getParam("value")->value();
        datalayer.charger.charger_setpoint_HV_IDC_END = value.toFloat();
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });

    // Route for enabling/disabling HV charger
    server.on("/updateChargerHvEnabled", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      if (request->hasParam("value")) {
        String value = request->getParam("value")->value();
        datalayer.charger.charger_HV_enabled = (bool)value.toInt();
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });

    // Route for enabling/disabling aux12v charger
    server.on("/updateChargerAux12vEnabled", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
        return request->requestAuthentication();
      if (request->hasParam("value")) {
        String value = request->getParam("value")->value();
        datalayer.charger.charger_aux12V_enabled = (bool)value.toInt();
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });
  }

  // Send a GET request to <ESP_IP>/update
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/plain", "Debug: all OK.");
  });

  // Route to handle reboot command
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
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
#ifdef HW_LILYGO
    doc["hardware"] = "LilyGo T-CAN485";
#endif  // HW_LILYGO
#ifdef HW_STARK
    doc["hardware"] = "Stark CMR Module";
#endif  // HW_STARK
#ifdef HW_3LB
    doc["hardware"] = "3LB board";
#endif  // HW_3LB
#ifdef HW_DEVKIT
    doc["hardware"] = "ESP32 DevKit V1";
#endif  // HW_DEVKIT

    doc["firmware"] = String(version_number);
    serializeJson(doc, content);
    return content;
  }
  return String();
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
    content += "</style>";

    // Compact header
    content += "<h2>" + String(ssidAP) + "</h2>";

    // Start content block
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
    content += "<h4>Software: " + String(version_number);

#ifdef COMMON_IMAGE
    content += " (Common image) ";
#endif
// Show hardware used:
#ifdef HW_LILYGO
    content += " Hardware: LilyGo T-CAN485";
#endif  // HW_LILYGO
#ifdef HW_STARK
    content += " Hardware: Stark CMR Module";
#endif  // HW_STARK
    content += " @ " + String(datalayer.system.info.CPU_temperature, 1) + " &deg;C</h4>";
    content += "<h4>Uptime: " + uptime_formatter::getUptime() + "</h4>";
#ifdef FUNCTION_TIME_MEASUREMENT
    // Load information
    content += "<h4>Core task max load: " + String(datalayer.system.status.core_task_max_us) + " us</h4>";
    content += "<h4>Core task max load last 10 s: " + String(datalayer.system.status.core_task_10s_max_us) + " us</h4>";
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
#endif  // FUNCTION_TIME_MEASUREMENT

    wl_status_t status = WiFi.status();
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + String(ssid.c_str());
    if (status == WL_CONNECTED) {
      // Get and display the signal strength (RSSI) and channel
      content += " RSSI:" + String(WiFi.RSSI()) + " dBm Ch: " + String(WiFi.channel());
    }
    content += "</h4>";
    if (status == WL_CONNECTED) {
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
        content += datalayer.system.info.inverter_protocol;
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

      switch (led_get_color()) {
        case led_color::GREEN:
          content += "#2D3F2F;";
          break;
        case led_color::YELLOW:
          content += "#F5CC00;";
          break;
        case led_color::BLUE:
          content += "#2B35AF;";  // Blue in test mode
          break;
        case led_color::RED:
          content += "#A70107;";
          break;
        default:  // Some new color, make background green
          content += "#2D3F2F;";
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
        if (datalayer.battery.settings.user_settings_limit_discharge) {
          content += " (Manual)</h4>";
        } else {
          content += " (BMS)</h4>";
        }
        content += "<h4 style='color: white;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A";
        if (datalayer.battery.settings.user_settings_limit_charge) {
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

      content += "<h4>Battery allows contactor closing: ";
      if (datalayer.system.status.battery_allows_contactor_closing == true) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005;</span>";
      }

      content += " Inverter allows contactor closing: ";
      if (datalayer.system.status.inverter_allows_contactor_closing == true) {
        content += "<span>&#10003;</span></h4>";
      } else {
        content += "<span style='color: red;'>&#10005;</span></h4>";
      }
      if (emulator_pause_status == NORMAL)
        content += "<h4>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
      else
        content += "<h4 style='color: red;'>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";

      if (contactor_control_enabled) {
        content += "<h4>Contactors controlled by emulator, state: ";
        if (datalayer.system.status.contactors_battery2_engaged) {
          content += "<span style='color: green;'>ON</span>";
        } else {
          content += "<span style='color: red;'>OFF</span>";
        }
        content += "</h4>";
        if (contactor_control_enabled_double_battery) {
          if (pwm_contactor_control) {
            content += "<h4>Cont. Neg.: ";
            if (datalayer.system.status.contactors_battery2_engaged) {
              content += "<span style='color: green;'>Economized</span>";
              content += " Cont. Pos.: ";
              content += "<span style='color: green;'>Economized</span>";
            } else {
              content += "<span style='color: red;'>&#10005;</span>";
              content += " Cont. Pos.: ";
              content += "<span style='color: red;'>&#10005;</span>";
            }
          } else {  // No PWM_CONTACTOR_CONTROL , we can read the pin and see feedback. Helpful if channel overloaded
            content += "<h4>Cont. Neg.: ";
            if (digitalRead(SECOND_BATTERY_CONTACTORS_PIN) == HIGH) {
              content += "<span style='color: green;'>&#10003;</span>";
            } else {
              content += "<span style='color: red;'>&#10005;</span>";
            }
          }  //no PWM_CONTACTOR_CONTROL
          content += "</h4>";
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

        content += "<h4>Automatic contactor closing allowed:</h4>";
        content += "<h4>Battery: ";
        if (datalayer.system.status.battery2_allowed_contactor_closing == true) {
          content += "<span>&#10003;</span>";
        } else {
          content += "<span style='color: red;'>&#10005;</span>";
        }

        content += " Inverter: ";
        if (datalayer.system.status.inverter_allows_contactor_closing == true) {
          content += "<span>&#10003;</span></h4>";
        } else {
          content += "<span style='color: red;'>&#10005;</span></h4>";
        }

        if (emulator_pause_status == NORMAL)
          content += "<h4>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
        else
          content += "<h4 style='color: red;'>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";

        if (contactor_control_enabled) {
          content += "<h4>Contactors controlled by emulator, state: ";
          if (datalayer.system.status.contactors_battery2_engaged) {
            content += "<span style='color: green;'>ON</span>";
          } else {
            content += "<span style='color: red;'>OFF</span>";
          }
          content += "</h4>";

          if (contactor_control_enabled_double_battery) {
            content += "<h4>Cont. Neg.: ";
            if (pwm_contactor_control) {
              if (datalayer.system.status.contactors_battery2_engaged) {
                content += "<span style='color: green;'>Economized</span>";
                content += " Cont. Pos.: ";
                content += "<span style='color: green;'>Economized</span>";
              } else {
                content += "<span style='color: red;'>&#10005;</span>";
                content += " Cont. Pos.: ";
                content += "<span style='color: red;'>&#10005;</span>";
              }
            } else {  // No PWM_CONTACTOR_CONTROL , we can read the pin and see feedback. Helpful if channel overloaded
#if defined(SECOND_POSITIVE_CONTACTOR_PIN) && defined(SECOND_NEGATIVE_CONTACTOR_PIN)
              if (digitalRead(SECOND_NEGATIVE_CONTACTOR_PIN) == HIGH) {
                content += "<span style='color: green;'>&#10003;</span>";
              } else {
                content += "<span style='color: red;'>&#10005;</span>";
              }

              content += " Cont. Pos.: ";
              if (digitalRead(SECOND_POSITIVE_CONTACTOR_PIN) == HIGH) {
                content += "<span style='color: green;'>&#10003;</span>";
              } else {
                content += "<span style='color: red;'>&#10005;</span>";
              }
#endif
            }
            content += "</h4>";
          }
        }
        content += "</div>";
        content += "</div>";
      }
    }

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
#if defined(DEBUG_VIA_WEB) || defined(LOG_TO_SD)
    content += "<button onclick='Log()'>Log</button> ";
#endif  // DEBUG_VIA_WEB
    content += "<button onclick='Cellmon()'>Cellmonitor</button> ";
    content += "<button onclick='Events()'>Events</button> ";
    content += "<button onclick='askReboot()'>Reboot Emulator</button>";
    if (WEBSERVER_AUTH_REQUIRED)
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
    content +=
        "function askReboot() { if (window.confirm('Are you sure you want to reboot the emulator? NOTE: If "
        "emulator is handling contactors, they will open during reboot!')) { "
        "reboot(); } }";
    content += "function reboot() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/reboot', true);";
    content += "  xhr.send();";
    content += "}";
    if (WEBSERVER_AUTH_REQUIRED) {
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
        "window.location.reload();};xhr.open('GET','/pause?p='+pause,true);xhr.send();";
    content += "}";
    content += "function estop(stop){";
    content +=
        "var xhr=new "
        "XMLHttpRequest();xhr.onload=function() { "
        "window.location.reload();};xhr.open('GET','/equipmentStop?stop='+stop,true);xhr.send();";
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
#ifdef DEBUG_LOG
    logging.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
#endif  // DEBUG_LOG
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
#ifdef DEBUG_LOG
    logging.println("OTA update finished successfully!");
#endif  // DEBUG_LOG
  } else {
#ifdef DEBUG_LOG
    logging.println("There was an error during OTA update!");
#endif  // DEBUG_LOG
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
