#include "webserver.h"
#include <Preferences.h>
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Measure OTA progress
unsigned long ota_progress_millis = 0;

#include "cellmonitor_html.h"
#include "events_html.h"
#include "index_html.cpp"
#include "settings_html.h"

enum WifiState {
  INIT,          //before connecting first time
  RECONNECTING,  //we've connected before, but lost connection
  CONNECTED      //we are connected
};

WifiState wifi_state = INIT;

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

unsigned const long WIFI_MONITOR_INTERVAL_TIME = 15000;
unsigned const long INIT_WIFI_CONNECT_TIMEOUT = 8000;        // Timeout for initial WiFi connect in milliseconds
unsigned const long DEFAULT_WIFI_RECONNECT_INTERVAL = 1000;  // Default WiFi reconnect interval in ms
unsigned const long MAX_WIFI_RETRY_INTERVAL = 90000;         // Maximum wifi retry interval in ms
unsigned long last_wifi_monitor_time = millis();             //init millis so wifi monitor doesn't run immediately
unsigned long wifi_reconnect_interval = DEFAULT_WIFI_RECONNECT_INTERVAL;
unsigned long last_wifi_attempt_time = millis();  //init millis so wifi monitor doesn't run immediately

void init_webserver() {
  // Configure WiFi
  if (AccessPointEnabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
  } else {
    WiFi.mode(WIFI_STA);  // Only Router connection
  }
  init_WiFi_STA(ssid, password, wifi_channel);

  String content = index_html;

  // Route for root / web page
  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, processor); });

  // Route for going to settings web page
  server.on("/settings", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, settings_processor); });

  // Route for going to cellmonitor web page
  server.on("/cellmonitor", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, cellmonitor_processor);
  });

  // Route for going to event log web page
  server.on("/events", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, events_processor); });

  // Route for editing Wh
  server.on("/updateBatterySize", HTTP_GET, [](AsyncWebServerRequest* request) {
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
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.max_percentage = value.toInt() * 100;
      storeSettings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing SOCMin
  server.on("/updateSocMin", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.settings.min_percentage = value.toInt() * 100;
      storeSettings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxChargeA
  server.on("/updateMaxChargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.info.max_charge_amp_dA = value.toInt() * 10;
      storeSettings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxDischargeA
  server.on("/updateMaxDischargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      datalayer.battery.info.max_discharge_amp_dA = value.toInt() * 10;
      storeSettings();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

#ifdef TEST_FAKE_BATTERY
  // Route for editing FakeBatteryVoltage
  server.on("/updateFakeBatteryVoltage", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "Bad Request");
    }

    String value = request->getParam("value")->value();
    float val = value.toFloat();

    datalayer.battery.status.voltage_dV = val * 10;

    request->send(200, "text/plain", "Updated successfully");
  });
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
  // Route for editing ChargerTargetV
  server.on("/updateChargeSetpointV", HTTP_GET, [](AsyncWebServerRequest* request) {
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
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "Bad Request");
    }

    String value = request->getParam("value")->value();
    float val = value.toFloat();

    if (!(val <= datalayer.battery.info.max_charge_amp_dA && val <= CHARGER_MAX_A)) {
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
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      charger_aux12V_enabled = (bool)value.toInt();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });
#endif

  // Send a GET request to <ESP_IP>/update
  server.on("/debug", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  // Route to handle reboot command
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Rebooting server...");
    //TODO: Should we handle contactors gracefully? Ifdef CONTACTOR_CONTROL then what?
    delay(1000);
    ESP.restart();
  });

  // Initialize ElegantOTA
  init_ElegantOTA();

  // Start server
  server.begin();

#ifdef MQTT
  // Init MQTT
  init_mqtt();
#endif
}

