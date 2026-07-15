#include "wifi.h"
#include <esp_mac.h>                                                     // esp_read_mac()
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"  // hold_pins_across_reset()
#include "../../communication/nvm/comm_nvm.h"
#include "../hal/hal.h"  // esp32hal / AP_BUTTON_PIN()
#include "../safety/safety.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/logging.h"
#ifndef SMALL_FLASH_DEVICE
#include <ESPmDNS.h>
#endif

bool wifi_enabled = true;
bool wifiap_enabled = true;
bool mdns_enabled = true;    //If true, allows battery monitor te be found by .local address
bool espnow_enabled = true;  //If true, allows battery emulator to send battery status by using ESPNow messages
uint16_t wifi_channel = 0;
#ifndef SMALL_FLASH_DEVICE
extern const char* version_number;
#endif

std::string custom_hostname;  //If not set, defaults to "battery-emulator-" + last two MAC bytes (see init_WiFi)
std::string ssid;
std::string password;
std::string ssidAP;
std::string passwordAP;
const char* DEFAULT_AP_PASSWORD = "123456789";

// Set your Static IP address. Only used incase Static address option is set
bool static_IP_enabled = false;
std::string static_local_IP;
std::string static_gateway;
std::string static_subnet;
std::string static_dns;

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

bool ap_active = false;
static bool ap_button_inited = false;
static bool ap_button_was_pressed = false;
static unsigned long ap_button_press_start = 0;
static const unsigned long AP_BUTTON_AP_MS = 5000;              // >=5 s: start AP
static const unsigned long AP_BUTTON_STA_WIPE_MS = 15000;       // >=15 s: wipe STA settings + reboot
static const unsigned long AP_BUTTON_FACTORY_RESET_MS = 30000;  // >=30 s: factory reset

// Provisioning window: while the AP runs with the factory-default password it is
// only kept up for a limited time, then shut down. Rebooting or long-pressing the
// BOOT button (>=5 s) opens a new window. Setting a custom AP password lifts the
// restriction entirely (AP stays up indefinitely).
static const unsigned long AP_PROVISIONING_WINDOW_MS = 5UL * 60UL * 1000UL;  // 5 minutes
static unsigned long ap_started_at = 0;                                      // millis() when the AP was (re)started
static bool ap_provisioning_expired = false;  // blocks automatic AP re-enable until reboot/button

static bool ap_password_is_default() {
  return passwordAP == DEFAULT_AP_PASSWORD;
}

// Shut the AP down once it has been running with the factory-default password
// for longer than the provisioning window.
static void check_ap_provisioning_window() {
  if (!ap_active || !ap_password_is_default()) {
    return;
  }
  if (millis() - ap_started_at < AP_PROVISIONING_WINDOW_MS) {
    return;
  }
  if (WiFi.softAPgetStationNum() > 0) {
    return;  // a client is connected: don't cut off an active provisioning session
  }
  // Direct log so EVERY expiry produces a log/syslog line, not just the first per
  // boot (set_event only emits its log line on the inactive->active transition).
  LOG_SET_NEXT_SEVERITY(6);  // info
  logging.println("AP provisioning window expired (factory-default AP password), disabling access point.");
  WiFi.softAPdisconnect(true);  // stop the AP and drop the AP bit from the WiFi mode; STA stays up
  ap_active = false;
  ap_provisioning_expired = true;
  // The advance warning is about a *running* AP; the timeout event below takes over from here.
  clear_event(EVENT_WIFI_AP_PASSWORD_DEFAULT);
  set_event(EVENT_WIFI_AP_PROVISION_TIMEOUT, 0);
}

String default_hostname() {
  uint8_t mac_bytes[6];
  esp_read_mac(mac_bytes, ESP_MAC_WIFI_STA);  // reads eFuse directly, valid even before WiFi starts
  char mac_suffix[5];
  snprintf(mac_suffix, sizeof(mac_suffix), "%02x%02x", mac_bytes[4], mac_bytes[5]);
  return "battery-emulator-" + String(mac_suffix);
}

