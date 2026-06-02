// pauLTU3
// CYD network configuration, AP mode and OTA web server.
// Testing-only build.
// This web and OTA path has not been validated on a real system and should be used for development only.

#include "screen_network.h"

#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include <cstdio>

namespace {

// Fixed AP channel used by the screen.
// This project keeps AP on channel 1 to stay compatible with ESP-NOW expectations.
constexpr uint8_t AP_CHANNEL = 1;

// Delay between repeated STA reconnect attempts.
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 20000;

// Common maximum length for persisted text values.
constexpr size_t MAX_STR_VALUE = 64;

// Structure stored in Preferences.
// It contains everything needed to reconnect the screen after reboot.
struct NetworkConfig {
    char ssid[MAX_STR_VALUE] = {0};
    char password[MAX_STR_VALUE] = {0};
    bool use_static_ip = false;
    char local_ip[16] = {0};
    char gateway[16] = {0};
    char subnet[16] = {0};
    char dns1[16] = {0};
    char dns2[16] = {0};
};

// Preferences handle used for NVS storage.
Preferences g_preferences;

// Lightweight HTTP server used for configuration and OTA.
WebServer g_server(80);

// In-memory copy of saved settings.
NetworkConfig g_config;

// Active AP SSID.
char g_ap_ssid[32] = {0};

// Hostname used for mDNS and STA host name.
char g_hostname[32] = {0};

// Reusable helper buffers returned by public getter functions.
char g_sta_ssid[64] = {0};
char g_sta_ip[16] = {0};

// Last time a STA reconnect was started.
uint32_t g_last_wifi_attempt_ms = 0;

// OTA sets this flag so reboot can happen after the browser gets the HTTP response.
bool g_update_reboot_pending = false;
uint32_t g_update_reboot_due_ms = 0;

// Prevent repeated mDNS startup calls.
bool g_mdns_started = false;

// Render a small result page after save/forget/restart/update actions.
// When auto_reconnect is enabled, JavaScript keeps trying the common screen URLs
// until the main page becomes reachable again.
String recovery_page(const __FlashStringHelper *title,
                     const __FlashStringHelper *message,
                     bool auto_reconnect) {
    String page;
    page.reserve(2600);

    page += F("<!doctype html><html><head><meta charset=\"utf-8\">");
    page += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
    page += F("<title>");
    page += title;
    page += F("</title>");
    page += F("<style>"
              "body{font-family:Arial,sans-serif;background:#f4f6f8;color:#111;margin:0;padding:18px;}"
              ".wrap{max-width:680px;margin:48px auto;}"
              ".card{background:#fff;border:1px solid #d8dee6;border-radius:12px;padding:18px;}"
              "a{color:#1565c0;font-weight:700;}code{background:#eef2f5;padding:2px 6px;border-radius:6px;}"
              "</style></head><body><div class=\"wrap\"><div class=\"card\"><h2>");
    page += title;
    page += F("</h2><p>");
    page += message;
    page += F("</p><p><a id=\"backlink\" href=\"/\">Open main page</a></p>");

    if (auto_reconnect) {
        page += F("<script>"
                  "const targets=[window.location.origin,'http://");
        page += g_hostname;
        page += F(".local','http://");
        page += WiFi.softAPIP().toString();
        page += F("'];"
                  "let idx=0;"
                  "function tryNext(){"
                  "const target=targets[idx % targets.length]+'/' ;"
                  "document.getElementById('backlink').href=target;"
                  "fetch(target,{mode:'no-cors'}).then(()=>{window.location.href=target;}).catch(()=>{"
                  "idx++;setTimeout(tryNext,1500);"
                  "});}"
                  "setTimeout(tryNext,1200);"
                  "</script>");
    }

    page += F("</div></div></body></html>");
    return page;
}

// Escape text before inserting it into HTML.
String html_escape(const String &value) {
    String escaped;
    escaped.reserve(value.length() + 16);

    for (size_t i = 0; i < value.length(); ++i) {
        switch (value[i]) {
            case '&':
                escaped += F("&amp;");
                break;
            case '<':
                escaped += F("&lt;");
                break;
            case '>':
                escaped += F("&gt;");
                break;
            case '"':
                escaped += F("&quot;");
                break;
            case '\'':
                escaped += F("&#39;");
                break;
            default:
                escaped += value[i];
                break;
        }
    }

    return escaped;
}

// Read a string from Preferences with a default value.
String prefs_string(const char *key, const char *default_value = "") {
    return g_preferences.getString(key, default_value);
}

// Safely copy an Arduino String into a fixed char buffer.
void copy_string(char *destination, size_t destination_size, const String &source) {
    if (destination_size == 0) {
        return;
    }

    strncpy(destination, source.c_str(), destination_size - 1);
    destination[destination_size - 1] = '\0';
}

// Load saved network settings from NVS.
void load_config() {
    g_preferences.begin("netcfg", true);
    copy_string(g_config.ssid, sizeof(g_config.ssid), prefs_string("ssid"));
    copy_string(g_config.password, sizeof(g_config.password), prefs_string("pass"));
    g_config.use_static_ip = g_preferences.getBool("static", false);
    copy_string(g_config.local_ip, sizeof(g_config.local_ip), prefs_string("ip"));
    copy_string(g_config.gateway, sizeof(g_config.gateway), prefs_string("gw"));
    copy_string(g_config.subnet, sizeof(g_config.subnet), prefs_string("subnet", "255.255.255.0"));
    copy_string(g_config.dns1, sizeof(g_config.dns1), prefs_string("dns1"));
    copy_string(g_config.dns2, sizeof(g_config.dns2), prefs_string("dns2"));
    g_preferences.end();
}

// Save current network settings to NVS.
void save_config() {
    g_preferences.begin("netcfg", false);
    g_preferences.putString("ssid", g_config.ssid);
    g_preferences.putString("pass", g_config.password);
    g_preferences.putBool("static", g_config.use_static_ip);
    g_preferences.putString("ip", g_config.local_ip);
    g_preferences.putString("gw", g_config.gateway);
    g_preferences.putString("subnet", g_config.subnet);
    g_preferences.putString("dns1", g_config.dns1);
    g_preferences.putString("dns2", g_config.dns2);
    g_preferences.end();
}

// Accept an empty IP field and treat it as 0.0.0.0.
bool parse_ip_or_zero(const char *value, IPAddress &target) {
    if (value == nullptr || value[0] == '\0') {
        target = IPAddress(static_cast<uint32_t>(0));
        return true;
    }

    return target.fromString(value);
}

// Apply static IP parameters before starting Wi-Fi.
bool apply_sta_ip_config() {
    if (!g_config.use_static_ip) {
        return true;
    }

    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;

    if (!local_ip.fromString(g_config.local_ip) ||
        !gateway.fromString(g_config.gateway) ||
        !subnet.fromString(g_config.subnet) ||
        !parse_ip_or_zero(g_config.dns1, dns1) ||
        !parse_ip_or_zero(g_config.dns2, dns2)) {
        Serial.println("WiFi static IP config invalid");
        return false;
    }

    return WiFi.config(local_ip, gateway, subnet, dns1, dns2);
}

// Start connecting to the configured external Wi-Fi network.
void begin_sta_connect() {
    if (g_config.ssid[0] == '\0') {
        return;
    }

    // Reset previous STA state first.
    WiFi.disconnect(false, true);
    delay(50);

    // Clear static config when DHCP is selected.
    if (!g_config.use_static_ip) {
        const IPAddress unset_ip(static_cast<uint32_t>(0));
        WiFi.config(unset_ip, unset_ip, unset_ip, unset_ip, unset_ip);
    } else if (!apply_sta_ip_config()) {
        Serial.println("WiFi static IP apply failed");
    }

    // Apply hostname and start the actual STA connection.
    WiFi.setHostname(g_hostname);
    WiFi.begin(g_config.ssid, g_config.password);
    g_last_wifi_attempt_ms = millis();
    Serial.printf("WiFi connect started: ssid=%s\n", g_config.ssid);
}

// Human-readable STA status for the web page.
String station_status_text() {
    if (WiFi.status() == WL_CONNECTED) {
        return F("Connected");
    }
    if (g_config.ssid[0] == '\0') {
        return F("Not configured");
    }
    return F("Disconnected");
}

// Human-readable STA IP string for the web page.
String station_ip_text() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return F("-");
}

