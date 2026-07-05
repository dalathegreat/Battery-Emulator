#include "wifi.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#ifndef SMALL_FLASH_DEVICE
#include <ESPmDNS.h>
#endif

bool wifi_enabled = true;
bool wifiap_enabled = true;
bool mdns_enabled = true;    //If true, allows battery monitor te be found by .local address
bool espnow_enabled = true;  //If true, allows battery emulator to send battery status by using ESPNow messages
uint16_t wifi_channel = 0;

std::string custom_hostname;  //If not set, the default naming format 'esp32-XXXXXX' will be used
std::string ssid;
std::string password;
std::string ssidAP;
std::string passwordAP;

// Set your Static IP address. Only used incase Static address option is set
bool static_IP_enabled = false;
uint8_t static_local_IP1 = 0;
uint8_t static_local_IP2 = 0;
uint8_t static_local_IP3 = 0;
uint8_t static_local_IP4 = 0;
uint8_t static_gateway1 = 0;
uint8_t static_gateway2 = 0;
uint8_t static_gateway3 = 0;
uint8_t static_gateway4 = 0;
uint8_t static_subnet1 = 0;
uint8_t static_subnet2 = 0;
uint8_t static_subnet3 = 0;
uint8_t static_subnet4 = 0;

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

static bool ap_button_inited = false;
static bool ap_button_was_pressed = false;
static unsigned long ap_button_press_start = 0;
static const unsigned long AP_BUTTON_HOLD_MS = 5000;  // 5-second long-press to start AP

