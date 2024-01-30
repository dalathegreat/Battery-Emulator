#include "webserver.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Measure OTA progress
unsigned long ota_progress_millis = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Battery Emulator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" type="image/png" href="favicon.png">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Battery Emulator</h2>
  %PLACEHOLDER%
</script>
</body>
</html>
)rawliteral";

String wifi_state;
bool wifi_connected;

// Wifi connect time declarations and definition
unsigned long wifi_connect_start_time;
unsigned long wifi_connect_current_time;
const long wifi_connect_timeout = 5000;  // Timeout for WiFi connect in milliseconds

void init_webserver() {
  // Configure WiFi
  if (AccessPointEnabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
    init_WiFi_STA(ssid, password);
  } else {
    WiFi.mode(WIFI_STA);  // Only Router connection
    init_WiFi_STA(ssid, password);
  }

  // Route for root / web page
  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, processor); });

  // Route for going to settings web page
  server.on("/settings", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, settings_processor); });

  // Route for editing Wh
  server.on("/updateBatterySize", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      BATTERY_WH_MAX = value.toInt();
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing SOCMax
  server.on("/updateSocMax", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      MAXPERCENTAGE = value.toInt() * 10;
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing SOCMin
  server.on("/updateSocMin", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      MINPERCENTAGE = value.toInt() * 10;
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxChargeA
  server.on("/updateMaxChargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      MAXCHARGEAMP = value.toInt() * 10;
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Route for editing MaxDischargeA
  server.on("/updateMaxDischargeA", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      MAXDISCHARGEAMP = value.toInt() * 10;
      request->send(200, "text/plain", "Updated successfully");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

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
  Serial.print("Creating Access Point: ");
  Serial.println(ssidAP);
  Serial.print("With password: ");
  Serial.println(passwordAP);

  WiFi.softAP(ssidAP, passwordAP);

  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point created.");
  Serial.print("IP address: ");
  Serial.println(IP);
}

void init_WiFi_STA(const char* ssid, const char* password) {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  wifi_connect_start_time = millis();
  wifi_connect_current_time = wifi_connect_start_time;
  while ((wifi_connect_current_time - wifi_connect_start_time) <= wifi_connect_timeout &&
         WiFi.status() != WL_CONNECTED) {  // do this loop for up to 5000ms
    // to break the loop when the connection is not established (wrong ssid or password).
    delay(500);
    Serial.print(".");
    wifi_connect_current_time = millis();
  }
  if (WiFi.status() == WL_CONNECTED) {  // WL_CONNECTED is assigned when connected to a WiFi network
    wifi_connected = true;
    wifi_state = "Connected";
    // Print local IP address and start web server
    Serial.println("");
    Serial.print("Connected to WiFi network: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    wifi_state = "Not connected";
    Serial.print("Not connected to WiFi network: ");
    Serial.println(ssid);
    Serial.println("Please check WiFi network name and password, and if WiFi network is available.");
  }
}

void init_ElegantOTA() {
  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}

String processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Show version number
    content += "<h4>Software version: " + String(versionNumber) + "</h4>";

    // Display LED color
    content += "<h4>LED color: ";
    switch (LEDcolor) {
      case GREEN:
        content += "GREEN</h4>";
        break;
      case YELLOW:
        content += "YELLOW</h4>";
        break;
      case BLUE:
        content += "BLUE</h4>";
        break;
      case RED:
        content += "RED</h4>";
        break;
      case TEST_ALL_COLORS:
        content += "RGB Testing loop</h4>";
        break;
      default:
        break;
    }
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + String(ssid) + "</h4>";
    content += "<h4>Wifi status: " + wifi_state + "</h4>";
    if (wifi_connected == true) {
      content += "<h4>IP: " + WiFi.localIP().toString() + "</h4>";
      // Get and display the signal strength (RSSI)
      content += "<h4>Signal Strength: " + String(WiFi.RSSI()) + " dBm</h4>";
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
#ifdef CHADEMO_BATTERY
    content += "Chademo V2X mode";
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
    content += "I-Miev / C-Zero / Ion Triplet";
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
    content += "Kia/Hyundai 64kWh";
#endif
#ifdef NISSAN_LEAF_BATTERY
    content += "Nissan LEAF";
#endif
#ifdef RENAULT_KANGOO_BATTERY
    content += "Renault Kangoo";
#endif
#ifdef RENAULT_ZOE_BATTERY
    content += "Renault Zoe";
#endif
#ifdef SERIAL_LINK_RECEIVER
    content += "Serial link to another LilyGo board";
#endif
#ifdef TESLA_MODEL_3_BATTERY
    content += "Tesla Model S/3/X/Y";
#endif
#ifdef TEST_FAKE_BATTERY
    content += "Fake battery for testing purposes";
#endif
    content += "</h4>";
    // Close the block
    content += "</div>";

    // Start a new block with a specific background color. Color changes depending on BMS status
    switch (LEDcolor) {
      case GREEN:
        content += "<div style='background-color: #2D3F2F; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
      case YELLOW:
        content += "<div style='background-color: #F5CC00; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
      case BLUE:
        content += "<div style='background-color: #2B35AF; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
      case RED:
        content += "<div style='background-color: #A70107; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
      case TEST_ALL_COLORS:  //Blue in test mode
        content += "<div style='background-color: #2B35AF; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
      default:  //Some new color, make background green
        content += "<div style='background-color: #2D3F2F; padding: 10px; margin-bottom: 10px; border-radius: 50px'>";
        break;
    }

    // Display battery statistics within this block
    float socFloat = static_cast<float>(SOC) / 100.0;                 // Convert to float and divide by 100
    float sohFloat = static_cast<float>(StateOfHealth) / 100.0;       // Convert to float and divide by 100
    float voltageFloat = static_cast<float>(battery_voltage) / 10.0;  // Convert to float and divide by 10
    float currentFloat = 0;
    if (battery_current > 32767) {  //Handle negative values on this unsigned value
      currentFloat = static_cast<float>(-(65535 - battery_current)) / 10.0;  // Convert to float and divide by 10
    } else {
      currentFloat = static_cast<float>(battery_current) / 10.0;  // Convert to float and divide by 10
    }
    float powerFloat = 0;
    if (stat_batt_power > 32767) {  //Handle negative values on this unsigned value
      powerFloat = static_cast<float>(-(65535 - stat_batt_power));
    } else {
      powerFloat = static_cast<float>(stat_batt_power);
    }
    float tempMaxFloat = 0;
    float tempMinFloat = 0;
    if (temperature_max > 32767) {  //Handle negative values on this unsigned value
      tempMaxFloat = static_cast<float>(-(65536 - temperature_max)) / 10.0;  // Convert to float and divide by 10
    } else {
      tempMaxFloat = static_cast<float>(temperature_max) / 10.0;  // Convert to float and divide by 10
    }
    if (temperature_min > 32767) {  //Handle negative values on this unsigned value
      tempMinFloat = static_cast<float>(-(65536 - temperature_min)) / 10.0;  // Convert to float and divide by 10
    } else {
      tempMinFloat = static_cast<float>(temperature_min) / 10.0;  // Convert to float and divide by 10
    }
    content += "<h4 style='color: white;'>SOC: " + String(socFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) + " V</h4>";
    content += "<h4 style='color: white;'>Current: " + String(currentFloat, 1) + " A</h4>";
    content += formatPowerValue("Power", powerFloat, "", 1);
    content += formatPowerValue("Total capacity", capacity_Wh, "h", 0);
    content += formatPowerValue("Remaining capacity", remaining_capacity_Wh, "h", 1);
    content += formatPowerValue("Max discharge power", max_target_discharge_power, "", 1);
    content += formatPowerValue("Max charge power", max_target_charge_power, "", 1);
    content += "<h4>Cell max: " + String(cell_max_voltage) + " mV</h4>";
    content += "<h4>Cell min: " + String(cell_min_voltage) + " mV</h4>";
    content += "<h4>Temperature max: " + String(tempMaxFloat, 1) + " C</h4>";
    content += "<h4>Temperature min: " + String(tempMinFloat, 1) + " C</h4>";
    if (bms_status == 3) {
      content += "<h4>BMS Status: OK </h4>";
    } else {
      content += "<h4>BMS Status: FAULT </h4>";
    }
    if (bms_char_dis_status == 2) {
      content += "<h4>Battery charging!</h4>";
    } else if (bms_char_dis_status == 1) {
      content += "<h4>Battery discharging!</h4>";
    } else {  //0 idle
      content += "<h4>Battery idle</h4>";
    }
    content += "<h4>Automatic contactor closing allowed:</h4>";
    content += "<h4>Battery: ";
    if (batteryAllowsContactorClosing) {
      content += "<span>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }

    content += " Inverter: ";
    if (inverterAllowsContactorClosing) {
      content += "<span>&#10003;</span></h4>";
    } else {
      content += "<span style='color: red;'>&#10005;</span></h4>";
    }

    // Close the block
    content += "</div>";

    content += "<button onclick='goToUpdatePage()'>Perform OTA update</button>";
    content += " ";
    content += "<button onclick='goToSettingsPage()'>Change Battery Settings</button>";
    content += " ";
    content += "<button onclick='promptToReboot()'>Reboot Emulator</button>";
    content += "<script>";
    content += "function goToUpdatePage() { window.location.href = '/update'; }";
    content += "function goToSettingsPage() { window.location.href = '/settings'; }";
    content +=
        "function promptToReboot() { if (window.confirm('Are you sure you want to reboot the emulator? NOTE: If "
        "emulator is handling contactors, they will open during reboot!')) { "
        "rebootServer(); } }";
    content += "function rebootServer() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/reboot', true);";
    content += "  xhr.send();";
    content += "}";
    content += "</script>";

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 10000);";
    content += "</script>";

    return content;
  }
  return String();
}

String settings_processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Show current settings with edit buttons and input fields
    content += "<h4 style='color: white;'>Battery capacity: <span id='BATTERY_WH_MAX'>" + String(BATTERY_WH_MAX) +
               " Wh </span> <button onclick='editWh()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>SOC max percentage: " + String(MAXPERCENTAGE / 10.0, 1) +
               " </span> <button onclick='editSocMax()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>SOC min percentage: " + String(MINPERCENTAGE / 10.0, 1) +
               " </span> <button onclick='editSocMin()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max charge speed: " + String(MAXCHARGEAMP / 10.0, 1) +
               " A </span> <button onclick='editMaxChargeA()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max discharge speed: " + String(MAXDISCHARGEAMP / 10.0, 1) +
               " A </span> <button onclick='editMaxDischargeA()'>Edit</button></h4>";

    content += "<script>";
    content += "function editWh() {";
    content += "var value = prompt('How much energy the battery can store. Enter new Wh value (1-65000):');";
    content += "if (value !== null) {";
    content += "  if (value >= 1 && value <= 65000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateBatterySize?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 1 and 65000.');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editSocMax() {";
    content +=
        "var value = prompt('Inverter will see fully charged (100pct)SOC when this value is reached. Enter new maximum "
        "SOC value that battery will charge to (50.0-100.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 50 && value <= 100) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateSocMax?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 50.0 and 100.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editSocMin() {";
    content +=
        "var value = prompt('Inverter will see completely discharged (0pct)SOC when this value is reached. Enter new "
        "minimum SOC value that battery will discharge to (0-50.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 50) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateSocMin?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 50.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editMaxChargeA() {";
    content +=
        "var value = prompt('BYD CAN specific setting, some inverters needs to be artificially limited. Enter new "
        "maximum charge current in A (0-1000.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateMaxChargeA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editMaxDischargeA() {";
    content +=
        "var value = prompt('BYD CAN specific setting, some inverters needs to be artificially limited. Enter new "
        "maximum discharge current in A (0-1000.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateMaxDischargeA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "</script>";

    // Close the block
    content += "</div>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";
    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  ESP32Can.CANStop();
  bms_status = UPDATING;  //Inform inverter that we are updating
  LEDcolor = BLUE;
}

void onOTAProgress(size_t current, size_t final) {
  bms_status = UPDATING;  //Inform inverter that we are updating
  LEDcolor = BLUE;
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  bms_status = UPDATING;  //Inform inverter that we are updating
  LEDcolor = BLUE;
}

template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(String label, T value, String unit, int precision) {
  String result = "<h4 style='color: white;'>" + label + ": ";

  if (std::is_same<T, float>::value || std::is_same<T, uint16_t>::value) {
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