void init_WiFi_AP() {
#ifdef DEBUG_VIA_USB
  Serial.println("Creating Access Point: " + String(ssidAP));
  Serial.println("With password: " + String(passwordAP));
#endif
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
#ifdef DEBUG_VIA_USB
  Serial.println("Access Point created.");
  Serial.print("IP address: ");
  Serial.println(IP);
#endif
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

void wifi_monitor() {
  unsigned long currentMillis = millis();
  if (currentMillis - last_wifi_monitor_time > WIFI_MONITOR_INTERVAL_TIME) {
    last_wifi_monitor_time = currentMillis;
    wl_status_t status = WiFi.status();
    if (status != WL_CONNECTED && status != WL_IDLE_STATUS) {
#ifdef DEBUG_VIA_USB
      Serial.println(getConnectResultString(status));
#endif
      if (wifi_state == INIT) {  //we haven't been connected yet, try the init logic
        init_WiFi_STA(ssid, password, wifi_channel);
      } else {  //we were connected before, try the reconnect logic
        if (currentMillis - last_wifi_attempt_time > wifi_reconnect_interval) {
          last_wifi_attempt_time = currentMillis;
#ifdef DEBUG_VIA_USB
          Serial.println("WiFi not connected, trying to reconnect...");
#endif
          wifi_state = RECONNECTING;
          WiFi.reconnect();
          wifi_reconnect_interval = min(wifi_reconnect_interval * 2, MAX_WIFI_RETRY_INTERVAL);
        }
      }
    } else if (status == WL_CONNECTED && wifi_state != CONNECTED) {
      wifi_state = CONNECTED;
      wifi_reconnect_interval = DEFAULT_WIFI_RECONNECT_INTERVAL;
// Print local IP address and start web server
#ifdef DEBUG_VIA_USB
      Serial.print("Connected to WiFi network: " + String(ssid));
      Serial.print(" IP address: " + WiFi.localIP().toString());
      Serial.print(" Signal Strength: " + String(WiFi.RSSI()) + " dBm");
      Serial.println(" Channel: " + String(WiFi.channel()));
      Serial.println(" Hostname: " + String(WiFi.getHostname()));
#endif
    }
  }

  if (ota_active && ota_timeout_timer.elapsed()) {
    // OTA timeout, try to restore can and clear the update event
    ESP32Can.CANInit();
    clear_event(EVENT_OTA_UPDATE);
    set_event(EVENT_OTA_UPDATE_TIMEOUT, 0);
    ota_active = false;
  }
}

void init_WiFi_STA(const char* ssid, const char* password, const uint8_t wifi_channel) {
// Connect to Wi-Fi network with SSID and password
#ifdef DEBUG_VIA_USB
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif
  WiFi.begin(ssid, password, wifi_channel);
  WiFi.setAutoReconnect(true);  // Enable auto reconnect
  wl_status_t result = static_cast<wl_status_t>(WiFi.waitForConnectResult(INIT_WIFI_CONNECT_TIMEOUT));
  if (result) {
    //TODO: Add event or serial print?
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
#endif
#ifdef HW_STARK
    content += "<h4>Hardware: Stark CMR Module</h4>";
#endif
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
    content += "<h4>5s function timing: " + String(datalayer.system.status.time_snap_5s_us) + " us</h4>";
    content += "<h4>CAN/serial RX function timing: " + String(datalayer.system.status.time_snap_comm_us) + " us</h4>";
    content += "<h4>CAN TX function timing: " + String(datalayer.system.status.time_snap_cantx_us) + " us</h4>";
    content += "<h4>OTA function timing: " + String(datalayer.system.status.time_snap_ota_us) + " us</h4>";
#endif

    wl_status_t status = WiFi.status();
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + String(ssid) + "</h4>";
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
#ifdef BYD_CAN
    content += "BYD Battery-Box Premium HVS over CAN Bus";
#endif
#ifdef BYD_MODBUS
    content += "BYD 11kWh HVM battery over Modbus RTU";
#endif
#ifdef LUNA2000_MODBUS
    content += "Luna2000 battery over Modbus RTU";
#endif
#ifdef PYLON_CAN
    content += "Pylontech battery over CAN bus";
#endif
#ifdef SERIAL_LINK_TRANSMITTER
    content += "Serial link to another LilyGo board";
#endif
#ifdef SMA_CAN
    content += "BYD Battery-Box H 8.9kWh, 7 mod over CAN bus";
#endif
#ifdef SOFAR_CAN
    content += "Sofar Energy Storage Inverter High Voltage BMS General Protocol (Extended Frame) over CAN bus";
#endif
#ifdef SOLAX_CAN
    content += "SolaX Triple Power LFP over CAN bus";
#endif
    content += "</h4>";

    content += "<h4 style='color: white;'>Battery protocol: ";
#ifdef BMW_I3_BATTERY
    content += "BMW i3";
#endif
#ifdef BYD_ATTO_3_BATTERY
    content += "BYD Atto 3";
#endif
#ifdef CHADEMO_BATTERY
    content += "Chademo V2X mode";
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
    content += "I-Miev / C-Zero / Ion Triplet";
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
    content += "Kia/Hyundai 64kWh";
#endif
#ifdef KIA_E_GMP_BATTERY
    content += "Kia/Hyundai EGMP platform";
#endif
#ifdef MG_5_BATTERY
    content += "MG 5";
#endif
#ifdef NISSAN_LEAF_BATTERY
    content += "Nissan LEAF";
#endif
#ifdef RENAULT_KANGOO_BATTERY
    content += "Renault Kangoo";
#endif
#ifdef RENAULT_ZOE_GEN1_BATTERY
    content += "Renault Zoe Gen1 22/40";
#endif
#ifdef RENAULT_ZOE_GEN2_BATTERY
    content += "Renault Zoe Gen2 50";
#endif
#ifdef SERIAL_LINK_RECEIVER
    content += "Serial link to another LilyGo board";
#endif
#ifdef TESLA_MODEL_3_BATTERY
    content += "Tesla Model S/3/X/Y";
#endif
#ifdef VOLVO_SPA_BATTERY
    content += "Volvo / Polestar 78kWh battery";
#endif
#ifdef TEST_FAKE_BATTERY
    content += "Fake battery for testing purposes";
#endif
    content += "</h4>";

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
    content += "<h4 style='color: white;'>Charger protocol: ";
#ifdef CHEVYVOLT_CHARGER
    content += "Chevy Volt Gen1 Charger";
#endif
#ifdef NISSANLEAF_CHARGER
    content += "Nissan LEAF 2013-2024 PDM charger";
#endif
    content += "</h4>";
#endif

    // Close the block
    content += "</div>";

    // Start a new block with a specific background color. Color changes depending on system status
    content += "<div style='background-color: ";
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

    content += "<h4 style='color: white;'>Real SOC: " + String(socRealFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Scaled SOC: " + String(socScaledFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) + " V</h4>";
    content += "<h4 style='color: white;'>Current: " + String(currentFloat, 1) + " A</h4>";
    content += formatPowerValue("Power", powerFloat, "", 1);
    content += formatPowerValue("Total capacity", datalayer.battery.info.total_capacity_Wh, "h", 0);
    content += formatPowerValue("Remaining capacity", datalayer.battery.status.remaining_capacity_Wh, "h", 1);
    content += formatPowerValue("Max discharge power", datalayer.battery.status.max_discharge_power_W, "", 1);
    content += formatPowerValue("Max charge power", datalayer.battery.status.max_charge_power_W, "", 1);
    content += "<h4>Cell max: " + String(datalayer.battery.status.cell_max_voltage_mV) + " mV</h4>";
    content += "<h4>Cell min: " + String(datalayer.battery.status.cell_min_voltage_mV) + " mV</h4>";
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

    // Close the block
    content += "</div>";

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
#endif
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
#endif
    // Close the block
    content += "</div>";
#endif

    content += "<button onclick='OTA()'>Perform OTA update</button>";
    content += " ";
    content += "<button onclick='Settings()'>Change Settings</button>";
    content += " ";
    content += "<button onclick='Cellmon()'>Cellmonitor</button>";
    content += " ";
    content += "<button onclick='Events()'>Events</button>";
    content += " ";
    content += "<button onclick='askReboot()'>Reboot Emulator</button>";
    content += "<script>";
    content += "function OTA() { window.location.href = '/update'; }";
    content += "function Cellmon() { window.location.href = '/cellmonitor'; }";
    content += "function Settings() { window.location.href = '/settings'; }";
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
  // Log when OTA has started
  ESP32Can.CANStop();
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
#endif
    // Reset the "watchdog"
    ota_timeout_timer.reset();
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
#ifdef DEBUG_VIA_USB
    Serial.println("OTA update finished successfully!");
#endif
  } else {
#ifdef DEBUG_VIA_USB
    Serial.println("There was an error during OTA update!");
#endif

    // If we fail without a timeout, try to restore CAN
    ESP32Can.CANInit();
  }
  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);
}

template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(String label, T value, String unit, int precision) {
  String result = "<h4 style='color: white;'>" + label + ": ";

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