void init_WiFi() {
  DEBUG_PRINTF("init_Wifi enabled=%d, ap=%d, ssid=%s\n", wifi_enabled, wifiap_enabled, ssid.c_str());

  // Register event handlers BEFORE WiFi.mode() creates the arduino_events task.
  // WiFi events can fire immediately once the task exists, and vector reallocation
  // during concurrent emplace_back() would corrupt the iterator in _checkForEvent().
  WiFi.onEvent(onWifiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  if (!custom_hostname.empty()) {
    WiFi.setHostname(custom_hostname.c_str());
  }

  if (wifiap_enabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
  } else if (wifi_enabled) {
    WiFi.mode(WIFI_STA);  // Only Router connection
  }

  // Set WiFi to auto reconnect
  WiFi.setAutoReconnect(true);

  if (static_IP_enabled) {
    // Set static IP
    IPAddress local_IP((uint8_t)static_local_IP1, (uint8_t)static_local_IP2, (uint8_t)static_local_IP3,
                       (uint8_t)static_local_IP4);
    IPAddress gateway((uint8_t)static_gateway1, (uint8_t)static_gateway2, (uint8_t)static_gateway3,
                      (uint8_t)static_gateway4);
    IPAddress subnet((uint8_t)static_subnet1, (uint8_t)static_subnet2, (uint8_t)static_subnet3,
                     (uint8_t)static_subnet4);
    WiFi.config(local_IP, gateway, subnet);
  }

  // Start Wi-Fi connection
  DEBUG_PRINTF("start Wifi\n");
  connectToWiFi();

  DEBUG_PRINTF("init_Wifi complete\n");
}

// Task to monitor Wi-Fi status and handle reconnections
void wifi_monitor() {
  check_ap_button();

  if (ssid.empty() || password.empty()) {
    return;
  }

  unsigned long currentMillis = millis();

  // Check if it's time to monitor the Wi-Fi status
  // WIFI_CHECK_INTERVAL for normal checks and INIT_WIFI_FULL_RECONNECT_INTERVAL for first connections or  full connect attepts
  if ((hasConnectedBefore && (currentMillis - lastWiFiCheck > current_check_interval)) ||
      (!hasConnectedBefore && (currentMillis - lastWiFiCheck > INIT_WIFI_FULL_RECONNECT_INTERVAL))) {

    // Uncomment for testing, but otherwise this quickly fills up the log
    //DEBUG_PRINTF("Wi-Fi status: %d, %d, %d, %d, %d\n", hasConnectedBefore, currentMillis, lastWiFiCheck,
    //             current_check_interval, INIT_WIFI_FULL_RECONNECT_INTERVAL);

    lastWiFiCheck = currentMillis;

    wl_status_t status = WiFi.status();
    // WL_IDLE_STATUS can mean we're connected but haven't yet gotten the IP.
    if (status != WL_CONNECTED && status != WL_IDLE_STATUS) {
      // Increase the current check interval if it's not at the maximum
      if (current_check_interval + STEP_WIFI_CHECK_INTERVAL <= MAX_STEP_WIFI_CHECK_INTERVAL) {
        current_check_interval += STEP_WIFI_CHECK_INTERVAL;
      }
      DEBUG_PRINTF("Wi-Fi not connected (status=%d), attempting to reconnect\n", status);

      // Try WiFi.reconnect() if it was successfully connected at least once
      if (hasConnectedBefore) {
        lastReconnectAttempt = currentMillis;  // Reset reconnection attempt timer
        logging.println("Wi-Fi reconnect attempt...");
        if (WiFi.reconnect()) {
          logging.println("Wi-Fi reconnect attempt sucess...");
          reconnectAttempts = 0;  // Reset the attempt counter on successful reconnect
        } else {
          logging.println("Wi-Fi reconnect attempt error...");
          reconnectAttempts++;
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            logging.println("Failed to reconnect multiple times, forcing a full connection attempt...");
            FullReconnectToWiFi();
          }
        }
      } else {
        // If no previous connection, force a full connection attempt
        if (currentMillis - lastReconnectAttempt > current_full_reconnect_interval) {
          logging.println("No previous OK connection, force a full connection attempt...");
          wifiap_enabled = true;
          WiFi.mode(WIFI_AP_STA);
          init_WiFi_AP();

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
  if (ssid.empty() || password.empty()) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    lastReconnectAttempt = millis();  // Reset the reconnect attempt timer
    logging.println("Connecting to Wi-Fi...");
    if (wifi_channel > 14) {
      wifi_channel = 0;
    }  //prevent users going out of bounds
    DEBUG_PRINTF("Connecting to Wi-Fi SSID: %s, Channel: %d\n", ssid.c_str(), wifi_channel);
    WiFi.begin(ssid.c_str(), password.c_str(), wifi_channel);
  } else {
    logging.println("Wi-Fi already connected.");
  }
}

// Event handler for successful Wi-Fi connection
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  clear_event(EVENT_WIFI_DISCONNECT);
  set_event(EVENT_WIFI_CONNECT, 0);
  connected_once = true;
  DEBUG_PRINTF("Wi-Fi connected. status: %d, RSSI: %d dBm, IP address: %s, SSID: %s\n", WiFi.status(), -WiFi.RSSI(),
               WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());
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
  logging.print("Wi-Fi Got IP. ");
  logging.print("IP address: ");
  logging.println(WiFi.localIP().toString());
}

// Event handler for Wi-Fi disconnection
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {

  if (connected_once) {
    set_event(EVENT_WIFI_DISCONNECT, 0);
  }
  logging.println("Wi-Fi disconnected.");
  //we dont do anything here, the reconnect will be handled by the monitor
  //too many events received when the connection is lost
  //normal reconnect retry start at first 2 seconds
}

// Initialise mDNS (Only available on devices with )
void init_mDNS() {
#ifndef SMALL_FLASH_DEVICE
  // Calulate the host name using the last two chars from the MAC address so each one is likely unique on a network.
  // e.g batteryemulator8C.local where the mac address is 08:F9:E0:D1:06:8C
  String mac = WiFi.macAddress();
  String mdnsHost = "batteryemulator" + mac.substring(mac.length() - 2);

  if (!custom_hostname.empty()) {
    mdnsHost = String(custom_hostname.c_str());
  }

  // Initialize mDNS .local resolution
  if (!MDNS.begin(mdnsHost)) {
    logging.println("Error setting up MDNS responder!");
  } else {
    // Advertise via bonjour the service so we can auto discover these battery emulators on the local network.
    MDNS.addService(mdnsHost, "tcp", 80);
  }
#endif
}

void init_WiFi_AP() {

  DEBUG_PRINTF("Creating Access Point: %s\n", ssidAP.c_str());
  DEBUG_PRINTF("Access Point password is set (%u characters)\n", (unsigned)passwordAP.length());

  WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());
  IPAddress IP = WiFi.softAPIP();

  DEBUG_PRINTF("Access Point created.\nIP address: %s\n", IP.toString().c_str());
}

// Long-press the board button (usually BOOT/GPIO0) for 5 s to start the AP if it's off.
static void check_ap_button() {
  const gpio_num_t pin = esp32hal->AP_BUTTON_PIN();
  if (pin == GPIO_NUM_NC) {
    return;  // board has no AP button
  }

  if (!ap_button_inited) {
    // Configure lazily, after boot, so we never disturb GPIO0's strapping at reset.
    pinMode(pin, INPUT_PULLUP);
    ap_button_inited = true;
    return;  // let the pull-up settle before the first read
  }

  const bool pressed = (digitalRead(pin) == LOW);  // active-low (button to GND)
  const unsigned long now = millis();

  if (pressed && !ap_button_was_pressed) {
    ap_button_press_start = now;  // press started
  } else if (pressed && ap_button_was_pressed) {
    if (now - ap_button_press_start >= AP_BUTTON_HOLD_MS) {
      if (!ap_active) {
        logging.println("AP button long-press: starting Wi-Fi access point.");
        WiFi.mode(WIFI_AP_STA);
        init_WiFi_AP();  // idempotent; sets ap_active
      }
      ap_button_press_start = now;  // re-arm; won't retrigger until released/re-held
    }
  }
  ap_button_was_pressed = pressed;
}
