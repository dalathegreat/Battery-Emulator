#include "wifi.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"  // hold_pins_across_reset()
#include "../../communication/nvm/comm_nvm.h"
#include "../hal/hal.h"                 // esp32hal / AP_BUTTON_PIN()
#include "../network/hostname.h"        // active_hostname()
#include "../network/network_status.h"  // network_bring_services_up()
#include "../safety/safety.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/logging.h"

bool wifiap_enabled = true;
bool espnow_enabled = true;         //If true, allows battery emulator to send battery status by using ESPNow messages
std::string espnow_peer_macs = "";  //Empty = broadcast, otherwise a list of receiver MAC addresses
uint16_t wifi_channel = 0;
extern const char* version_number;

std::string ssid;
std::string password;
std::string ssidAP;
std::string passwordAP;
const char* DEFAULT_AP_PASSWORD = "123456789";

// Set your Static IP address. Only used incase Static address option is set
bool wifi_static_IP_enabled = false;
IPAddress wifi_static_local_IP;
IPAddress wifi_static_gateway;
IPAddress wifi_static_subnet;
IPAddress wifi_static_dns;

// Configuration Parameters
static const uint16_t WIFI_CHECK_INTERVAL = 2000;            // 2 s normal check interval when last connected
static const uint16_t STEP_WIFI_CHECK_INTERVAL = 2000;       // 2 s step increase in checks for normal reconnects
static const uint16_t MAX_STEP_WIFI_CHECK_INTERVAL = 10000;  // 10 s cap on the normal-reconnect check interval

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
  LOG_SET_NEXT_SEVERITY(5);  // notice
  logging.println("AP provisioning window expired (factory-default AP password), disabling access point.");
  WiFi.softAPdisconnect(true);  // stop the AP and drop the AP bit from the WiFi mode; STA stays up
  ap_active = false;
  ap_provisioning_expired = true;
  // The advance warning is about a *running* AP; the timeout event below takes over from here.
  clear_event(EVENT_WIFI_AP_PASSWORD_DEFAULT);
  set_event(EVENT_WIFI_AP_PROVISION_TIMEOUT, 0);
}

// STA is configured only when the user has actually configured credentials.
static bool wifi_sta_configured() {
  return !ssid.empty() && !password.empty();
}

// The WiFi radio only needs to come up if either the AP is broadcast or STA
// credentials are configured
static bool wifi_required() {
  return wifiap_enabled || wifi_sta_configured();
}

void init_WiFi() {
  if (!wifi_required()) {
    DEBUG_PRINTF("init_Wifi: neither AP nor STA configured; skipping\n");
    return;
  }

  //DEBUG_PRINTF("init_Wifi ap=%d, ssid=%s\n", wifiap_enabled, ssid.c_str());

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
  WiFi.onEvent(onApStaConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(onApStaDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  // Always set a WiFi hostname: the user's custom one if set, otherwise a default of
  // "battery-emulator-" + the last two bytes of the MAC address, so every device has a
  // meaningful, likely-unique hostname even without configuration.
  String hostname = active_hostname();
  WiFi.setHostname(hostname.c_str());
  ssidAP = std::string(hostname.c_str());  // Access Point SSID now matches the hostname, be consistent with MDNS too

  if (wifiap_enabled) {
    WiFi.mode(WIFI_AP_STA);  // Simultaneous WiFi AP and Router connection
    init_WiFi_AP();
  } else {
    WiFi.mode(WIFI_STA);  // Only Router connection
  }

  // Set WiFi to auto reconnect
  WiFi.setAutoReconnect(true);

  // Always associate with the strongest AP when several access points broadcast the same
  // SSID (mesh / multi-AP networks). The driver defaults to WIFI_FAST_SCAN, which stops at
  // the first matching AP it hears and can therefore latch onto a distant one. A full scan
  // collects every candidate first, and the sort method then picks the best RSSI.
  // Both settings end up in the STA config, so WiFi.reconnect(), FullReconnectToWiFi() and
  // the driver's own auto-reconnect all reuse them: the strongest AP is picked at boot AND
  // at every reconnect. Takes roughly 1 second of scan time per connection attempt.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  if (wifi_static_IP_enabled) {
    if (wifi_static_local_IP != IPAddress() && wifi_static_gateway != IPAddress() &&
        wifi_static_subnet != IPAddress()) {
      // WiFi.config() stops the DHCP client and unconditionally overwrites the DNS server. Passing no DNS
      // therefore leaves the resolver at 0.0.0.0 and breaks MQTT-by-hostname/release checks. Default to
      // the gateway, which is the resolver on virtually every home network.
      IPAddress dns = (wifi_static_dns != IPAddress()) ? wifi_static_dns : wifi_static_gateway;
      if (!WiFi.config(wifi_static_local_IP, wifi_static_gateway, wifi_static_subnet, dns)) {
        logging.println("Static IP configuration rejected, falling back to DHCP");
      }
    } else {
      logging.println("Static IP settings are invalid, falling back to DHCP");
    }
  }

  // Start Wi-Fi connection
  //DEBUG_PRINTF("start Wifi\n");
  connectToWiFi();

  //DEBUG_PRINTF("init_Wifi complete\n");
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
    pinMode(pin, (pin < GPIO_NUM_34) ? INPUT_PULLUP : INPUT);
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
      LOG_SET_NEXT_SEVERITY(5);  // notice
      logging.println("Factory reset performed from the board button.");
      erase_phy_cal_data();
      graceful_restart();
    } else if (held >= AP_BUTTON_STA_WIPE_MS) {
      clear_wifi_sta_settings();
      LOG_SET_NEXT_SEVERITY(5);  // notice
      logging.println("Network settings wiped from the board button.");
      erase_phy_cal_data();
      hold_pins_across_reset();
      graceful_restart();
    } else if (held >= AP_BUTTON_AP_MS) {
      if (!ap_active) {
        ap_provisioning_expired = false;  // manual start opens a fresh provisioning window
        // Emergency recovery: bring the AP up for this boot only.
        wifiap_enabled = true;
        if (WiFi.getMode() == WIFI_MODE_NULL) {
          // Radio was off: bring it up + register handlers
          init_WiFi();
        } else {
          // Radio already up (STA configured): just add the AP.
          WiFi.mode(WIFI_AP_STA);
          init_WiFi_AP();  // sets ap_active
        }
      }
    }
  }
  ap_button_was_pressed = pressed;
}

