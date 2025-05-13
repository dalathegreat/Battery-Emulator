#include "wifi.h"
#include "../../include.h"
#include "../utils/events.h"

// Configuration Parameters
static const uint16_t WIFI_CHECK_INTERVAL = 2000;       // 1 seconds normal check interval when last connected
static const uint16_t STEP_WIFI_CHECK_INTERVAL = 2000;  // 3 seconds wait step increase in checks for normal reconnects
static const uint16_t MAX_STEP_WIFI_CHECK_INTERVAL =
    10000;  // 15 seconds wait step increase in checks for normal reconnects

static const uint16_t INIT_WIFI_FULL_RECONNECT_INTERVAL =
    10000;  // 10 seconds starting wait interval for full reconnects and first connection
static const uint16_t MAX_WIFI_FULL_RECONNECT_INTERVAL = 60000;  // 60 seconds maximum wait interval for full reconnects
static const uint16_t STEP_WIFI_FULL_RECONNECT_INTERVAL =
    5000;  // 5 seconds wait step increase in checks for full reconnects
static const uint16_t MAX_RECONNECT_ATTEMPTS =
    3;  // Maximum number of reconnect attempts before forcing a full connection

// State variables
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastWiFiCheck = 0;
static bool hasConnectedBefore = false;

static uint16_t reconnectAttempts = 0;  // Counter for reconnect attempts
static uint16_t current_full_reconnect_interval = INIT_WIFI_FULL_RECONNECT_INTERVAL;
static uint16_t current_check_interval = WIFI_CHECK_INTERVAL;
static bool connected_once = false;

void init_WiFi() {

#ifdef WIFIAP
  WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
  init_WiFi_AP();
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
  // WIFI_CHECK_INTERVAL for normal checks and INIT_WIFI_FULL_RECONNECT_INTERVAL for first connections or  full connect attepts
  if ((hasConnectedBefore && (currentMillis - lastWiFiCheck > current_check_interval)) ||
      (!hasConnectedBefore && (currentMillis - lastWiFiCheck > INIT_WIFI_FULL_RECONNECT_INTERVAL))) {

    lastWiFiCheck = currentMillis;

    wl_status_t status = WiFi.status();
    if (status != WL_CONNECTED) {
      // Increase the current check interval if it's not at the maximum
      if (current_check_interval + STEP_WIFI_CHECK_INTERVAL <= MAX_STEP_WIFI_CHECK_INTERVAL)
        current_check_interval += STEP_WIFI_CHECK_INTERVAL;
#ifdef DEBUG_LOG
      logging.println("Wi-Fi not connected, attempting to reconnect...");
#endif
      // Try WiFi.reconnect() if it was successfully connected at least once
      if (hasConnectedBefore) {
        lastReconnectAttempt = currentMillis;  // Reset reconnection attempt timer
#ifdef DEBUG_LOG
        logging.println("Wi-Fi reconnect attempt...");
#endif
        if (WiFi.reconnect()) {
#ifdef DEBUG_LOG
          logging.println("Wi-Fi reconnect attempt sucess...");
#endif
          reconnectAttempts = 0;  // Reset the attempt counter on successful reconnect
        } else {
#ifdef DEBUG_LOG
          logging.println("Wi-Fi reconnect attempt error...");
#endif
          reconnectAttempts++;
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
#ifdef DEBUG_LOG
            logging.println("Failed to reconnect multiple times, forcing a full connection attempt...");
#endif
            FullReconnectToWiFi();
          }
        }
      } else {
        // If no previous connection, force a full connection attempt
        if (currentMillis - lastReconnectAttempt > current_full_reconnect_interval) {
#ifdef DEBUG_LOG
          logging.println("No previous OK connection, force a full connection attempt...");
#endif
          FullReconnectToWiFi();
        }
      }
    }
  }
}

// Function to force a full reconnect to Wi-Fi
void FullReconnectToWiFi() {

  // Increase the current reconnect interval if it's not at the maximum
  if (current_full_reconnect_interval + STEP_WIFI_FULL_RECONNECT_INTERVAL <= MAX_WIFI_FULL_RECONNECT_INTERVAL) {
    current_full_reconnect_interval += STEP_WIFI_FULL_RECONNECT_INTERVAL;
  }
  hasConnectedBefore = false;  // Reset the flag to force a full reconnect
  WiFi.disconnect();           //force disconnect from the current network
  connectToWiFi();             //force a full connection attempt
}

// Function to handle Wi-Fi connection
void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    lastReconnectAttempt = millis();  // Reset the reconnect attempt timer
#ifdef DEBUG_LOG
    logging.println("Connecting to Wi-Fi...");
#endif
    WiFi.begin(ssid.c_str(), password.c_str(), wifi_channel);
  } else {
#ifdef DEBUG_LOG
    logging.println("Wi-Fi already connected.");
#endif
  }
}

// Event handler for successful Wi-Fi connection
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  clear_event(EVENT_WIFI_DISCONNECT);
  set_event(EVENT_WIFI_CONNECT, 0);
  connected_once = true;
#ifdef DEBUG_LOG
  logging.print("Wi-Fi connected. RSSI: ");
  logging.print(-WiFi.RSSI());
  logging.print(" dBm, IP address: ");
  logging.println(WiFi.localIP().toString());
#endif
  hasConnectedBefore = true;                                            // Mark as successfully connected at least once
  reconnectAttempts = 0;                                                // Reset the attempt counter
  current_full_reconnect_interval = INIT_WIFI_FULL_RECONNECT_INTERVAL;  // Reset the full reconnect interval
  current_check_interval = WIFI_CHECK_INTERVAL;                         // Reset the full reconnect interval
  clear_event(EVENT_WIFI_CONNECT);
}

// Event handler for Wi-Fi Got IP
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  //clear disconnects events if we got a IP
  clear_event(EVENT_WIFI_DISCONNECT);
#ifdef DEBUG_LOG
  logging.print("Wi-Fi Got IP. ");
  logging.print("IP address: ");
  logging.println(WiFi.localIP().toString());
#endif
}

// Event handler for Wi-Fi disconnection
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (connected_once)
    set_event(EVENT_WIFI_DISCONNECT, 0);
#ifdef DEBUG_LOG
  logging.println("Wi-Fi disconnected.");
#endif
  //we dont do anything here, the reconnect will be handled by the monitor
  //too many events received when the connection is lost
  //normal reconnect retry start at first 2 seconds
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
#ifdef DEBUG_LOG
    logging.println("Error setting up MDNS responder!");
#endif
  } else {
    // Advertise via bonjour the service so we can auto discover these battery emulators on the local network.
    MDNS.addService("battery_emulator", "tcp", 80);
  }
}
#endif  // MDNSRESPONDER

#ifdef WIFIAP
void init_WiFi_AP() {
#ifdef DEBUG_LOG
  logging.println("Creating Access Point: " + String(ssidAP));
  logging.println("With password: " + String(passwordAP));
#endif
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
#ifdef DEBUG_LOG
  logging.println("Access Point created.");
  logging.print("IP address: ");
  logging.println(IP.toString());
#endif
}
#endif  // WIFIAP