// Initialise mDNS
static void init_mDNS() {
#ifndef SMALL_FLASH_DEVICE
  // Reuse the network hostname (custom, or the "battery-emulator-<mac>" default set in init_WiFi()). Be consistent with AP too.
  String mdnsHost = String(WiFi.getHostname());

  // Initialize mDNS .local resolution
  if (!MDNS.begin(mdnsHost)) {
    logging.println("Error setting up mDNS responder!");
  } else {
    // Advertise via bonjour the web inteface so we can auto discover these battery emulators on the local network.
    MDNS.addService("http", "tcp", 80);
    logging.println("mDNS responder started.");
  }
#endif
}

void init_WiFi() {
  DEBUG_PRINTF("init_Wifi enabled=%d, ap=%d, ssid=%s\n", wifi_enabled, wifiap_enabled, ssid.c_str());

  // Keep the WiFi driver's mode/config changes in RAM instead of NVS. Credentials
  // are stored in our own Preferences and reapplied at boot, so driver-level
  // persistence is redundant. Without this, esp_wifi_set_config()/set_mode() (e.g.
  // softAPdisconnect() on AP provisioning timeout) write to NVS, and the flash
  // erase suspends the cache, stalling tasks on BOTH cores for up to ~45 ms.
  WiFi.persistent(false);

  // Register event handlers BEFORE WiFi.mode() creates the arduino_events task.
  // WiFi events can fire immediately once the task exists, and vector reallocation
  // during concurrent emplace_back() would corrupt the iterator in _checkForEvent().
  WiFi.onEvent(onWifiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  // Always set a WiFi hostname: the user's custom one if set, otherwise a default of
  // "battery-emulator-" + the last two bytes of the MAC address, so every device has a
  // meaningful, likely-unique hostname even without configuration.
  String hostname;
  if (!custom_hostname.empty()) {
    hostname = String(custom_hostname.c_str());
  } else {
    hostname = default_hostname();
  }
  WiFi.setHostname(hostname.c_str());
  ssidAP = std::string(hostname.c_str());  // Access Point SSID now matches the hostname, be consistent with MDNS too

  if (wifiap_enabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
  } else if (wifi_enabled) {
    WiFi.mode(WIFI_STA);  // Only Router connection
  }

  // Set WiFi to auto reconnect
  WiFi.setAutoReconnect(true);

  if (static_IP_enabled) {
    IPAddress local_IP, gateway, subnet, dns;
    if (local_IP.fromString(static_local_IP.c_str()) && gateway.fromString(static_gateway.c_str()) &&
        subnet.fromString(static_subnet.c_str())) {
      // WiFi.config() stops the DHCP client and unconditionally overwrites the DNS server. Passing no DNS
      // therefore leaves the resolver at 0.0.0.0 and breaks MQTT-by-hostname/release checks. Default to
      // the gateway, which is the resolver on virtually every home network.
      if (!dns.fromString(static_dns.c_str())) {
        dns = gateway;
      }
      if (!WiFi.config(local_IP, gateway, subnet, dns)) {
        logging.println("Static IP configuration rejected, falling back to DHCP");
      }
    } else {
      logging.println("Static IP settings are invalid, falling back to DHCP");
    }
  }

  // Start Wi-Fi connection
  DEBUG_PRINTF("start Wifi\n");
  connectToWiFi();

  DEBUG_PRINTF("init_Wifi complete\n");
}

// Board button (usually BOOT/GPIO0):
//   held >= 30 s then released -> factory reset (clear settings) + reboot
//   held >=  5 s then released -> start the Wi-Fi AP if it isn't running
static void check_ap_button() {
  const gpio_num_t pin = esp32hal->AP_BUTTON_PIN();
  if (pin == GPIO_NUM_NC) {
    return;  // board has no AP button
  }

  if (!ap_button_inited) {
    // Configure lazily, after boot, so we never disturb GPIO0 strapping at reset.
    pinMode(pin, INPUT_PULLUP);
    ap_button_inited = true;
    return;  // let the pull-up settle before the first read
  }

  const bool pressed = (digitalRead(pin) == LOW);  // active-low (button to GND)
  const unsigned long now = millis();

  if (pressed && !ap_button_was_pressed) {
    ap_button_press_start = now;  // press started
  } else if (pressed && ap_button_was_pressed) {
    // Still held: white blink, rate steps up as each tier is reached.
    const unsigned long held = now - ap_button_press_start;
    if (held >= AP_BUTTON_FACTORY_RESET_MS) {
      set_led_override(true, LED_COLOR_WHITE, 100);  // >=30 s
    } else if (held >= AP_BUTTON_STA_WIPE_MS) {
      set_led_override(true, LED_COLOR_WHITE, 200);  // >=15 s
    } else if (held >= AP_BUTTON_AP_MS) {
      set_led_override(true, LED_COLOR_WHITE, 400);  // >=5 s
    } else {
      set_led_override(false, 0, 0);  // <5 s: no feedback yet
    }
  } else if (!pressed && ap_button_was_pressed) {
    // Released: act based on how long it was held.
    set_led_override(false, 0, 0);  // released: stop blink feedback
    const unsigned long held = now - ap_button_press_start;
    if (held >= AP_BUTTON_FACTORY_RESET_MS) {
      BatteryEmulatorSettingsStore settings;
      settings.clearAll();
      logging.println("Factory reset performed from the board button.");
      erase_phy_cal_data();
      graceful_restart();
    } else if (held >= AP_BUTTON_STA_WIPE_MS) {
      clear_wifi_sta_settings();
      logging.println("Network settings wiped from the board button.");
      erase_phy_cal_data();
      hold_pins_across_reset();
      graceful_restart();
    } else if (held >= AP_BUTTON_AP_MS) {
      if (!ap_active) {
        ap_provisioning_expired = false;  // manual start opens a fresh provisioning window
        WiFi.mode(WIFI_AP_STA);
        init_WiFi_AP();  // sets ap_active, restarts the provisioning window timer
        logging.println("AP started from the board button.");
      }
    }
  }
  ap_button_was_pressed = pressed;
}

// Task to monitor Wi-Fi status and handle reconnections
void wifi_monitor() {
  check_ap_button();
  check_ap_provisioning_window();

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
          // Don't resurrect the rescue AP if its provisioning window already
          // expired with the factory-default password still in place.
          if (!ap_provisioning_expired) {
            wifiap_enabled = true;
            WiFi.mode(WIFI_AP_STA);
            init_WiFi_AP();
          }

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
#ifndef SMALL_FLASH_DEVICE
  syslog_start();
#endif

  //clear disconnects events if we got a IP
  clear_event(EVENT_WIFI_DISCONNECT);
  logging.print("Wi-Fi Got IP. ");
  logging.print("IP address: ");
  logging.println(WiFi.localIP().toString());

#ifndef SMALL_FLASH_DEVICE
  // One-shot boot notice — fires once per boot, not on every reconnect.
  static bool boot_logged = false;
  if (!boot_logged) {
    boot_logged = true;
    LOG_SET_NEXT_SEVERITY(5);  // RFC 5424 severity 5 = Notice
    logging.printf("Bootup complete, running version %s\n", version_number);
  }
#endif

  static bool mdns_started = false;
  if (mdns_enabled && !mdns_started) {
    init_mDNS();
    mdns_started = true;
  }
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

void init_WiFi_AP() {

  DEBUG_PRINTF("Creating Access Point: %s\n", ssidAP.c_str());
  DEBUG_PRINTF("Access Point password set (%u characters)\n", (unsigned)passwordAP.length());

  if (!ap_active) {
    // (Re)start the provisioning window timer only on an off->on transition, so
    // repeated re-inits from the STA reconnect fallback don't keep extending it.
    ap_started_at = millis();
  }

  WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());
  bool ap_was_active = ap_active;
  ap_active = true;

  if (!ap_was_active && ap_password_is_default()) {
    // Warn in advance that the provisioning window is ticking. Direct log fires on
    // every AP start; the event additionally shows on the web UI / MQTT (set_event
    // emits its own log line only on the first inactive->active transition per boot).
    LOG_SET_NEXT_SEVERITY(6);  // info
    logging.println("AP using default password.");
    set_event(EVENT_WIFI_AP_PASSWORD_DEFAULT, 0);
  }
  IPAddress IP = WiFi.softAPIP();

  DEBUG_PRINTF("Access Point created, IP address: %s\n", IP.toString().c_str());
}
