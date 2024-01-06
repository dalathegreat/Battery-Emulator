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
#ifdef ENABLE_AP
  WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
  init_WiFi_AP();
  init_WiFi_STA(ssid, password);
#else
  WiFi.mode(WIFI_STA);  // Only Router connection
  init_WiFi_STA(ssid, password);
#endif

  // Route for root / web page
  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, processor); });

  // Send a GET request to <ESP_IP>/update
  server.on("/debug", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  // Initialize ElegantOTA
  init_ElegantOTA();

  // Start server
  server.begin();
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
      tempMaxFloat = static_cast<float>(-(65535 - temperature_max)) / 10.0;  // Convert to float and divide by 10
    } else {
      tempMaxFloat = static_cast<float>(temperature_max) / 10.0;  // Convert to float and divide by 10
    }
    if (temperature_min > 32767) {  //Handle negative values on this unsigned value
      tempMinFloat = static_cast<float>(-(65535 - temperature_min)) / 10.0;  // Convert to float and divide by 10
    } else {
      tempMinFloat = static_cast<float>(temperature_min) / 10.0;  // Convert to float and divide by 10
    }
    content += "<h4 style='color: white;'>SOC: " + String(socFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>SOH: " + String(sohFloat, 2) + "</h4>";
    content += "<h4 style='color: white;'>Voltage: " + String(voltageFloat, 1) + " V</h4>";
    content += "<h4 style='color: white;'>Current: " + String(currentFloat, 1) + " A</h4>";
    content += "<h4 style='color: white;'>Power: " + String(powerFloat, 0) + " W</h4>";
    content += "<h4>Total capacity: " + String(capacity_Wh) + " Wh</h4>";
    content += "<h4>Remaining capacity: " + String(remaining_capacity_Wh) + " Wh</h4>";
    content += "<h4>Max discharge power: " + String(max_target_discharge_power) + " W</h4>";
    content += "<h4>Max charge power: " + String(max_target_charge_power) + " W</h4>";
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
    // Close the block
    content += "</div>";

    content += "<button onclick='goToUpdatePage()'>Perform OTA update</button>";
    content += "<script>";
    content += "function goToUpdatePage() { window.location.href = '/update'; }";
    content += "</script>";

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 10000);";
    content += "</script>";

    return content;
  }
  return String();
}

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
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
  // <Add your own code here>
}
