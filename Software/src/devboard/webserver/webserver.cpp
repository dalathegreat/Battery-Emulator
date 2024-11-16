#include "webserver.h"
#include <Preferences.h>
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Measure OTA progress
unsigned long ota_progress_millis = 0;

#include "advanced_battery_html.h"
#include "cellmonitor_html.h"
#include "events_html.h"
#include "index_html.cpp"
#include "settings_html.h"

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

const char get_firmware_info_html[] = R"rawliteral(%X%)rawliteral";

void init_webserver() {

  String content = index_html;

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(401); });

  // Route for firmware info from ota update page
  server.on("/GetFirmwareInfo", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });

  // Route for going to settings web page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, settings_processor);
  });

  // Route for going to advanced battery info web page
  server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, advanced_battery_processor);
  });

  // Route for going to cellmonitor web page
  server.on("/cellmonitor", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, cellmonitor_processor);
  });

  // Route for going to event log web page
  server.on("/events", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, events_processor);
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

  // Route for editing SSID
  server.on("/updateSSID", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      if (value.length() <= 63) {  // Check if SSID is within the allowable length
        ssid = value.c_str();
        storeSettings();
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
        storeSettings();
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
      storeSettings();
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
      storeSettings();
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
      storeSettings();
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
        setBatteryPause(true, false, true);
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
      storeSettings();
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
      storeSettings();
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
      storeSettings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for resetting SOH on Nissan LEAF batteries
  server.on("/resetSOH", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    datalayer_extended.nissanleaf.UserRequestSOHreset = true;
    request->send(200, "text/plain", "Updated successfully");
  });

#ifdef TEST_FAKE_BATTERY
  // Route for editing FakeBatteryVoltage
  server.on("/updateFakeBatteryVoltage", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "Bad Request");
    }

    String value = request->getParam("value")->value();
    float val = value.toFloat();

    datalayer.battery.status.voltage_dV = val * 10;

    request->send(200, "text/plain", "Updated successfully");
  });
#endif  // TEST_FAKE_BATTERY

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
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

    if (!(val * charger_setpoint_HV_IDC <= CHARGER_MAX_POWER)) {
      request->send(400, "text/plain", "Bad Request");
    }

    charger_setpoint_HV_VDC = val;

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

    if (!(val * charger_setpoint_HV_VDC <= CHARGER_MAX_POWER)) {
      request->send(400, "text/plain", "Bad Request");
    }

    charger_setpoint_HV_IDC = value.toFloat();

    request->send(200, "text/plain", "Updated successfully");
  });

  // Route for editing ChargerEndA
  server.on("/updateChargeEndA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (WEBSERVER_AUTH_REQUIRED && !request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      charger_setpoint_HV_IDC_END = value.toFloat();
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
      charger_HV_enabled = (bool)value.toInt();
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
      charger_aux12V_enabled = (bool)value.toInt();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });
#endif  // defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER

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
#endif  // HW_STARK

    doc["firmware"] = String(version_number);
    serializeJson(doc, content);
    return content;
  }
  return String();
}