// Human-readable STA channel string for the web page.
String station_channel_text() {
    if (WiFi.status() == WL_CONNECTED) {
        return String(WiFi.channel());
    }
    return F("-");
}

// Render one text input control with name, value and placeholder.
String input_value(const char *name, const char *value, const char *placeholder = "") {
    String html;
    html += F("<input name=\"");
    html += name;
    html += F("\" value=\"");
    html += html_escape(value);
    html += F("\" placeholder=\"");
    html += placeholder;
    html += F("\">");
    return html;
}

// Return checked attribute only when the checkbox must be checked.
String checked_attr(bool checked) {
    return checked ? F(" checked") : String();
}

// Main configuration page served by the screen.
void handle_root() {
    String page;
    page.reserve(7000);

    page += F("<!doctype html><html><head><meta charset=\"utf-8\">");
    page += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
    page += F("<title>CYD Screen Network</title>");
    page += F("<style>"
              "body{font-family:Arial,sans-serif;background:#f4f6f8;color:#111;margin:0;padding:18px;}"
              ".wrap{max-width:820px;margin:0 auto;}"
              ".card{background:#fff;border:1px solid #d8dee6;border-radius:12px;padding:16px;margin-bottom:16px;}"
              "h1,h2{margin:0 0 12px 0;}label{display:block;font-weight:700;margin-top:10px;}"
              "input{width:100%;box-sizing:border-box;padding:10px;margin-top:6px;border:1px solid #bcc6d0;border-radius:8px;}"
              "button{border:none;border-radius:10px;padding:12px 16px;font-weight:700;cursor:pointer;margin-top:12px;}"
              ".primary{background:#1e88e5;color:#fff;}.warn{background:#f9a825;color:#111;}.danger{background:#e53935;color:#fff;}"
              ".ok{background:#43a047;color:#fff;}.muted{background:#eceff1;color:#111;}"
              ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;}"
              ".note{background:#fff7d6;border:1px solid #f0d47b;border-radius:10px;padding:12px;}"
              "code{background:#eef2f5;padding:2px 6px;border-radius:6px;}"
              "</style></head><body><div class=\"wrap\">");

    page += F("<div class=\"card\"><h1>CYD Screen Network</h1>");
    page += F("<div class=\"grid\">");
    page += F("<div><strong>AP SSID:</strong> ");
    page += html_escape(g_ap_ssid);
    page += F("</div><div><strong>AP IP:</strong> ");
    page += WiFi.softAPIP().toString();
    page += F("</div><div><strong>STA Status:</strong> ");
    page += station_status_text();
    page += F("</div><div><strong>STA IP:</strong> ");
    page += station_ip_text();
    page += F("</div><div><strong>STA Channel:</strong> ");
    page += station_channel_text();
    page += F("</div><div><strong>Hostname:</strong> ");
    page += html_escape(g_hostname);
    page += F(".local</div></div></div>");

    page += F("<div class=\"note\"><strong>ESP-NOW note:</strong> The screen receiver is fixed to Wi-Fi channel <code>1</code>. ");
    page += F("If you connect the screen to a router on another channel, ESP-NOW traffic may stop working until you move the router to channel 1 or disconnect STA Wi-Fi.</div>");
    page += F("<div class=\"note\" style=\"margin-top:12px;\"><strong>Testing disclaimer:</strong> This CYD integration is a testing version and has not been validated on a real system. Use only for development and bench testing.</div>");

    page += F("<div class=\"card\"><h2>Wi-Fi Credentials</h2><form method=\"post\" action=\"/save\">");
    page += F("<label>SSID</label>");
    page += input_value("ssid", g_config.ssid, "Your Wi-Fi name");
    page += F("<label>Password</label>");
    page += F("<input type=\"password\" name=\"password\" value=\"");
    page += html_escape(g_config.password);
    page += F("\" placeholder=\"Wi-Fi password\">");

    page += F("<h2 style=\"margin-top:18px;\">IP Settings</h2>");
    page += F("<label><input type=\"checkbox\" name=\"use_static\" value=\"1\" style=\"width:auto;margin-right:8px;\"");
    page += checked_attr(g_config.use_static_ip);
    page += F(">Use static IP</label>");
    page += F("<label>Static IP</label>");
    page += input_value("local_ip", g_config.local_ip, "192.168.1.50");
    page += F("<label>Gateway</label>");
    page += input_value("gateway", g_config.gateway, "192.168.1.1");
    page += F("<label>Subnet</label>");
    page += input_value("subnet", g_config.subnet, "255.255.255.0");
    page += F("<label>DNS 1</label>");
    page += input_value("dns1", g_config.dns1, "192.168.1.1");
    page += F("<label>DNS 2</label>");
    page += input_value("dns2", g_config.dns2, "8.8.8.8");
    page += F("<button class=\"primary\" type=\"submit\">Save and Reconnect</button></form>");
    page += F("<form method=\"post\" action=\"/forget\"><button class=\"danger\" type=\"submit\">Forget Wi-Fi</button></form>");
    page += F("<form method=\"post\" action=\"/restart\"><button class=\"warn\" type=\"submit\">Restart Screen</button></form>");
    page += F("</div>");

    page += F("<div class=\"card\"><h2>OTA Update</h2>");
    page += F("<form method=\"post\" action=\"/update\" enctype=\"multipart/form-data\">");
    page += F("<label>Firmware File (.bin)</label><input type=\"file\" name=\"firmware\">");
    page += F("<button class=\"ok\" type=\"submit\">Upload Firmware</button></form>");
    page += F("</div></div></body></html>");

    g_server.send(200, "text/html", page);
}

