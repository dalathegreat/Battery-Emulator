#include "webserver.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Battery Emulator Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
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
  <h2>Battery Emulator Web Server</h2>
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
  WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi Access Point and WiFi STAtion
  init_WiFi_AP();
  init_WiFi_STA(ssid, password);

  // Route for root / web page
  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest* request) { request->send_P(200, "text/html", index_html, processor); });

  // Send a GET request to <ESP_IP>/update
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "OK"); });

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
    wifi_state = "connected";
    // Print local IP address and start web server
    Serial.println("");
    Serial.print("Connected to WiFi network: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    wifi_state = "not connected";
    Serial.print("Not connected to WiFi network: ");
    Serial.println(ssid);
    Serial.println("Please check WiFi network name and password, and if WiFi network is available.");
  }
}

String processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    // Display ssid of network connected to and, if connected to the WiFi, its own IP
    content += "<h4>SSID: " + String(ssid) + "</h4>";
    content += "<h4>status: " + wifi_state + "</h4>";
    if (wifi_connected == true) {
      content += "<h4>IP: " + WiFi.localIP().toString() + "</h4>";
    }
    return content;
  }
  return String();
}