String processor(const String& var) {
  if (var == "X") {
    String content = "";
    content += "<h2>" + String(ssidAP) + "</h2>";  // ssidAP name is used as header name
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Show version number
    content += "<h4>Software: " + String(version_number) + "</h4>";
// Show hardware used:
#ifdef HW_LILYGO
    content += "<h4>Hardware: LilyGo T-CAN485</h4>";
#endif  // HW_LILYGO
#ifdef HW_STARK
    content += "<h4>Hardware: Stark CMR Module</h4>";
#endif  // HW_STARK
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
    content +=
        "<h4>loop() task max load last 10 s: " + String(datalayer.system.status.loop_task_10s_max_us) + " us</h4>";
    content += "<h4>Max load @ worst case execution of core task:</h4>";
    content += "<h4>10ms function timing: " + String(datalayer.system.status.time_snap_10ms_us) + " us</h4>";
    content += "<h4>Values function timing: " + String(datalayer.system.status.time_snap_values_us) + " us</h4>";
    content += "<h4>CAN/serial RX function timing: " + String(datalayer.system.status.time_snap_comm_us) + " us</h4>";
    content += "<h4>CAN TX function timing: " + String(datalayer.system.status.time_snap_cantx_us) + " us</h4>";
    content += "<h4>OTA function timing: " + String(datalayer.system.status.time_snap_ota_us) + " us</h4>";
#endif  // FUNCTION_TIME_MEASUREMENT

    wl_status_t status = WiFi.status();
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + String(ssid.c_str()) + "</h4>";
    if (status == WL_CONNECTED) {
      content += "<h4>IP: " + WiFi.localIP().toString() + "</h4>";
      // Get and display the signal strength (RSSI) and channel
      content += "<h4>Signal strength: " + String(WiFi.RSSI()) + " dBm, at channel " + String(WiFi.channel()) + "</h4>";
    } else {
      content += "<h4>Wifi state: " + getConnectResultString(status) + "</h4>";
    }
    // Close the block
    content += "</div>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #333; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";

    // Display which components are used
    content += "<h4 style='color: white;'>Inverter protocol: ";
    content += datalayer.system.info.inverter_protocol;
    content += "</h4>";
    content += "<h4 style='color: white;'>Battery protocol: ";
    content += datalayer.system.info.battery_protocol;
#ifdef DOUBLE_BATTERY
    content += " (Double battery)";
#endif  // DOUBLE_BATTERY
    if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
      content += " (LFP)";
    }
    content += "</h4>";

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
    content += "<h4 style='color: white;'>Charger protocol: ";
#ifdef CHEVYVOLT_CHARGER
    content += "Chevy Volt Gen1 Charger";
#endif  // CHEVYVOLT_CHARGER
#ifdef NISSANLEAF_CHARGER
    content += "Nissan LEAF 2013-2024 PDM charger";
#endif  // NISSANLEAF_CHARGER
    content += "</h4>";
#endif  // defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER

    // Close the block
    content += "</div>";

#ifdef DOUBLE_BATTERY
    // Start a new block with a specific background color. Color changes depending on BMS status
    content += "<div style='display: flex; width: 100%;'>";
    content += "<div style='flex: 1; background-color: ";
#else
    // Start a new block with a specific background color. Color changes depending on system status
    content += "<div style='background-color: ";
#endif  // DOUBLE_BATTERY

    switch (led_get_color()) {
      case led_color::GREEN:
        content += "#2D3F2F;";
        break;
      case led_color::YELLOW:
        content += "#F5CC00;";
        break;
      case led_color::BLUE:
      case led_color::RGB:
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

    content += "<h4 style='color: white;'>Real SOC: " + String(socRealFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Scaled SOC: " + String(socScaledFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) + " V</h4>";
    content += "<h4 style='color: white;'>Current: " + String(currentFloat, 1) + " A</h4>";
    content += formatPowerValue("Power", powerFloat, "", 1);
    content += formatPowerValue("Total capacity", datalayer.battery.info.total_capacity_Wh, "h", 0);
    content += formatPowerValue("Real Remaining capacity", datalayer.battery.status.remaining_capacity_Wh, "h", 1);
    content +=
        formatPowerValue("Scaled Remaining capacity", datalayer.battery.status.reported_remaining_capacity_Wh, "h", 1);

    if (emulator_pause_status == NORMAL) {
      content += formatPowerValue("Max discharge power", datalayer.battery.status.max_discharge_power_W, "", 1);
      content += formatPowerValue("Max charge power", datalayer.battery.status.max_charge_power_W, "", 1);
      content += "<h4 style='color: white;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A</h4>";
      content += "<h4 style='color: white;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A</h4>";
    } else {
      content += formatPowerValue("Max discharge power", datalayer.battery.status.max_discharge_power_W, "", 1, "red");
      content += formatPowerValue("Max charge power", datalayer.battery.status.max_charge_power_W, "", 1, "red");
      content += "<h4 style='color: red;'>Max discharge current: " + String(maxCurrentDischargeFloat, 1) + " A</h4>";
      content += "<h4 style='color: red;'>Max charge current: " + String(maxCurrentChargeFloat, 1) + " A</h4>";
    }

    content += "<h4>Cell max: " + String(datalayer.battery.status.cell_max_voltage_mV) + " mV</h4>";
    content += "<h4>Cell min: " + String(datalayer.battery.status.cell_min_voltage_mV) + " mV</h4>";
    if (cell_delta_mv > datalayer.battery.info.max_cell_voltage_deviation_mV) {
      content += "<h4 style='color: red;'>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    } else {
      content += "<h4>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    }
    content += "<h4>Temperature max: " + String(tempMaxFloat, 1) + " C</h4>";
    content += "<h4>Temperature min: " + String(tempMinFloat, 1) + " C</h4>";
    if (datalayer.battery.status.bms_status == ACTIVE) {
      content += "<h4>System status: OK </h4>";
    } else if (datalayer.battery.status.bms_status == UPDATING) {
      content += "<h4>System status: UPDATING </h4>";
    } else {
      content += "<h4>System status: FAULT </h4>";
    }
    if (datalayer.battery.status.current_dA == 0) {
      content += "<h4>Battery idle</h4>";
    } else if (datalayer.battery.status.current_dA < 0) {
      content += "<h4>Battery discharging!</h4>";
    } else {  // > 0
      content += "<h4>Battery charging!</h4>";
    }

    content += "<h4>Automatic contactor closing allowed:</h4>";
    content += "<h4>Battery: ";
    if (datalayer.system.status.battery_allows_contactor_closing == true) {
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

#ifdef CONTACTOR_CONTROL
    content += "<h4>Contactors controlled by Battery-Emulator: ";
    if (datalayer.system.status.contactor_control_closed) {
      content += "<span style='color: green;'>ON</span>";
    } else {
      content += "<span style='color: red;'>OFF</span>";
    }
    content += "</h4>";

    content += "<h4>Pre Charge: ";
    if (digitalRead(PRECHARGE_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += " Cont. Neg.: ";
    if (digitalRead(NEGATIVE_CONTACTOR_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }

    content += " Cont. Pos.: ";
    if (digitalRead(POSITIVE_CONTACTOR_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += "</h4>";
#endif

    // Close the block
    content += "</div>";

#ifdef DOUBLE_BATTERY
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
    sohFloat = static_cast<float>(datalayer.battery2.status.soh_pptt) / 100.0;  // Convert to float and divide by 100
    voltageFloat =
        static_cast<float>(datalayer.battery2.status.voltage_dV) / 10.0;  // Convert to float and divide by 10
    currentFloat =
        static_cast<float>(datalayer.battery2.status.current_dA) / 10.0;        // Convert to float and divide by 10
    powerFloat = static_cast<float>(datalayer.battery2.status.active_power_W);  // Convert to float
    tempMaxFloat = static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0;  // Convert to float
    tempMinFloat = static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0;  // Convert to float
    cell_delta_mv = datalayer.battery2.status.cell_max_voltage_mV - datalayer.battery2.status.cell_min_voltage_mV;

    content += "<h4 style='color: white;'>Real SOC: " + String(socRealFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Scaled SOC: " + String(socScaledFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) + " V</h4>";
    content += "<h4 style='color: white;'>Current: " + String(currentFloat, 1) + " A</h4>";
    content += formatPowerValue("Power", powerFloat, "", 1);
    content += formatPowerValue("Total capacity", datalayer.battery2.info.total_capacity_Wh, "h", 0);
    content += formatPowerValue("Real Remaining capacity", datalayer.battery2.status.remaining_capacity_Wh, "h", 1);
    content +=
        formatPowerValue("Scaled Remaining capacity", datalayer.battery2.status.reported_remaining_capacity_Wh, "h", 1);
    content += formatPowerValue("Max discharge power", datalayer.battery2.status.max_discharge_power_W, "", 1);
    content += formatPowerValue("Max charge power", datalayer.battery2.status.max_charge_power_W, "", 1);
    content += "<h4>Cell max: " + String(datalayer.battery2.status.cell_max_voltage_mV) + " mV</h4>";
    content += "<h4>Cell min: " + String(datalayer.battery2.status.cell_min_voltage_mV) + " mV</h4>";
    if (cell_delta_mv > datalayer.battery2.info.max_cell_voltage_deviation_mV) {
      content += "<h4 style='color: red;'>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    } else {
      content += "<h4>Cell delta: " + String(cell_delta_mv) + " mV</h4>";
    }
    content += "<h4>Temperature max: " + String(tempMaxFloat, 1) + " C</h4>";
    content += "<h4>Temperature min: " + String(tempMinFloat, 1) + " C</h4>";
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
    if (datalayer.system.status.battery2_allows_contactor_closing == true) {
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

#ifdef CONTACTOR_CONTROL
    content += "<h4>Contactors controlled by Battery-Emulator: ";
    if (datalayer.system.status.contactor_control_closed) {
      content += "<span style='color: green;'>ON</span>";
    } else {
      content += "<span style='color: red;'>OFF</span>";
    }
    content += "</h4>";

    content += "<h4>Pre Charge: ";
    if (digitalRead(PRECHARGE_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += " Cont. Neg.: ";
    if (digitalRead(NEGATIVE_CONTACTOR_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }

    content += " Cont. Pos.: ";
    if (digitalRead(POSITIVE_CONTACTOR_PIN) == HIGH) {
      content += "<span style='color: green;'>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += "</h4>";
#endif

    content += "</div>";
    content += "</div>";
#endif  // DOUBLE_BATTERY

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
    // Start a new block with orange background color
    content += "<div style='background-color: #FF6E00; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4>Charger HV Enabled: ";
    if (charger_HV_enabled) {
      content += "<span>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += "</h4>";

    content += "<h4>Charger Aux12v Enabled: ";
    if (charger_aux12V_enabled) {
      content += "<span>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += "</h4>";
#ifdef CHEVYVOLT_CHARGER
    float chgPwrDC = static_cast<float>(charger_stat_HVcur * charger_stat_HVvol);
    float chgPwrAC = static_cast<float>(charger_stat_ACcur * charger_stat_ACvol);
    float chgEff = chgPwrDC / chgPwrAC * 100;
    float ACcur = charger_stat_ACcur;
    float ACvol = charger_stat_ACvol;
    float HVvol = charger_stat_HVvol;
    float HVcur = charger_stat_HVcur;
    float LVvol = charger_stat_LVvol;
    float LVcur = charger_stat_LVcur;

    content += formatPowerValue("Charger Output Power", chgPwrDC, "", 1);
    content += "<h4 style='color: white;'>Charger Efficiency: " + String(chgEff) + "%</h4>";
    content += "<h4 style='color: white;'>Charger HVDC Output V: " + String(HVvol, 2) + " V</h4>";
    content += "<h4 style='color: white;'>Charger HVDC Output I: " + String(HVcur, 2) + " A</h4>";
    content += "<h4 style='color: white;'>Charger LVDC Output I: " + String(LVcur, 2) + "</h4>";
    content += "<h4 style='color: white;'>Charger LVDC Output V: " + String(LVvol, 2) + "</h4>";
    content += "<h4 style='color: white;'>Charger AC Input V: " + String(ACvol, 2) + " VAC</h4>";
    content += "<h4 style='color: white;'>Charger AC Input I: " + String(ACcur, 2) + " A</h4>";
#endif  // CHEVYVOLT_CHARGER
#ifdef NISSANLEAF_CHARGER
    float chgPwrDC = static_cast<float>(charger_stat_HVcur * 100);
    charger_stat_HVcur = chgPwrDC / (datalayer.battery.status.voltage_dV / 10);  // P/U=I
    charger_stat_HVvol = static_cast<float>(datalayer.battery.status.voltage_dV / 10);
    float ACvol = charger_stat_ACvol;
    float HVvol = charger_stat_HVvol;
    float HVcur = charger_stat_HVcur;

    content += formatPowerValue("Charger Output Power", chgPwrDC, "", 1);
    content += "<h4 style='color: white;'>Charger HVDC Output V: " + String(HVvol, 2) + " V</h4>";
    content += "<h4 style='color: white;'>Charger HVDC Output I: " + String(HVcur, 2) + " A</h4>";
    content += "<h4 style='color: white;'>Charger AC Input V: " + String(ACvol, 2) + " VAC</h4>";
#endif  // NISSANLEAF_CHARGER
    // Close the block
    content += "</div>";
#endif  // defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER

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
    content += "<button onclick='Cellmon()'>Cellmonitor</button> ";
    content += "<button onclick='Events()'>Events</button> ";
    content += "<button onclick='askReboot()'>Reboot Emulator</button>";
    if (WEBSERVER_AUTH_REQUIRED)
      content += "<button onclick='logout()'>Logout</button>";
    if (!datalayer.system.settings.equipment_stop_active)
      content +=
          "<br/><br/><button style=\"background:red;color:white;cursor:pointer;\""
          " onclick=\""
          "if(confirm('This action will open contactors on the battery and stop all CAN communications. Are you "
          "sure?')) { estop(true); }\""
          ">Open Contactors</button><br/>";
    else
      content +=
          "<br/><br/><button style=\"background:green;color:white;cursor:pointer;\""
          "20px;font-size:16px;font-weight:bold;cursor:pointer;border-radius:5px; margin:10px;"
          " onclick=\""
          "if(confirm('This action will restore the battery state. Are you sure?')) { estop(false); }\""
          ">Close Contactors</button><br/>";
    content += "<script>";
    content += "function OTA() { window.location.href = '/update'; }";
    content += "function Cellmon() { window.location.href = '/cellmonitor'; }";
    content += "function Settings() { window.location.href = '/settings'; }";
    content += "function Advanced() { window.location.href = '/advanced'; }";
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
#ifdef DEBUG_VIA_USB
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
#endif  // DEBUG_VIA_USB
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
#ifdef DEBUG_VIA_USB
    Serial.println("OTA update finished successfully!");
#endif  // DEBUG_VIA_USB
  } else {
#ifdef DEBUG_VIA_USB
    Serial.println("There was an error during OTA update!");
#endif  // DEBUG_VIA_USB
    //try to Resume the battery pause and CAN communication
    setBatteryPause(false, false);
  }
}

template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(String label, T value, String unit, int precision, String color) {
  String result = "<h4 style='color: " + color + ";'>" + label + ": ";

  if (std::is_same<T, float>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) {
    float convertedValue = static_cast<float>(value);

    if (convertedValue >= 1000.0 || convertedValue <= -1000.0) {
      result += String(convertedValue / 1000.0, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }

  result += unit + "</h4>";
  return result;
}