// Task to monitor Wi-Fi status and handle reconnections
void wifi_monitor() {
  check_ap_button();  // must always run: emergency-recovery path even when the radio is off
  check_ap_provisioning_window();

  if (!wifi_sta_configured()) {
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
      DEBUG_PRINTF("Wi-Fi not connected(%d), reconnect attempt\n", status);

      // Try WiFi.reconnect() if it was successfully connected at least once
      if (hasConnectedBefore) {
        lastReconnectAttempt = currentMillis;  // Reset reconnection attempt timer
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

bool wifi_connected() {
  return WiFi.status() == WL_CONNECTED;
}

// Function to handle Wi-Fi connection
void connectToWiFi() {
  if (!wifi_sta_configured()) {
    return;
  }

  if (!wifi_connected()) {
    lastReconnectAttempt = millis();  // Reset the reconnect attempt timer
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
  // SSID and BSSID are taken from the event payload. The BSSID identifies which AP we landed on when
  // several of them share the SSID.
  const wifi_event_sta_connected_t& ap = info.wifi_sta_connected;
  DEBUG_PRINTF("Wi-Fi connected(%d), RSSI: %d dBm, SSID: %.*s, BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n", WiFi.status(),
               WiFi.RSSI(), ap.ssid_len, (const char*)ap.ssid, ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3],
               ap.bssid[4], ap.bssid[5]);
  hasConnectedBefore = true;                                            // Mark as successfully connected at least once
  reconnectAttempts = 0;                                                // Reset the attempt counter
  current_full_reconnect_interval = INIT_WIFI_FULL_RECONNECT_INTERVAL;  // Reset the full reconnect interval
  current_check_interval = WIFI_CHECK_INTERVAL;                         // Reset the full reconnect interval
  clear_event(EVENT_WIFI_CONNECT);
}

static void log_ap_sta_event(const char* verb, const uint8_t* mac) {
  logging.printf("AP: %02x:%02x:%02x:%02x:%02x:%02x %s\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], verb);
}

void onApStaConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  log_ap_sta_event("connected", info.wifi_ap_staconnected.mac);
}

void onApStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  log_ap_sta_event("disconnected", info.wifi_ap_stadisconnected.mac);
}

// Event handler for Wi-Fi Got IP
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  //clear disconnects events if we got a IP
  clear_event(EVENT_WIFI_DISCONNECT);

  // One-shot boot notice — fires once per boot, not on every reconnect.
  static bool boot_logged = false;
  if (!boot_logged) {
    boot_logged = true;
    LOG_SET_NEXT_SEVERITY(5);  // RFC 5424 severity 5 = Notice
    logging.printf("Bootup complete, running version %s\n", version_number);
  }

  network_bring_services_up(WiFi.localIP());  // log IP + syslog_start() + init_mDNS()
}

// Event handler for Wi-Fi disconnection
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {

  if (connected_once) {
    set_event(EVENT_WIFI_DISCONNECT, 0);  // also printing a log entry
  }
  //we dont do anything here, the reconnect will be handled by the monitor
  //too many events received when the connection is lost
  //normal reconnect retry start at first 2 seconds
}

void init_WiFi_AP() {

  DEBUG_PRINTF("Creating Wi-Fi AP: %s (password set with %u chars)\n", ssidAP.c_str(), (unsigned)passwordAP.length());

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
    logging.println("Access Point using default password!");
    set_event(EVENT_WIFI_AP_PASSWORD_DEFAULT, 0);
  }
  IPAddress IP = WiFi.softAPIP();

  DEBUG_PRINTF("Access Point IP address: %s\n", IP.toString().c_str());
}