// Save web form settings and start reconnect logic.
void handle_save() {
    copy_string(g_config.ssid, sizeof(g_config.ssid), g_server.arg("ssid"));
    copy_string(g_config.password, sizeof(g_config.password), g_server.arg("password"));
    g_config.use_static_ip = g_server.hasArg("use_static");
    copy_string(g_config.local_ip, sizeof(g_config.local_ip), g_server.arg("local_ip"));
    copy_string(g_config.gateway, sizeof(g_config.gateway), g_server.arg("gateway"));
    copy_string(g_config.subnet, sizeof(g_config.subnet), g_server.arg("subnet"));
    copy_string(g_config.dns1, sizeof(g_config.dns1), g_server.arg("dns1"));
    copy_string(g_config.dns2, sizeof(g_config.dns2), g_server.arg("dns2"));

    save_config();
    begin_sta_connect();

    g_server.send(200,
                  "text/html",
                  recovery_page(F("Saved"),
                                F("Settings stored and reconnect started. The page will reopen automatically when the screen is reachable again."),
                                true));
}

// Delete saved credentials and go back to AP-only mode.
void handle_forget() {
    memset(&g_config, 0, sizeof(g_config));
    save_config();
    WiFi.disconnect(false, true);
    g_server.send(200,
                  "text/html",
                  recovery_page(F("Wi-Fi removed"),
                                F("Saved credentials were cleared. The page will reopen on the AP address."),
                                true));
}

