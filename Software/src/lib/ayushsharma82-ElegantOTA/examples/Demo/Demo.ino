/*
  -----------------------
  ElegantOTA - Demo Example
  -----------------------

  Skill Level: Beginner

  This example provides with a bare minimal app with ElegantOTA functionality.

  Github: https://github.com/ayushsharma82/ElegantOTA
  WiKi: https://docs.elegantota.pro

  Works with following hardware:
  - ESP8266
  - ESP32
  - RP2040 (with WiFi) (Example: Raspberry Pi Pico W)


  Important note for RP2040 users:
  - RP2040 requires LittleFS partition for the OTA updates to work. Without LittleFS partition, OTA updates will fail.
    Make sure to select Tools > Flash Size > "2MB (Sketch 1MB, FS 1MB)" option.
  - If using bare RP2040, it requires WiFi module like Pico W for ElegantOTA to work.

  -------------------------------

  Upgrade to ElegantOTA Pro: https://elegantota.pro

*/


#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
#elif defined(TARGET_RP2040)
  #include <WiFi.h>
  #include <WebServer.h>
#endif

#include <ElegantOTA.h>

const char* ssid = "";
const char* password = "";

#if defined(ESP8266)
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#elif defined(TARGET_RP2040)
  WebServer server(80);
#endif

unsigned long ota_progress_millis = 0;

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

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  ElegantOTA.loop();
}
