#include "webserver.h"
#include <Preferences.h>
#include <esp_random.h>
#include <algorithm>
#include <atomic>
#include <new>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../battery/BYD-ATTO-3-BALANCE-HTML.h"
#include "../../battery/Battery.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/battery_aggregate.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/safety/safety.h"
#include "../../inverter/INVERTERS.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../../shunt/Shunt.h"
#include "../network/hostname.h"
#include "../network/network_status.h"
#include "../sdcard/sdcard.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/millis64.h"
#include "../utils/time_format.h"
#include "../utils/timer.h"
#include "../utils/version.h"
#include "../wifi/wifi.h"
#include "esp_task_wdt.h"
#include "favicon.h"
#include "html_escape.h"
#include "webserver_can_streaming.h"

#include <string>

std::string http_username;
std::string http_password;

bool webserver_auth = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncAuthenticationMiddleware web_auth_middleware;

// Measure OTA progress
static MyTimer ota_progress_timer = MyTimer(1000);

#include "advanced_battery_html.h"
#include "can_replay_html.h"
#include "cellmonitor_html.h"
#include "checked_html.h"
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

static void send_main_page(AsyncWebServerRequest* request, bool live_only);

bool webserver_auth_is_ready() {
  return webserver_auth && !http_username.empty() && !http_password.empty();
}

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
      float firstTimestamp = -1.0f;
      float lastTimestamp = 0.0f;
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
    if (webserver_auth_is_ready() && !request->authenticate(http_username.c_str(), http_password.c_str())) {
      return request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
    }
    handler(request);
  });
}

void init_webserver() {
  if (webserver_auth_is_ready()) {
    web_auth_middleware.setUsername(http_username.c_str());
    web_auth_middleware.setPassword(http_password.c_str());
    web_auth_middleware.setRealm(WEB_AUTH_REALM);
    web_auth_middleware.setAuthType(AsyncAuthType::AUTH_BASIC);
    server.addMiddleware(&web_auth_middleware);
  }

  server
      .on("/logout", HTTP_GET,
          [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(
                401, "text/plain", "Logout requested. Cancel the browser login prompt to finish logging out.");
            response->addHeader("WWW-Authenticate", String("Basic realm=\"") + WEB_AUTH_REALM + "\"");
            response->addHeader("Cache-Control", "no-store");
            response->addHeader("Connection", "close");
            request->send(response);
          })
      .skipServerMiddlewares();

#ifndef SMALL_FLASH_DEVICE
  // Browsers fetch the icon for the login page too, so the route must work
  // without credentials. Cached hard: the icon only changes with a firmware
  // update, so one fetch per browser instead of one per page load.
  server
      .on("/favicon.svg", HTTP_GET,
          [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200, "image/svg+xml", FAVICON_SVG);
            response->addHeader("Cache-Control", "public, max-age=604800");
            request->send(response);
          })
      .skipServerMiddlewares();
#endif  // SMALL_FLASH_DEVICE

  // Route for firmware info from ota update page
  def_route_with_auth("/GetFirmwareInfo", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  // Route for root / web page
  def_route_with_auth("/", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    // Clear OTA active flag as a safeguard in case onOTAEnd() wasn't called
    ota_active = false;
    send_main_page(request, false);
  });

  // The changing part of the main page on its own. The page polls this instead of reloading itself.
  def_route_with_auth("/live", server, HTTP_GET, [](AsyncWebServerRequest* request) { send_main_page(request, true); });

  // Route for going to settings web page
  def_route_with_auth("/settings", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    // Using make_shared to ensure lifetime for the settings object during send() lambda execution
    auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);

    request->send(200, "text/html", settings_html,
                  [settings](const String& content) { return settings_processor(content, *settings); });
  });

  // Route for going to advanced battery info web page
  def_route_with_auth("/advanced", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { send_advanced_battery_page(request); });

  // Served pre-compressed from flash rather than the template processor, so it never competes
  // for heap and costs a third of the space the plain HTML would.
  def_route_with_auth("/bydbalance", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response =
        request->beginResponse(200, "text/html", BYD_BALANCE_PAGE_GZ, sizeof(BYD_BALANCE_PAGE_GZ));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  def_route_with_auth("/bydCellBalanceTimes", server, HTTP_PUT, [](AsyncWebServerRequest* request) {
    const uint8_t index = request->hasParam("battery") ? request->getParam("battery")->value().toInt() : 0;
    if (!byd_cell_balance_times_available(index)) {
      request->send(404, "text/plain", "BYD battery not available");
    } else if (!request_byd_cell_balance_times(index)) {
      request->send(409, "text/plain", "A scan is active or the cell count is not available yet");
    } else {
      request->send(202, "text/plain", "Queued");
    }
  });

  def_route_with_auth("/bydCellBalanceTimes", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    const uint8_t index = request->hasParam("battery") ? request->getParam("battery")->value().toInt() : 0;
    if (!byd_cell_balance_times_available(index)) {
      request->send(404, "text/plain", "BYD battery not available");
      return;
    }

    AsyncWebServerResponse* response =
        request->beginResponse(200, "application/json", byd_cell_balance_times_json(index));
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  });

  // Route for going to CAN tools web page
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

  if (datalayer.system.info.web_logging_active
#ifdef SDCARD
      || datalayer.system.info.SD_logging_active
#endif
  ) {
    // Route for going to debug logging web page
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/html", debug_logger_processor());
      request->send(response);
    });
  }

  // Define the handler to import can log
  server.on(
      "/import_can_log", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Ready to receive file.");  // Response when request is made
      },
      handleFileUpload);

#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    // Define the handler to export can log
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_can_writing();
      request->send(SD, CAN_LOG_FILE, String(), true);
      resume_can_writing();
    });

    // Define the handler to delete can log
    server.on("/delete_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_can_log();
      request->send(200, "text/plain", "Log file deleted");
    });
  }
#endif  // SDCARD

#ifdef SDCARD
  if (datalayer.system.info.SD_logging_active) {
    // Define the handler to delete log file
    server.on("/delete_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_log();
      request->send(200, "text/plain", "Log file deleted");
    });

    // Define the handler to export debug log
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_log_writing();
      request->send(SD, LOG_FILE, String(), true);
      resume_log_writing();
    });
  } else