// Manual reboot endpoint from the web page.
void handle_restart() {
    g_server.send(200,
                  "text/html",
                  recovery_page(F("Restarting"),
                                F("The screen is rebooting now. The page will reopen automatically when the web server is back."),
                                true));
    delay(300);
    ESP.restart();
}

// Final OTA page shown after upload finishes.
void handle_update_result() {
    const bool ok = !Update.hasError();
    g_server.send(200,
                  "text/html",
                  ok ? recovery_page(F("Update complete"),
                                     F("Firmware upload finished. The screen will reboot and this page will reopen automatically."),
                                     true)
                     : recovery_page(F("Update failed"),
                                     F("Firmware upload failed. Use the link below to return to the main page."),
                                     false));
    if (ok) {
        g_update_reboot_pending = true;
        g_update_reboot_due_ms = millis() + 1000;
    }
}

// Streaming handler called by WebServer during OTA file upload.
void handle_update_upload() {
    HTTPUpload &upload = g_server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA upload start: %s\n", upload.filename.c_str());

        // Start OTA without knowing file size in advance.
        Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Write the current uploaded chunk into flash.
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        // Finalize the image and verify it.
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.printf("OTA upload complete: %u bytes\n", upload.totalSize);
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        // Abort and clean up if browser upload was canceled.
        Update.abort();
        Serial.println("OTA upload aborted");
    }
}

