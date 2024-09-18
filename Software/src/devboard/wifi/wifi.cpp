#include "wifi.h"
#include "../../include.h"
#include "../utils/events.h"

// Configuration Parameters
static const uint16_t WIFI_CHECK_INTERVAL = 5000;                 // 5 seconds
static const uint16_t INIT_WIFI_FULL_RECONNECT_INTERVAL = 10000;  // 10 seconds
static const uint16_t MAX_WIFI_FULL_RECONNECT_INTERVAL = 60000;   // 60 seconds
static const uint16_t STEP_WIFI_FULL_RECONNECT_INTERVAL = 5000;   // 5 seconds
static const uint16_t MAX_RECONNECT_ATTEMPTS =
    3;  // Maximum number of reconnect attempts before forcing a full connection

// State variables
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastWiFiCheck = 0;
static bool hasConnectedBefore = false;
static uint16_t reconnectAttempts = 0;  // Counter for reconnect attempts
static uint16_t current_full_reconnect_interval = INIT_WIFI_FULL_RECONNECT_INTERVAL;

void init_WiFi() {
  Serial.begin(115200);

#ifdef WIFIAP
  if (AccessPointEnabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
  } else {
    WiFi.mode(WIFI_STA);  // Only Router connection
  }
#else
  WiFi.mode(WIFI_STA);  // Only Router connection
#endif  // WIFIAP

  // Set WiFi to auto reconnect
  WiFi.setAutoReconnect(true);

#ifdef WIFICONFIG
  // Set static IP
  WiFi.config(local_IP, gateway, subnet);
#endif

  // Initialize Wi-Fi event handlers
  WiFi.onEvent(onWifiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  // Start Wi-Fi connection
  connectToWiFi();
}

// Task to monitor Wi-Fi status and handle reconnections
void wifi_monitor() {
  unsigned long currentMillis = millis();

  // Check if it's time to monitor the Wi-Fi status
  if (currentMillis - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = currentMillis;

    wl_status_t status = WiFi.status();
    if (status != WL_CONNECTED) {
      Serial.println("Wi-Fi not connected, attempting to reconnect...");

      // Try WiFi.reconnect() if it was successfully connected at least once
      if (hasConnectedBefore) {
        if (WiFi.reconnect()) {
          Serial.println("Wi-Fi reconnect attempt...");
          reconnectAttempts = 0;  // Reset the attempt counter on successful reconnect
        } else {
          reconnectAttempts++;
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            Serial.println("Failed to reconnect multiple times, forcing a full connection attempt...");
            FullReconnectToWiFi();
          }
        }
      } else {
        // If no previous connection, force a full connection attempt
        if (currentMillis - lastReconnectAttempt > current_full_reconnect_interval) {
          Serial.println("No previous OK connection, force a full connection attempt...");
          FullReconnectToWiFi();
        }
      }
    }
  }
}

// Function to force a full reconnect to Wi-Fi
static void FullReconnectToWiFi() {

  // Increase the current reconnect interval if it's not at the maximum
  if (current_full_reconnect_interval + STEP_WIFI_FULL_RECONNECT_INTERVAL <= MAX_WIFI_FULL_RECONNECT_INTERVAL) {
    current_full_reconnect_interval += STEP_WIFI_FULL_RECONNECT_INTERVAL;
  }
  hasConnectedBefore = false;       // Reset the flag to force a full reconnect
  lastReconnectAttempt = millis();  // Reset the reconnect attempt timer
  WiFi.disconnect();                //force disconnect from the current network
  connectToWiFi();                  //force a full connection attempt
}

// Function to handle Wi-Fi connection
static void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(ssid.c_str(), password.c_str(), wifi_channel);
  } else {
    Serial.println("Wi-Fi already connected.");
  }
}

// Event handler for successful Wi-Fi connection
static void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  set_event(EVENT_WIFI_CONNECT, 0);
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  hasConnectedBefore = true;                                            // Mark as successfully connected at least once
  reconnectAttempts = 0;                                                // Reset the attempt counter
  current_full_reconnect_interval = INIT_WIFI_FULL_RECONNECT_INTERVAL;  // Reset the full reconnect interval
  clear_event(EVENT_WIFI_CONNECT);
}

// Event handler for Wi-Fi Got IP
static void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Wi-Fi Got IP.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Event handler for Wi-Fi disconnection
static void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  set_event(EVENT_WIFI_DISCONNECT, 0);
  Serial.println("Wi-Fi disconnected.");
  lastReconnectAttempt = millis();  // Reset reconnection attempt timer
  clear_event(EVENT_WIFI_DISCONNECT);
}

#ifdef MDNSRESPONDER
// Initialise mDNS
void init_mDNS() {

  // Calulate the host name using the last two chars from the MAC address so each one is likely unique on a network.
  // e.g batteryemulator8C.local where the mac address is 08:F9:E0:D1:06:8C
  String mac = WiFi.macAddress();
  String mdnsHost = "batteryemulator" + mac.substring(mac.length() - 2);

  // Initialize mDNS .local resolution
  if (!MDNS.begin(mdnsHost)) {
#ifdef DEBUG_VIA_USB
    Serial.println("Error setting up MDNS responder!");
#endif
  } else {
    // Advertise via bonjour the service so we can auto discover these battery emulators on the local network.
    MDNS.addService("battery_emulator", "tcp", 80);
  }
}
#endif  // MDNSRESPONDER

#ifdef WIFIAP
void init_WiFi_AP() {
#ifdef DEBUG_VIA_USB
  Serial.println("Creating Access Point: " + String(ssidAP));
  Serial.println("With password: " + String(passwordAP));
#endif  // DEBUG_VIA_USB
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
#ifdef DEBUG_VIA_USB
  Serial.println("Access Point created.");
  Serial.print("IP address: ");
  Serial.println(IP);
#endif  // DEBUG_VIA_USB
}
#endif  // WIFIAP