#endif  // SDCARD
  {
    // Define the handler to export debug log
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0) {
        logs = "No logs available.";
      }

      String filename = "log_" + format_ms_stamp(millis64()) + ".txt";

      // Use request->send with dynamic headers
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
      request->send(response);
    });
  }

  // Route for going to cellmonitor web page
  def_route_with_auth("/cellmonitor", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { send_cellmonitor_page(request); });

  // Route for going to event log web page
  def_route_with_auth("/events", server, HTTP_GET, [](AsyncWebServerRequest* request) { send_events_page(request); });

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
    erase_phy_cal_data();
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.println("Factory reset performed from the web interface.");
    request->send(200, "text/html", "OK");
  });

  const char* boolSettingNames[] = {
      "DBLBTR",       "CNTCTRL",      "CNTCTRLDBL",    "PWMCNTCTRL",  "PERBMSRESET",   "STATICIP",     "REMBMSRESET",
      "EXTPRECHARGE", "USBENABLED",   "CANLOGUSB",     "WEBENABLED",  "WIFIAPENABLED", "MQTTENABLED",  "NOINVDISC",
      "HADISC",       "MQTTCELLV",    "GTWRHD",        "DIGITALHVIL", "PERFPROFILE",   "INTERLOCKREQ", "SOCESTIMATED",
      "PYLONOFFSET",  "PYLONORDER",   "DEYEBYD",       "NCCONTACTOR", "TRIBTR",        "CNTCTRLTRI",   "ESPNOWENABLED",
      "PRIMOGEN24",   "CTINVERT",     "LOWPASSFILTER", "WEBAUTH",     "SLOWCANINV",    "CHGTAPERSOC",  "MEASURECPUTEMP",
      "SYSLOGEN",     "PERBMSDEFSOC", "PERBMSSKIPBAL", "INVOFFGRID",  "CHGESTIMATED",  "MQTTHEAP",     "HADISCFWU",
      "INVACCREB",    "LEAFAUTOOFS",
#ifdef SDCARD
      "SDLOGENABLED", "CANLOGSD",
#endif  // SDCARD
  };

  const char* uintSettingNames[] = {
      "BATTCVMAX",  "BATTCVMIN",    "MAXPRETIME",    "MAXPREFREQ",    "WIFICHANNEL",   "DCHGPOWER",     "CHGPOWER",
      "MQTTPORT",   "MQTTTIMEOUT",  "SOFAR_ID",      "PYLONSEND",     "INVCELLS",      "INVMODULES",    "INVCELLSPER",
      "INVVLEVEL",  "INVCAPACITY",  "INVBTYPE",      "PRECHGMS",      "PWMFREQ",       "PWMHOLD",       "GTWCOUNTRY",
      "GTWMAPREG",  "GTWCHASSIS",   "GTWPACK",       "LEDMODE",       "GPIOOPT1",      "GPIOOPT2",      "GPIOOPT3",
      "INVSUNTYPE", "GPIOOPT4",     "CTVNOM",        "CTANOM",        "CTATTEN",       "PYLONBAUD",     "PYLONBRAND",
      "DALYPWRPCT", "DALYPWRDV",    "DALYDVSTART",   "DALYPWRDEG",    "DALYPWR0C",     "GPIOOPT5",      "GPIOOPT6",
      "INVICNT",    "FOXESSTYPE",   "FOXESSSUBTYPE", "FOXESSMODULES", "CHGTAPERSTART", "CHGTAPERFLOOR", "SYSLOGPORT",
      "SYSLOGFAC",  "PERBMSRESETH",
  };

  const char* stringSettingNames[] = {"APPASSWORD", "HOSTNAME",    "MQTTSERVER", "MQTTUSER",  "MQTTPASSWORD",
                                      "HTTPUSER",   "HTTPPASS",    "LOCALIP",    "GATEWAY",   "SUBNET",
                                      "DNS",        "HADISCTOPIC", "SYSLOGIP",   "ESPNOWMACS"};

  // Handles the form POST from UI to save settings of the common image
  server.on("/saveSettings", HTTP_POST,
            [boolSettingNames, stringSettingNames, uintSettingNames](AsyncWebServerRequest* request) {
              BatteryEmulatorSettingsStore settings;
              auto webAuthParam = request->getParam("WEBAUTH", true);
              auto httpUserParam = request->getParam("HTTPUSER", true);
              auto httpPassParam = request->getParam("HTTPPASS", true);
              auto httpPassConfirmParam = request->getParam("HTTPPASSCONFIRM", true);

              bool requestedWebAuth = webAuthParam != nullptr && webAuthParam->value() == "on";
              String requestedHttpUser =
                  httpUserParam != nullptr ? httpUserParam->value() : settings.getString("HTTPUSER", "admin");
              String requestedHttpPass = (httpPassParam != nullptr && !httpPassParam->value().isEmpty())
                                             ? httpPassParam->value()
                                             : settings.getString("HTTPPASS");

              String requestedHttpPassConfirm =
                  (httpPassConfirmParam != nullptr && !httpPassConfirmParam->value().isEmpty())
                      ? httpPassConfirmParam->value()
                      : requestedHttpPass;

              if (requestedHttpPass != requestedHttpPassConfirm) {
                request->send(400, "text/plain", "Web interface passwords do not match.");
                return;
              }

              if (requestedWebAuth && (requestedHttpUser.isEmpty() || requestedHttpPass.isEmpty())) {
                request->send(400, "text/plain",
                              "Set a username and password before enabling web interface password protection.");
                return;
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
                  auto type = p->value().toFloat() * 10.0f;
                  settings.saveUInt("BATTPVMAX", (int)type);
                } else if (p->name() == "BATTPVMIN") {
                  auto type = p->value().toFloat() * 10.0f;
                  settings.saveUInt("BATTPVMIN", (int)type);
                } else if (p->name() == "charger") {
                  auto type = static_cast<ChargerType>(atoi(p->value().c_str()));
                  settings.saveUInt("CHGTYPE", (int)type);
                } else if (p->name() == "CHGSTARQ") {
                  // Stored as the CHG_STA_RQ bits themselves. 11b is the charge stop request and
                  // is not offered, so anything else falls back to "no request".
                  uint8_t request = atoi(p->value().c_str());
                  if (request > 2) {
                    request = 0;
                  }
                  settings.saveUInt("CHGSTARQ", request);
                  // Unlike the other settings this one is taken into use without a reboot, so the
                  // reset offered below sends the newly chosen request rather than the old one.
                  user_selected_LEAF_chg_sta_rq = request;
                } else if (p->name() == "CHGCOMM") {
                  auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
                  settings.saveUInt("CHGCOMM", (int)type);
                } else if (p->name() == "EQSTOP") {
                  auto type = static_cast<STOP_BUTTON_BEHAVIOR>(atoi(p->value().c_str()));
                  settings.saveUInt("EQSTOP", (int)type);
                } else if (p->name() == "BATT2COMM") {
                  auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
                  settings.saveUInt("BATT2COMM", (int)type);
                } else if (p->name() == "BATT3COMM") {
                  auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
                  settings.saveUInt("BATT3COMM", (int)type);
                } else if (p->name() == "shunttype") {
                  auto type = static_cast<ShuntType>(atoi(p->value().c_str()));
                  settings.saveUInt("SHUNTTYPE", (int)type);
                } else if (p->name() == "SHUNTCOMM") {
                  auto type = static_cast<comm_interface>(atoi(p->value().c_str()));
                  settings.saveUInt("SHUNTCOMM", (int)type);
                } else if (p->name() == "CTOFFSET") {
                  // allow negative offsets so save as string
                  settings.saveString("CTOFFSET", p->value().c_str());
                } else if (p->name() == "CTATTEN") {
                  auto type = static_cast<adc_attenuation_t>(atoi(p->value().c_str()));
                  settings.saveUInt("CTATTEN", (int)type);
                } else if (p->name() == "CPUTEMPOFFSET") {
                  // allow negative offsets so save as number
                  settings.saveInt("CPUTEMPOFFSET", atoi(p->value().c_str()));
                } else if (p->name() == "SSID") {
                  settings.saveString("SSID", p->value().c_str());
                  ssid = settings.getString("SSID", "").c_str();
                } else if (p->name() == "PASSWORD") {
                  if (!p->value().isEmpty()) {  // blank = keep existing (field is rendered empty)
                    settings.saveString("PASSWORD", p->value().c_str());
                  }
                  password = settings.getString("PASSWORD", "").c_str();
                } else if (p->name() == "MQTTPUBLISHMS") {
                  auto interval = atoi(p->value().c_str()) * 1000;  // Convert seconds to milliseconds
                  settings.saveUInt("MQTTPUBLISHMS", interval);
                }

                for (auto& uintSetting : uintSettingNames) {
                  if (p->name() == uintSetting) {
                    auto value = atoi(p->value().c_str());
                    settings.saveUInt(uintSetting, value);
                  }
                }

                for (auto& stringSetting : stringSettingNames) {
                  if (p->name() == stringSetting) {
                    // Password fields are rendered blank; an empty value means "keep unchanged".
                    const bool isPasswordField =
                        (std::string(stringSetting) == "APPASSWORD" || std::string(stringSetting) == "MQTTPASSWORD" ||
                         std::string(stringSetting) == "HTTPPASS");
                    if (isPasswordField && p->value().isEmpty()) {
                      continue;  // keep existing stored password
                    }
                    if (settings.getString(stringSetting) != p->value()) {
                      settings.saveString(stringSetting, p->value().c_str());
                    }
                  }
                }
              }

              for (auto& boolSetting : boolSettingNames) {
                auto p = request->getParam(boolSetting, true);
                // The comparison default must match what the firmware boots with when the
                // key is unset, or saving that state writes nothing and the page keeps
                // disagreeing with the firmware. Only three bools boot true: WIFIAPENABLED,
                // LEAFAUTOOFS and GTWRHD (whose boot fallback is the driver global).
                bool default_value = false;
                if (std::string(boolSetting) == std::string("WIFIAPENABLED") ||
                    std::string(boolSetting) == std::string("LEAFAUTOOFS")) {
                  default_value = true;
                } else if (std::string(boolSetting) == std::string("GTWRHD")) {
                  default_value = user_selected_tesla_GTW_rightHandDrive;
                }
                const bool value = p != nullptr && p->value() == "on";
                if (settings.getBool(boolSetting, default_value) != value) {
                  settings.saveBool(boolSetting, value);
                }
              }

              // The double/triple battery checkboxes are hidden in the UI for integrations
              // that don't implement parallel batteries. Make sure a previously stored
              // value can't survive a switch to such an integration.
              auto selectedBatteryType = static_cast<BatteryType>(settings.getUInt("BATTTYPE", (int)BatteryType::None));
              if (!battery_supports_double(selectedBatteryType) && settings.getBool("DBLBTR", false)) {
                settings.saveBool("DBLBTR", false);
              }
              if (!battery_supports_triple(selectedBatteryType) && settings.getBool("TRIBTR", false)) {
                settings.saveBool("TRIBTR", false);
              }

              // The page offers a BMS reset when the starting sequence request was changed, since
              // the LBC only reads that signal while it powers up. Done after every setting is
              // stored so the reset runs against the saved configuration.
              auto bmsResetParam = request->getParam("CHGSTARQRESET", true);
              if (bmsResetParam != nullptr && bmsResetParam->value() == "1") {
                if (periodic_bms_reset || remote_bms_reset) {
                  LOG_SET_NEXT_SEVERITY(5);  // notice
                  logging.println("BMS reset requested from the settings page.");
                  start_bms_reset();
                } else {
                  LOG_SET_NEXT_SEVERITY(4);  // warning
                  logging.println(
                      "BMS reset requested from the settings page, but no BMS reset method is enabled. "
                      "The new setting applies at the next BMS power cycle.");
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
  update_int_setting("/updateUseScaledSOC", [](int value) { datalayer.battery_settings.soc_scaling_active = value; });

  // Route for enabling recovery mode charging
  update_int_setting("/enableRecoveryMode",
                     [](int value) { datalayer.battery_settings.user_requests_forced_charging_recovery_mode = value; });

  // Route for editing SOCMax
  update_string_setting("/updateSocMax", [](String value) {
    datalayer.battery_settings.max_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });

  // Route for pause/resume Battery emulator
  update_string("/pause", [](String value) { setBatteryPause(value == "true" || value == "1", false); });

  // Route for equipment stop/resume
  update_string("/equipmentStop", [](String value) {
    if (value == "true" || value == "1") {
      setBatteryPause(true, false,
                      EquipmentStop::STOP);  //Pause battery, do not pause CAN, equipment stop on (store to flash)
    } else {
      setBatteryPause(false, false, EquipmentStop::RESUME);
    }
  });

  // Route for editing SOC Calibration BYD
  update_string_setting("/editCalTargetSOC", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetSOC = static_cast<uint16_t>(value.toFloat());
  });

  // Save auto-calibrate enabled flag to RAM + NVM
  def_route_with_auth("/editBydAtto3AutoCalEnabled", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      bool enabled = request->getParam("value")->value().toInt() != 0;
      datalayer_extended.bydAtto3.auto_calibrate_soc_enabled = enabled;
      Preferences prefs;
      prefs.begin("batterySettings", false);
      prefs.putBool("BYDAUTOCALEN", enabled);
      prefs.end();
    }
    request->send(200, "text/plain", "OK");
  });

  // Save auto-calibrate drift threshold to RAM + NVM
  def_route_with_auth("/editBydAtto3AutoCalDriftPercent", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      int value = request->getParam("value")->value().toInt();
      if (value >= 1 && value <= 20) {
        datalayer_extended.bydAtto3.auto_calibrate_soc_drift_percent = (uint8_t)value;
        Preferences prefs;
        prefs.begin("batterySettings", false);
        prefs.putUInt("BYDAUTOCALDRIFT", (uint8_t)value);
        prefs.end();
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // Save native BMS termination enabled flag to RAM + NVM
  def_route_with_auth("/editBydAtto3NativeTermination", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      bool enabled = request->getParam("value")->value().toInt() != 0;
      datalayer_extended.bydAtto3.native_termination_enabled = enabled;
      Preferences prefs;
      prefs.begin("batterySettings", false);
      prefs.putBool("BYDNATTERM", enabled);
      prefs.end();
    }
    request->send(200, "text/plain", "OK");
  });

  // Save balancing enabled flag to RAM + NVM
  def_route_with_auth("/editBydAtto3BalancingEnabled", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      bool enabled = request->getParam("value")->value().toInt() != 0;
      datalayer_extended.bydAtto3.balancing_enabled = enabled;
      Preferences prefs;
      prefs.begin("batterySettings", false);
      prefs.putBool("BYDBALEN", enabled);
      prefs.end();
    }
    request->send(200, "text/plain", "OK");
  });

  // Save balancing hold duration to RAM + NVM
  def_route_with_auth("/editBydAtto3BalancingMinutes", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      int value = request->getParam("value")->value().toInt();
      if (value >= 1 && value <= 1440) {
        datalayer_extended.bydAtto3.balancing_hold_minutes = (uint16_t)value;
        Preferences prefs;
        prefs.begin("batterySettings", false);
        prefs.putUInt("BYDBALMIN", (uint16_t)value);
        prefs.end();
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // Route for editing AH Calibration BYD
  update_string_setting("/editCalTargetAH", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetAH = static_cast<uint16_t>(value.toFloat());
  });

  // Isolation monitor control (RoutineControl 0x2008). One setting, applied to both batteries.
  def_route_with_auth("/bydAtto3IsoDisable", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer_extended.bydAtto3.UserRequestIsoRoutineDisable = true;
    datalayer_extended.bydAtto3_2.UserRequestIsoRoutineDisable = true;
    request->send(200, "text/plain", "OK");
  });
  def_route_with_auth("/bydAtto3IsoEnable", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer_extended.bydAtto3.UserRequestIsoRoutineEnable = true;
    datalayer_extended.bydAtto3_2.UserRequestIsoRoutineEnable = true;
    request->send(200, "text/plain", "OK");
  });
  def_route_with_auth("/bydAtto3KeepIsoDisabled", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      bool enabled = request->getParam("value")->value().toInt() != 0;
      datalayer_extended.bydAtto3.keep_iso_disabled = enabled;
      datalayer_extended.bydAtto3_2.keep_iso_disabled = enabled;
      Preferences prefs;
      prefs.begin("batterySettings", false);
      prefs.putBool("BYDKEEPISOOFF", enabled);
      prefs.end();
    }
    request->send(200, "text/plain", "OK");
  });

  // Battery 2 auto-calibration routes
  update_string_setting("/editCalTargetSOC2", [](String value) {
    datalayer_extended.bydAtto3_2.calibrationTargetSOC = static_cast<uint16_t>(value.toFloat());
  });

  update_string_setting("/editCalTargetAH2", [](String value) {
    datalayer_extended.bydAtto3_2.calibrationTargetAH = static_cast<uint16_t>(value.toFloat());
  });

  def_route_with_auth("/editBydAtto3AutoCalEnabled2", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      bool enabled = request->getParam("value")->value().toInt() != 0;
      datalayer_extended.bydAtto3_2.auto_calibrate_soc_enabled = enabled;
      Preferences prefs;
      prefs.begin("batterySettings", false);
      prefs.putBool("BYDAUTOCALEN2", enabled);
      prefs.end();
    }
    request->send(200, "text/plain", "OK");
  });

  def_route_with_auth("/editBydAtto3AutoCalDriftPercent2", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      int value = request->getParam("value")->value().toInt();
      if (value >= 1 && value <= 20) {
        datalayer_extended.bydAtto3_2.auto_calibrate_soc_drift_percent = (uint8_t)value;
        Preferences prefs;
        prefs.begin("batterySettings", false);
        prefs.putUInt("BYDAUTOCALDRFT2", (uint8_t)value);
        prefs.end();
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // Route for editing SOCMin
  update_string_setting("/updateSocMin", [](String value) {
    datalayer.battery_settings.min_percentage = static_cast<uint16_t>(value.toFloat() * 100);
  });

  // Route for editing MaxChargeA
  update_string_setting("/updateMaxChargeA", [](String value) {
    datalayer.battery_settings.max_user_set_charge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing MaxDischargeA
  update_string_setting("/updateMaxDischargeA", [](String value) {
    datalayer.battery_settings.max_user_set_discharge_dA = static_cast<uint16_t>(value.toFloat() * 10);
  });

  for (const auto& cmd : battery_commands) {
    auto route = String("/") + cmd.identifier;
    server.on(
        route.c_str(), HTTP_PUT,
        [cmd](AsyncWebServerRequest* request) {
          if (webserver_auth_is_ready() && !request->authenticate(http_username.c_str(), http_password.c_str())) {
            return request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
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
          if (battIndex == "2") {
            batt = battery3;
          }
          if (batt) {
            cmd.action(batt);
          }
          request->send(200, "text/plain", "Command performed.");
        });

    register_dump_can_route(server);
  }

  // Route for editing BATTERY_USE_VOLTAGE_LIMITS
  update_int_setting("/updateUseVoltageLimit",
                     [](int value) { datalayer.battery_settings.user_set_voltage_limits_active = value; });

  // Route for editing MaxChargeVoltage
  update_string_setting("/updateMaxChargeVoltage", [](String value) {
    datalayer.battery_settings.max_user_set_charge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing MaxDischargeVoltage
  update_string_setting("/updateMaxDischargeVoltage", [](String value) {
    datalayer.battery_settings.max_user_set_discharge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing BMSresetDuration
  update_string_setting("/updateBMSresetDuration", [](String value) {
    datalayer.battery_settings.user_set_bms_reset_duration_ms = static_cast<uint32_t>(value.toFloat() * 1000);
  });

  // Route for the settings page "Perform a BMS reset now" button. Runs the same sequence as the
  // MQTT BMSRESET command, and says why when start_bms_reset() would silently do nothing.
  def_route_with_auth("/startBMSReset", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!periodic_bms_reset && !remote_bms_reset) {
      request->send(409, "text/plain", "No BMS reset method is active yet. Save the settings and reboot first.");
      return;
    }
    if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
      request->send(409, "text/plain", "A BMS reset is already in progress.");
      return;
    }
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.println("BMS reset requested from the settings page.");
    start_bms_reset();
    request->send(200, "text/plain", "OK");
  });

  // Route for the fake battery's Voltage and SOH, edited per pack on its More Battery Info tab.
  // Runtime values like before, so nothing is stored.
  def_route_with_auth("/updateFakeBattery", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    Battery* const packs[] = {battery, battery2, battery3};
    const long index = request->hasParam("battery") ? request->getParam("battery")->value().toInt() : 0;
    Battery* const batt = (index >= 1 && index <= 3) ? packs[index - 1] : nullptr;
    const AsyncWebParameter* voltage = request->getParam("Voltage");
    const AsyncWebParameter* soh = request->getParam("SOH");
    const float value = voltage ? voltage->value().toFloat() : (soh ? soh->value().toFloat() : -1.0f);
    // Negated range test, so that a NaN is rejected as well
    if (!batt || !(value >= 0.0f && value <= (voltage ? 5000.0f : 100.0f))) {
      request->send(400, "text/plain", "Invalid value");
      return;
    }
    if (voltage) {
      batt->set_fake_voltage(value);
    } else {
      batt->set_fake_soh(value);
    }
    request->send(200, "text/plain", "Updated successfully");
  });

  // Route for editing balancing enabled
  update_int_setting("/TeslaBalAct", [](int value) { datalayer.battery_settings.user_requests_balancing = value; });

  // Route for editing balancing max time
  update_string_setting("/BalTime", [](String value) {
    datalayer.battery_settings.balancing_max_time_ms = static_cast<uint32_t>(value.toFloat() * 60000);
  });

  // Route for editing balancing max power
  update_string_setting("/BalFloatPower", [](String value) {
    datalayer.battery_settings.balancing_float_power_W = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing balancing max pack voltage
  update_string_setting("/BalMaxPackV", [](String value) {
    datalayer.battery_settings.balancing_max_pack_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10);
  });

  // Route for editing balancing max cell voltage
  update_string_setting("/BalMaxCellV", [](String value) {
    datalayer.battery_settings.balancing_max_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing balancing max cell voltage deviation
  update_string_setting("/BalMaxDevCellV", [](String value) {
    datalayer.battery_settings.balancing_max_deviation_cell_voltage_mV = static_cast<uint16_t>(value.toFloat());
  });

  if (charger) {
    // Route for editing ChargerTargetV
    update_string_setting(
        "/updateChargeSetpointV", [](String value) { datalayer.charger.charger_setpoint_HV_VDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV);
        });

    // Route for editing ChargerTargetA
    update_string_setting(
        "/updateChargeSetpointA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC = value.toFloat(); },
        [](String value) {
          float val = value.toFloat();
          return (val <= CHARGER_MAX_A) && (val <= datalayer.battery_settings.max_user_set_charge_dA) &&
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
    hold_pins_across_reset();
    graceful_restart();
  });

  // Initialize ElegantOTA
  init_ElegantOTA();

  // Start server
  server.begin();
}

void webserver_tick() {
  can_dump_drain_tick();

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

/* One battery card's worth of numbers, filled either from a single pack or from the aggregate,
   so the three (four) cards on the main page all go through the same renderer. */
struct BatteryCardView {
  uint32_t total_capacity_Wh;
  uint32_t reported_total_capacity_Wh;
  uint32_t remaining_capacity_Wh;
  uint32_t reported_remaining_capacity_Wh;
  uint32_t max_charge_power_W;
  uint32_t max_discharge_power_W;
  int32_t active_power_W;
  uint16_t real_soc;
  uint16_t reported_soc;
  uint16_t soh_pptt;
  bool soh_available;
  uint16_t voltage_dV;
  uint16_t max_charge_current_dA;
  uint16_t max_discharge_current_dA;
  uint16_t cell_max_voltage_mV;
  uint16_t cell_min_voltage_mV;
  uint16_t max_cell_voltage_deviation_mV;
  int16_t current_dA;
  int16_t temperature_max_dC;
  int16_t temperature_min_dC;
};

static void fill_card_view(BatteryCardView& v, const DATALAYER_BATTERY_TYPE& pack) {
  v.total_capacity_Wh = pack.info.total_capacity_Wh;
  v.reported_total_capacity_Wh = pack.info.reported_total_capacity_Wh;
  v.remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
  v.reported_remaining_capacity_Wh = pack.status.reported_remaining_capacity_Wh;
  /* What this pack's BMS asked for, not what the system settled on. Only the system limits
     are ever converted to a current, so derive this pack's from its own voltage. */
  v.max_charge_power_W = pack.status.bms_max_charge_power_W;
  v.max_discharge_power_W = pack.status.bms_max_discharge_power_W;
  v.max_charge_current_dA = 0;
  v.max_discharge_current_dA = 0;
  if (pack.status.voltage_dV > 10) {
    v.max_charge_current_dA = power_W_to_current_dA(v.max_charge_power_W, pack.status.voltage_dV);
    v.max_discharge_current_dA = power_W_to_current_dA(v.max_discharge_power_W, pack.status.voltage_dV);
  }
  v.active_power_W = pack.status.active_power_W;
  v.real_soc = pack.status.real_soc;
  v.reported_soc = pack.status.reported_soc;
  v.soh_pptt = pack.status.soh_pptt;
  v.soh_available = pack.status.soh_available;
  v.voltage_dV = pack.status.voltage_dV;
  v.cell_max_voltage_mV = pack.status.cell_max_voltage_mV;
  v.cell_min_voltage_mV = pack.status.cell_min_voltage_mV;
  v.max_cell_voltage_deviation_mV = pack.info.max_cell_voltage_deviation_mV;
  v.current_dA = pack.status.current_dA;
  v.temperature_max_dC = pack.status.temperature_max_dC;
  v.temperature_min_dC = pack.status.temperature_min_dC;
}

static void fill_card_view_aggregate(BatteryCardView& v) {
  const DATALAYER_AGGREGATE_TYPE& a = datalayer.aggregate;
  v.total_capacity_Wh = a.total_capacity_Wh;
  v.reported_total_capacity_Wh = a.reported_total_capacity_Wh;
  v.remaining_capacity_Wh = a.remaining_capacity_Wh;
  v.reported_remaining_capacity_Wh = a.reported_remaining_capacity_Wh;
  v.max_charge_power_W = a.max_charge_power_W;
  v.max_discharge_power_W = a.max_discharge_power_W;
  v.active_power_W = a.active_power_W;
  v.real_soc = a.real_soc;
  v.reported_soc = a.reported_soc;
  v.soh_pptt = a.soh_pptt;
  v.soh_available = a.soh_available;
  v.voltage_dV = a.voltage_dV;
  v.max_charge_current_dA = a.max_charge_current_dA;
  v.max_discharge_current_dA = a.max_discharge_current_dA;
  v.cell_max_voltage_mV = a.cell_max_voltage_mV;
  v.cell_min_voltage_mV = a.cell_min_voltage_mV;
  v.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
  v.current_dA = a.current_dA;
  v.temperature_max_dC = a.temperature_max_dC;
  v.temperature_min_dC = a.temperature_min_dC;
}

/* Card background colours, kept as small helpers so the four call sites stay readable */
static String emulator_status_color() {
  switch (get_emulator_status()) {
    case EMULATOR_STATUS::STATUS_WARNING:
      return "#F5CC00;";
    case EMULATOR_STATUS::STATUS_ERROR:
      return "#A70107;";
    case EMULATOR_STATUS::STATUS_UPDATING:
      return "#2B35AF;";  // Blue in test mode
    default:
      return "#2D3F2F;";
  }
}

static String system_status_color() {
  return (datalayer.system.status.system_status == FAULT) ? "#A70107;" : "#2D3F2F;";
}

/* The combined card describes the installation, not a battery, so "Battery charging!" drops its
   first word and the next one takes the capital. A "(Battery limiting)" that follows names the
   limiting factor rather than the subject, and stays as it is. */
static String installation_status_text(const char* status) {
  String text(status);
  if (text.startsWith("Battery ")) {
    text.remove(0, 8);
    if (text.length() > 0 && text[0] >= 'a' && text[0] <= 'z') {
      text.setCharAt(0, text[0] - ('a' - 'A'));
    }
  }
  return text;
}

/* Render one battery card. pack_index 0 is the combined installation, 1-3 are the packs.

   The combined card - or the single pack card when only one battery is configured - carries
   exactly what leaves for the inverter: the scaled SOC and capacity, the limits the inverter is
   actually given along with what is setting them, and the charging status.

   A pack card in a multi-battery setup carries only that pack: real SOC and capacity with no
   scaling, since the SOC window is a system-wide setting that only the inverter ever sees, and
   the power limits its own BMS asked for rather than the ones the system settled on. */
static void render_battery_card(String& content, const String& style, const BatteryCardView& v, uint8_t pack_index) {
  const bool multi = (datalayer.system.info.configured_batteries > 1);
  const bool system_card = (pack_index == 0) || !multi;
  const bool scaled = system_card && datalayer.battery_settings.soc_scaling_active;

  content += "<div style='" + style + "'>";

  if (scaled) {
    content += "<h4 style='color: white;'>Scaled SOC: " + String(v.reported_soc / 100.0f, 2) +
               "&percnt; (real: " + String(v.real_soc / 100.0f, 2) + "&percnt;)</h4>";
  } else {
    content += "<h4 style='color: white;'>SOC: " + String(v.real_soc / 100.0f, 2) + "&percnt;</h4>";
  }

  // Unknown until the integration has decoded a state of health, rather than the soh_pptt default
  // shown as if it had been read from the pack. The combined card follows the same rule.
  if (v.soh_available) {
    content += "<h4 style='color: white;'>SOH: " + String(v.soh_pptt / 100.0f, 2) + "&percnt;</h4>";
  } else {
    content += "<h4 style='color: white;'>SOH: Unknown</h4>";
  }
  content += "<h4 style='color: white;'>Voltage: " + String(v.voltage_dV / 10.0f, 1) +
             " V &nbsp; Current: " + String(v.current_dA / 10.0f, 1) + " A</h4>";
  content += formatPowerValue("Power", (float)v.active_power_W, "", 1);

  if (scaled) {
    content +=
        "<h4 style='color: white;'>Scaled total capacity: " + formatPowerValue(v.reported_total_capacity_Wh, "h", 1) +
        " (real: " + formatPowerValue(v.total_capacity_Wh, "h", 1) + ")</h4>";
    content += "<h4 style='color: white;'>Scaled remaining capacity: " +
               formatPowerValue(v.reported_remaining_capacity_Wh, "h", 1) +
               " (real: " + formatPowerValue(v.remaining_capacity_Wh, "h", 1) + ")</h4>";
  } else {
    content += formatPowerValue("Total capacity", v.total_capacity_Wh, "h", 1);
    content += formatPowerValue("Remaining capacity", v.remaining_capacity_Wh, "h", 1);
  }

  if (system_card) {
    const bool stopped = datalayer.system.info.equipment_stop_active;
    const String limit_color = stopped ? "red" : "white";
    content += formatPowerValue("Max discharge power", v.max_discharge_power_W, "", 1, limit_color);
    content += formatPowerValue("Max charge power", v.max_charge_power_W, "", 1, limit_color);
    content += "<h4 style='color: " + limit_color +
               ";'>Max discharge current: " + String(v.max_discharge_current_dA / 10.0f, 1) + " A";
    if (!stopped) {
      if (datalayer.battery_settings.remote_settings_limit_discharge) {
        content += " (Remote)";
      } else if (datalayer.battery_settings.user_settings_limit_discharge) {
        content += " (Manual)";
      } else {
        content += " (BMS)";
      }
    }
    content += "</h4><h4 style='color: " + limit_color +
               ";'>Max charge current: " + String(v.max_charge_current_dA / 10.0f, 1) + " A";
    if (!stopped) {
      if (datalayer.battery_settings.remote_settings_limit_charge) {
        content += " (Remote)";
      } else if (datalayer.battery_settings.user_settings_limit_charge) {
        content += " (Manual)";
      } else {
        content += " (BMS)";
      }
    }
    content += "</h4>";
  } else {
    /* A zero here is a limit like any other: this pack's BMS allows nothing right now - full,
       empty, too cold, faulted - so it shows as 0 W, exactly as the system card would. */
    content += formatPowerValue("Max discharge power", v.max_discharge_power_W, "", 1);
    content += formatPowerValue("Max charge power", v.max_charge_power_W, "", 1);
    content += "<h4 style='color: white;'>Max discharge current: " + String(v.max_discharge_current_dA / 10.0f, 1) +
               " A</h4><h4 style='color: white;'>Max charge current: " + String(v.max_charge_current_dA / 10.0f, 1) +
               " A</h4>";
  }

  /* Cells and temperatures are a property of a pack, not of the installation: the combined card
     would only be repeating the extremes already visible on the cards right below it. The
     aggregate still carries them, because the inverter is told them. */
  if (pack_index != 0) {
    content +=
        "<h4>Cell min/max: " + String(v.cell_min_voltage_mV) + " mV / " + String(v.cell_max_voltage_mV) + " mV</h4>";
    uint16_t cell_delta_mv = v.cell_max_voltage_mV - v.cell_min_voltage_mV;
    if (cell_delta_mv > v.max_cell_voltage_deviation_mV) {
      content += "<h4 style='color: red;'>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    } else {
      content += "<h4>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    }
    content += "<h4>Temperature min/max: " + String(v.temperature_min_dC / 10.0f, 1) + " &deg;C / " +
               String(v.temperature_max_dC / 10.0f, 1) + " &deg;C</h4>";
  }

  if ((pack_index == 1) && battery && battery->supports_real_BMS_status()) {
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

  if (system_card) {
    const char* status = get_charging_status_text(v.current_dA, datalayer.battery_settings.inverter_limits_charge,
                                                  datalayer.battery_settings.inverter_limits_discharge,
                                                  datalayer.battery_settings.user_settings_limit_charge,
                                                  datalayer.battery_settings.user_settings_limit_discharge);
    // Only the combined card, which is the installation. A single pack keeps its own wording.
    content += "<h4>" + (pack_index == 0 ? installation_status_text(status) : String(status)) + "</h4>";
  } else if (v.current_dA == 0) {
    content += "<h4>Battery idle</h4>";
  } else if (v.current_dA < 0) {
    content += "<h4>Battery discharging!</h4>";
  } else {
    content += "<h4>Battery charging!</h4>";
  }

  content += "</div>";
}

/* The main page is split in two. Its styles, buttons and scripts never change, so they are sent
   straight from flash when the page is opened and never rebuilt. Only the live part in between -
   the status blocks and the battery cards - is rendered per request, both into the page and on
   its own for /live, which the page polls instead of reloading itself.

   The live part is rendered into one buffer reserved up front. Arduino's String grows by
   reallocating, so appending a few hundred pieces onto an empty String is a few hundred chances
   to ask a heap that has been up for weeks for an ever larger contiguous block - and when one of
   those fails, concat() drops the append and returns silently, leaving a page with elements
   missing. Reserving turns those chances into one, and that one is checked. Three packs come to
   roughly 10 kB, so this is headroom rather than a land grab. The buffer is sent as it is, with
   no second copy of it on the way out (see MainPageResponse). */
static constexpr size_t LIVE_RESERVE_BYTES = 12288;

/* Differs on every boot, so a page left open can tell that the emulator restarted - after a
   reboot or an OTA update - and reload itself to pick up the page of the running firmware. Drawn
   on first use rather than at startup, when the radio that seeds the RNG may not be up yet. */
static uint32_t boot_id() {
  static uint32_t id = 0;
  if (id == 0) {
    id = esp_random() | 1;
  }
  return id;
}

static bool render_live(CheckedHtml& content) {
  if (content.reserve(LIVE_RESERVE_BYTES)) {
    /* What the fixed part of the page needs from this render: which button of each pair to show,
       and which boot served it. */
    char state[96];
    snprintf(state, sizeof(state), "<i id='S' hidden data-b='%08lx' data-p='%d' data-e='%d'></i>",
             (unsigned long)boot_id(), emulator_pause_request_ON ? 1 : 0,
             datalayer.system.info.equipment_stop_active ? 1 : 0);
    content += state;

    // Start content block
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
    content += "<h4>";
#if defined(GIT_TAG) && defined(GITHUB_ORG) && defined(GITHUB_REPO)
    content += "<a href='https://github.com/" GITHUB_ORG "/" GITHUB_REPO "/releases/tag/" GIT_TAG
               "' target='_blank' style='color:#fff'>" +
               String(version_number) + "</a>";
#elif defined(GITHUB_PR) && defined(GITHUB_ORG) && defined(GITHUB_REPO)
    content += "<a href='https://github.com/" GITHUB_ORG "/" GITHUB_REPO "/pull/" GITHUB_PR
               "' target='_blank' style='color:#fff'>" +
               String(version_number) + "</a>";
#else
    content += String(version_number);
#endif

// Show hardware used:
#ifdef HW_LILYGO
    content += " running on LilyGo T-CAN485";
#endif  // HW_LILYGO
#ifdef HW_LILYGO2CAN
    content += " running on LilyGo T_2CAN";
#endif  // HW_LILYGO2CAN
#ifdef HW_BECOM
    content += " running on BECom";
#endif  // HW_BECOM
#ifdef HW_STARK
    content += " running on Stark CMR Module";
#endif  // HW_STARK
#ifdef HW_WAVESHARE
    content += " running on Waveshare ESP32-S3-RS485-CAN";
#endif  // HW_WAVESHARE
    if (datalayer.system.info.CPU_measurement_enabled) {
      content += " @ " + String(datalayer.system.info.CPU_temperature, 1) + " &deg;C";
    }
    content += "</h4><h4>for " + format_ms_string(millis64()) + "</h4>";
    if (datalayer.system.info.performance_measurement_active) {
      content +=
          "<h4>Free heap: " + String(ESP.getFreeHeap()) + ", max alloc: " + String(ESP.getMaxAllocHeap()) + "</h4>";
      FlashMode_t mode = ESP.getFlashChipMode();
      content += "<h4>Flash mode: " +
                 String(mode == FM_QIO    ? "QIO"
                        : mode == FM_QOUT ? "QOUT"
                        : mode == FM_DIO  ? "DIO"
                        : mode == FM_DOUT ? "DOUT"
                                          : /*mode == FM_UNKNOWN*/ "Unknown") +
                 ", size: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB</h4>";
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
    }

    // SSID/RSSI/channel are WiFi-specific; only show them when configured
    if (!ssid.empty()) {
      content += "<h4>SSID: " + html_escape(ssid.c_str());
      if (wifi_connected()) {
        // Get and display the signal strength (RSSI) and channel
        content += " RSSI: " + String(WiFi.RSSI()) + " dBm Ch: " + String(WiFi.channel());
      }
      content += "</h4>";
    }
    // Reachability/hostname/IP reflect the active interface
    if (network_connected()) {
      content += "<h4>" + html_escape(active_hostname()) + " [" + WiFi.localIP().toString();
      if (espnow_enabled) {
        // MAC is the station address, which is also the source address of the ESPNow
        // frames - handy when filling in the ESPNow receiver MAC list on another node.
        String mac = WiFi.macAddress();
        mac.toLowerCase();
        content += ' ';
        content += mac;
      }
      content += "]</h4>";
    } else {
      // Reached only when no interface is up; keep this interface-agnostic
      content += "<h4>Network state: Disconnected</h4>";
    }

    if (ap_active) {
      content += "<h4>Access Point active: " + WiFi.softAPIP().toString() + "</h4>";
    }

    // Close the block
    content += "</div>";

    if (inverter || battery || charger || user_selected_shunt_type != ShuntType::None) {
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
        if (battery3) {
          content += " ③";
        } else if (battery2) {
          content += " ②";
        }
        if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
          content += " (LFP)";
        }
        content += "</h4>";
      }

      if (user_selected_shunt_type != ShuntType::None) {
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
      BatteryCardView view;

      if (datalayer.system.info.configured_batteries > 1) {
        /* The whole installation on top, then the packs. Every card keeps all four corners
           rounded so the shapes stay readable, and the gaps are kept thin so the group still
           reads as one block. */
        fill_card_view_aggregate(view);
        render_battery_card(
            content,
            "background-color: " + emulator_status_color() + " padding: 10px; margin-bottom: 4px; border-radius: 50px;",
            view, 0);

        content += "<div style='display: flex; width: 100%; gap: 4px;'>";
        const String pack_style = "flex: 1; background-color: " + system_status_color() +
                                  " padding: 10px; margin-bottom: 10px; border-radius: 50px;";
        fill_card_view(view, datalayer.battery);
        render_battery_card(content, pack_style, view, 1);
        fill_card_view(view, datalayer.battery2);
        render_battery_card(content, pack_style, view, 2);
        if (battery3) {
          fill_card_view(view, datalayer.battery3);
          render_battery_card(content, pack_style, view, 3);
        }
        content += "</div>";
      } else {
        /* Single battery. The pack and the installation are the same thing, so read the
           aggregate: it is what the inverter is given, and it already holds this pack's
           scaled figures. */
        fill_card_view_aggregate(view);
        render_battery_card(content,
                            "background-color: " + emulator_status_color() +
                                " padding: 10px; margin-bottom: 10px; border-radius: 50px;",
                            view, 1);
      }
    }
    // Block for Contactor status and component request status
    // Start a new block with gray background color
    content += "<div style='background-color: #333; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4>System status: ";
    switch (datalayer.system.status.system_status) {
      case ACTIVE:
        content += String("OK");
        break;
      case UPDATING:
        content += String("UPDATING");
        break;
      case FAULT:
        content += String("FAULT ");
        content += "<button onclick='Events()'>Inspect reason</button> ";
        break;
      case INACTIVE:
        content += String("INACTIVE");
        break;
      case STANDBY:
        content += String("STANDBY");
        break;
      default:
        content += String("Unknown");
        break;
    }
    content += "</h4>";

    if (emulator_pause_status == NORMAL) {
      content += "<h4>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
    } else {
      content += "<h4 style='color: red;'>Power status: " + String(get_emulator_pause_status().c_str()) + " </h4>";
    }

    content += "<h4>Emulator allows contactor closing: ";
    if (datalayer.system.status.system_status == FAULT) {
      content += "<span style='color: red;'>✗</span>";
    } else {
      content += "<span>✓</span>";
    }
    content += "<br>Inverter allows contactor closing: ";
    if (datalayer.system.status.inverter_allows_contactor_closing == true) {
      content += "<span>✓</span></h4>";
    } else {
      content += "<span style='color: red;'>✗</span></h4>";
    }
    if (battery2) {
      content += "<h4>2ⁿᵈ battery allowed to join: ";
      if (datalayer.system.status.battery2_allowed_contactor_closing == true) {
        content += "<span>✓</span>";
      } else {
        content += "<span style='color: red;'>✗<br>(voltage mismatch)</span>";
      }
      content += "</h4>";
    }
    if (battery3) {
      content += "<h4>3ʳᵈ battery allowed to join: ";
      if (datalayer.system.status.battery3_allowed_contactor_closing == true) {
        content += "<span>✓</span>";
      } else {
        content += "<span style='color: red;'>✗<br>(voltage mismatch)</span>";
      }
      content += "</h4>";
    }

    if (!contactor_control_enabled) {
      content += "<div class=\"tooltip\">";
      content += "<h4>Contactors not fully controlled via emulator <span style=\"color:orange\">ⓘ</span></h4>";
      content +=
          "<span class=\"tooltiptext\">This means you are either running CAN controlled contactors OR manually "
          "powering the contactors. Battery-Emulator will have limited amount of control over the contactors!</span>";
      content += "</div>";
    } else {  //contactor_control_enabled TRUE
      content += "<div class=\"tooltip\"><h4>Contactors control — state: ";
      if (datalayer.system.status.contactors_engaged == 0) {
        content += "<span style='color: red;'>OFF<br>(DISCONNECTED)</span>";
      } else if (datalayer.system.status.contactors_engaged == 1) {
        if (pwm_contactor_control) {
          content += "<span style='color: green;'>Economized</span>";
        } else {
          content += "<span style='color: green;'>ON</span>";
        }
      } else if (datalayer.system.status.contactors_engaged == 2) {
        content += "<span style='color: red;'>OFF<br>(FAULT)</span>";
        content += "<span class=\"tooltip-icon\"> [!]</span>";
        content +=
            "<span class=\"tooltiptext\">Emulator spent too much time in critical FAULT event. Investigate event "
            "causing this via Events page. Reboot required to resume operation!</span>";
      } else if (datalayer.system.status.contactors_engaged == 3) {
        content += "<span style='color: orange;'>PRECHARGE</span>";
      }
      content += "</h4></div>";
      if (contactor_control_enabled_double_battery && battery2) {
        content += "<h4>Contactor for 2ⁿᵈ — state: ";
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
      if (contactor_control_enabled_triple_battery && battery3) {
        content += "<h4>Contactor for 3ʳᵈ — state: ";
        if (pwm_contactor_control) {
          if (datalayer.system.status.contactors_battery3_engaged) {
            content += "<span style='color: green;'>Economized</span>";
          } else {
            content += "<span style='color: red;'>OFF</span>";
          }
        } else if (
            esp32hal->TRIPLE_BATTERY_CONTACTORS_PIN() !=
            GPIO_NUM_NC) {  // No PWM_CONTACTOR_CONTROL , we can read the pin and see feedback. Helpful if channel overloaded
          if (digitalRead(esp32hal->TRIPLE_BATTERY_CONTACTORS_PIN()) == HIGH) {
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
        content += "<span>✓</span>";
      } else {
        content += "<span style='color: red;'>✗</span>";
      }
      content += "</h4>";

      content += "<h4>Charger Aux12v Enabled: ";
      if (datalayer.charger.charger_aux12V_enabled) {
        content += "<span>✓</span>";
      } else {
        content += "<span style='color: red;'>✗</span>";
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
  }
  return content.good();
}

/* The fixed part of the main page, around the live part. The styles and the header come before
   it; the buttons and the scripts after it. The buttons whose label follows the emulator's state
   come in pairs, both hidden until the script shows the right one from the live part's state. */
static const char main_page_head[] =
    R"html(<style>body{background-color:black;color:white}
button{background-color:#505E67;color:white;border:none;padding:10px 20px;margin-bottom:20px;cursor:pointer;border-radius:10px}
button:hover{background-color:#3A4A52}
h2{font-size:1.2em;margin:.3em 0 .5em}
h4{margin:.6em 0;line-height:1.2}
.tooltip .tooltiptext{visibility:hidden;width:200px;background-color:#3A4A52;color:white;text-align:center;border-radius:6px;padding:8px;position:absolute;z-index:1;margin-left:-100px;opacity:0;transition:opacity .3s;font-size:.9em;font-weight:normal;line-height:1.4}
.tooltip:hover .tooltiptext{visibility:visible;opacity:1}
.tooltip-icon{color:#505E67;cursor:help}
#live{cursor:pointer;transition:opacity .2s;-webkit-tap-highlight-color:transparent}
</style>
<h2><a href='https://dalathegreat.github.io/Battery-Emulator-Wiki/' target='_blank' rel='noopener' style='color:inherit'>Battery Emulator</a></h2>
<div id='bxUpd' style='text-align:center'></div><h4 id='note' hidden style='color:#F5CC00'></h4><div id='live'>)html";

static const char main_page_buttons[] =
    R"html(</div><button id='P0' hidden onclick="if(confirm('Are you sure you want to pause charging and discharging? This will set the maximum charge and discharge values to zero, preventing any further power flow.')) { PauseBattery(true); }">Pause charge/discharge</button><button id='P1' hidden onclick='PauseBattery(false)'>Resume charge/discharge</button> <button onclick="location='/update'">Perform OTA update</button> <button onclick="location='/settings'">Change Settings</button> <button onclick="location='/advanced'">More Battery/Cell Info</button> <button onclick="location='/canreplay'">CAN tools</button> )html";
static const char main_page_log_button[] = "<button onclick=\"location='/log'\">Log</button> ";
static const char main_page_more_buttons[] =
    "<button onclick='Events()'>Events</button> <button onclick='askReboot()'>Reboot Emulator</button> ";
static const char main_page_logout_button[] = "<button onclick=\"location='/logout'\">Logout</button>";

#ifdef GIT_TAG
// In-UI update notification (browser-side, 6h cached) - issue #1660. Release builds only.
#define MAIN_PAGE_UPDATE_CHECK \
  R"js(<script>(function(){var cur=')js" GIT_TAG R"js(';var el=document.getElementById('bxUpd');if(!el)return;
function p(v){return v.replace(/^v/,'').split('.').map(function(x){return parseInt(x,10)||0;});}
function nw(a,b){for(var i=0;i<Math.max(a.length,b.length);i++){var x=a[i]||0,y=b[i]||0;if(x>y)return true;if(x<y)return false;}return false;}
function show(t,u){if(nw(p(t),p(cur)))el.innerHTML="<a href='"+u+"' target='_blank' style='display:inline-block;margin:2px 0 10px;padding:8px 16px;background:#505E67;border:1px solid #4caf50;border-radius:10px;color:#fff;font-weight:bold;text-decoration:none'>&#128276; New version "+t+" available &rarr;</a>";}
var c=null;try{c=JSON.parse(localStorage.getItem('beUpd'));}catch(e){}var now=Date.now();
if(c&&c.t&&(now-c.t)<21600000){show(c.tag,c.url);return;}
fetch('https://api.github.com/repos/dalathegreat/Battery-Emulator/releases/latest').then(function(r){return r.json();}).then(function(d){if(!d||!d.tag_name)return;try{localStorage.setItem('beUpd',JSON.stringify({t:now,tag:d.tag_name,url:d.html_url}));}catch(e){}show(d.tag_name,d.html_url);}).catch(function(){});
})();</script>)js"
#else
#define MAIN_PAGE_UPDATE_CHECK ""
#endif

/* The page script polls /live and swaps it in, instead of reloading the whole page:
   - A hidden tab does not poll at all; it catches up the moment it is shown again.
   - A failed poll keeps what is on screen and retries in 5 s. From the second failure in a row
     the data is dimmed and the reason shown, so stale numbers never pass for fresh ones.
   - A poll answered by a different boot means the emulator restarted (reboot, OTA update), so
     the page reloads to get the page of the firmware now running. Rebooting from this page just
     keeps polling until that happens, instead of navigating away after a fixed delay.
   - Pause and the contactor buttons refresh the live part right away instead of reloading.
   - A tap or click on the live part (or on the note above it) refreshes it at once, dimming it
     briefly as acknowledgement. Not when it lands on a link, a button or a tooltip of its own,
     or ends a text selection, and not within 1 s of the last request, so tapping away cannot
     hammer the emulator. A tap that fails is reported at once, without waiting for a second.
   Plain ES5, so a browser too old for fetch() still parses it and falls back to reloading. The
   page does without COMMON_JAVASCRIPT: its reboot() would navigate away after a fixed delay. */
static const char main_page_end[] =
    R"html(<br/><button id='E0' hidden style="background:red;color:white;cursor:pointer;" onclick="if(confirm('This action will attempt to open contactors on the battery. Are you sure?')) { estop(true); }">Open Contactors</button><button id='E1' hidden style="background:green;color:white;cursor:pointer;" onclick="if(confirm('This action will attempt to close contactors and enable power transfer. Are you sure?')) { estop(false); }">Close Contactors</button><br/>
<script>
function Events(){location='/events'}
function g(i){return document.getElementById(i)}
function send(u,f){var x=new XMLHttpRequest();x.onload=function(){if(f)f();tick()};x.open('GET',u,true);x.send()}
function PauseBattery(p){send('/pause?value='+p)}
function estop(s){send('/equipmentStop?value='+s)}
function askReboot(){if(confirm('Are you sure you want to reboot the emulator? NOTE: If emulator is handling contactors, they will open during reboot!'))send('/reboot',function(){R=1})}
var L=g('live'),N=g('note'),T,B,F=0,R=0,Q=0,M=0;
function state(){var s=g('S');if(!s)return'';var p=s.getAttribute('data-p')=='1',e=s.getAttribute('data-e')=='1';
g('P0').hidden=p;g('P1').hidden=!p;g('E0').hidden=e;g('E1').hidden=!e;return s.getAttribute('data-b')}
function later(ms){clearTimeout(T);T=setTimeout(tick,ms)}
function tick(){clearTimeout(T);if(document.hidden)return;if(Q){Q=2;return}
if(!window.fetch)return location.reload();
Q=1;M=+new Date;var a=window.AbortController?new AbortController():0,w=a&&setTimeout(function(){a.abort()},8000);
function end(ms){clearTimeout(w);var q=Q;Q=0;later(q>1?0:ms>0?ms:15000)}
fetch('/live',{cache:'no-store',signal:a?a.signal:undefined})
.then(function(r){return r.text().then(function(h){if(!r.ok)throw h;return h})})
.then(function(h){L.innerHTML=h;var b=state();if(B&&b&&b!=B)location.reload();
B=b||B;F=0;L.style.opacity='';N.hidden=true;return R?3000:15000},
function(m){if(++F>1){L.style.opacity=.5;N.hidden=false;
N.textContent=(R?'Waiting for the emulator to restart.':typeof m=='string'&&m||'Emulator not reachable.')+' Retrying...'}
else L.style.opacity='';return 5000}).then(end,end)}
L.onclick=N.onclick=function(e){var t=e.target;if(t.closest&&t.closest('a,button,[onclick],.tooltip')||String(getSelection())||new Date-M<1000)return;
F=F||1;L.style.opacity=.7;tick()};
document.addEventListener('visibilitychange',tick);
B=state();later(B?15000:5000);
</script>)html" MAIN_PAGE_UPDATE_CHECK INDEX_HTML_FOOTER;

// Shown in place of the live part when there is not enough memory to render it. The page retries
// on its own, and fills in as soon as a render succeeds.
static const char main_page_low_memory[] =
    "<h4 style='color: #F5CC00;'>Not enough free memory to render this page right now. Retrying in a few "
    "seconds.</h4>";

namespace {
// Only one /live answer is out at a time; see send_main_page().
std::atomic<bool> live_busy{false};

/* Sends the page as a few pieces back to back: the fixed ones straight from flash, the live part
   from the buffer it was rendered into. Unlike the template engine, which copied everything
   after the first TCP segment into a cache of its own, nothing is held twice. */
class MainPageResponse : public AsyncAbstractResponse {
 public:
  explicit MainPageResponse(bool holds_live_slot) : AsyncAbstractResponse(nullptr), holds_live_slot(holds_live_slot) {
    _code = 200;
    _contentType = "text/html";
  }
  ~MainPageResponse() override {
    if (holds_live_slot) {
      live_busy.store(false);
    }
  }

  // The text must stay valid and unchanged until the response is gone.
  void add(const char* text, size_t length) {
    if (length > 0 && count < MAX_PARTS) {
      parts[count] = text;
      lengths[count] = length;
      count++;
      _contentLength += length;
    }
  }

  bool _sourceValid() const override { return true; }

  size_t _fillBuffer(uint8_t* data, size_t len) override {
    size_t written = 0;
    while (written < len && part < count) {
      const size_t n = std::min(len - written, lengths[part] - offset);
      memcpy(data + written, parts[part] + offset, n);
      written += n;
      offset += n;
      if (offset == lengths[part]) {
        part++;
        offset = 0;
      }
    }
    return written;
  }

  String live;

 private:
  static constexpr uint8_t MAX_PARTS = 8;
  const char* parts[MAX_PARTS] = {};
  size_t lengths[MAX_PARTS] = {};
  uint8_t count = 0;
  uint8_t part = 0;
  size_t offset = 0;
  bool holds_live_slot;
};
}  // namespace

template <size_t N>
static void add_part(MainPageResponse* page, const char (&text)[N]) {
  page->add(text, N - 1);
}

static void send_unavailable(AsyncWebServerRequest* request, const char* reason) {
  AsyncWebServerResponse* response = request->beginResponse(503, "text/plain", reason);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

static void send_main_page(AsyncWebServerRequest* request, bool live_only) {
  /* A poll that finds another page's answer still going out is turned away: it keeps what it
     shows and asks again in a few seconds. That bounds the heap the polls can take however many
     pages are open. Opening the page is never turned away. */
  if (live_only && live_busy.exchange(true)) {
    send_unavailable(request, "Emulator busy.");
    return;
  }
  MainPageResponse* page = new (std::nothrow) MainPageResponse(live_only);
  if (!page) {
    if (live_only) {
      live_busy.store(false);
    }
    send_unavailable(request, "Not enough free memory to update right now.");
    return;
  }

  CheckedHtml live;
  const bool rendered = render_live(live);
  if (rendered) {
    page->live = live.take();
  }

  if (live_only) {
    if (!rendered) {
      delete page;  // frees the slot
      send_unavailable(request, "Not enough free memory to update right now.");
      return;
    }
    page->add(page->live.c_str(), page->live.length());
    page->addHeader("Cache-Control", "no-store");
  } else {
    page->add(index_html_header, strlen(index_html_header));
    add_part(page, main_page_head);
    if (rendered) {
      page->add(page->live.c_str(), page->live.length());
    } else {
      add_part(page, main_page_low_memory);
    }
    add_part(page, main_page_buttons);
    if (datalayer.system.info.web_logging_active
#ifdef SDCARD
        || datalayer.system.info.SD_logging_active
#endif
    ) {
      add_part(page, main_page_log_button);
    }
    add_part(page, main_page_more_buttons);
    if (webserver_auth) {
      add_part(page, main_page_logout_button);
    }
    add_part(page, main_page_end);
  }
  request->send(page);
}

void onOTAStart() {
  //try to Pause the battery
  setBatteryPause(true, false, EquipmentStop::UNCHANGED, false);

  // Log when OTA has started
  set_event(EVENT_OTA_UPDATE, 0);

  // If already set, make a new attempt
  clear_event(EVENT_OTA_UPDATE_TIMEOUT);
  ota_active = true;

  ota_timeout_timer.reset();
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (ota_progress_timer.elapsed()) {
    if (final > 0) {
      constexpr float BYTES_PER_KB = 1024.0f;
      float percent = (float)current * 100.0f / (float) final;
      logging.printf("OTA progress: %.1f%% (%.1f / %.1f KB)\n", percent, current / BYTES_PER_KB, final / BYTES_PER_KB);
    }
    // Reset the "watchdog"
    ota_timeout_timer.reset();
  }
}

void onOTAEnd(bool success) {

  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);

  // Log when OTA has finished
  if (success) {
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.println("OTA update finished successfully!");
    hold_pins_across_reset();
    graceful_restart();
  } else {
    LOG_SET_NEXT_SEVERITY(3);  // err
    logging.println("OTA update failed.");
    // Unpause battery (preserving equipment stop if set)
    setBatteryPause(false, false, EquipmentStop::UNCHANGED, false);
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

    if (convertedValue >= 1000.0f || convertedValue <= -1000.0f) {
      result += String(convertedValue / 1000.0f, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }

  result += unit;
  return result;
}