// Start mDNS only after STA Wi-Fi is up.
void setup_mdns() {
    if (WiFi.status() != WL_CONNECTED || g_mdns_started) {
        return;
    }

    if (MDNS.begin(g_hostname)) {
        MDNS.addService("http", "tcp", 80);
        g_mdns_started = true;
        Serial.printf("mDNS started: http://%s.local\n", g_hostname);
    }
}

// Register all routes and start the HTTP server.
void setup_server() {
    g_server.on("/", HTTP_GET, handle_root);
    g_server.on("/save", HTTP_POST, handle_save);
    g_server.on("/forget", HTTP_POST, handle_forget);
    g_server.on("/restart", HTTP_POST, handle_restart);
    g_server.on("/update", HTTP_POST, handle_update_result, handle_update_upload);
    g_server.begin();
}

}  // namespace

void screen_network_init() {
    // Fixed names used by this project.
    snprintf(g_ap_ssid, sizeof(g_ap_ssid), "BatteryEmulator-CYD");
    snprintf(g_hostname, sizeof(g_hostname), "batteryemulator-cyd");

    // Load saved settings before touching Wi-Fi.
    load_config();

    // Keep Wi-Fi persistence under our own Preferences storage.
    WiFi.persistent(false);
    WiFi.setSleep(false);

    // AP+STA lets the screen host its own setup AP and still join normal Wi-Fi.
    WiFi.mode(WIFI_AP_STA);

    // Start AP immediately so the user can always reach the device.
    WiFi.softAP(g_ap_ssid, nullptr, AP_CHANNEL, false, 4);
    WiFi.setHostname(g_hostname);

    Serial.printf("Screen AP ready: ssid=%s ip=%s channel=%u\n",
                  g_ap_ssid,
                  WiFi.softAPIP().toString().c_str(),
                  static_cast<unsigned>(AP_CHANNEL));

    // Try STA Wi-Fi too when credentials already exist.
    if (g_config.ssid[0] != '\0') {
        begin_sta_connect();
    }

    // Start web configuration server.
    setup_server();
}

void screen_network_update() {
    // Handle one pending HTTP client request.
    g_server.handleClient();

    // Start mDNS once a real STA connection exists.
    if (WiFi.status() == WL_CONNECTED) {
        setup_mdns();
    } else if (g_config.ssid[0] != '\0' && (millis() - g_last_wifi_attempt_ms) >= WIFI_RETRY_INTERVAL_MS) {
        // Retry Wi-Fi every few seconds when credentials exist.
        begin_sta_connect();
    }

    // Reboot after OTA once the browser already received the completion page.
    if (g_update_reboot_pending && millis() >= g_update_reboot_due_ms) {
        ESP.restart();
    }
}

bool screen_network_is_sta_connected() {
    // Small wrapper used by other modules.
    return WiFi.status() == WL_CONNECTED;
}

int screen_network_sta_channel() {
    return screen_network_is_sta_connected() ? WiFi.channel() : -1;
}

const char *screen_network_sta_ssid() {
    if (!screen_network_is_sta_connected()) {
        return nullptr;
    }

    // Copy String result to a stable buffer before returning.
    copy_string(g_sta_ssid, sizeof(g_sta_ssid), WiFi.SSID());
    return g_sta_ssid;
}

const char *screen_network_sta_ip() {
    if (!screen_network_is_sta_connected()) {
        return nullptr;
    }

    // Copy String result to a stable buffer before returning.
    copy_string(g_sta_ip, sizeof(g_sta_ip), WiFi.localIP().toString());
    return g_sta_ip;
}

int screen_network_sta_rssi() {
    return screen_network_is_sta_connected() ? WiFi.RSSI() : 0;
}

const char *screen_network_ap_ssid() {
    return g_ap_ssid;
}

const char *screen_network_hostname() {
    return g_hostname;
}
