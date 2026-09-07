#include "modbus_gateway.h"

// Built in on boards with enough flash; compiled out on 4 MB / SMALL_FLASH_DEVICE
// boards so it costs them nothing. Activation is still runtime-gated (enabled in
// settings + RS485 present + inverter link is CAN).
#ifndef SMALL_FLASH_DEVICE

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>
#include <mbedtls/md.h>

#include <vector>

#include "../../communication/nvm/comm_nvm.h"  // BatteryEmulatorSettingsStore
#include "../../devboard/hal/hal.h"
#include "../../inverter/InverterProtocol.h"  // extern InverterProtocol* inverter; InverterInterfaceType
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/eModbus-eModbus/ModbusClientRTU.h"
#include "../../lib/eModbus-eModbus/ModbusServerWiFi.h"
#include "../rs485/comm_rs485.h"

using namespace Modbus;  // FunctionCode / Error enums (READ_HOLD_REGISTER, ...)

// The eModbus objects are constructed with a compile-time DE pin. Mirror
// ModbusInverterProtocol: pass -1 and let rs485_begin()'s UART half-duplex mode
// (boards with a real DE pin) or the auto-direction transceiver (LilyGo, no DE
// pin) drive the bus direction. Do NOT let eModbus toggle a GPIO here.
#ifndef RS485_DE_PIN
#define RS485_DE_PIN -1
#endif

// ---- Fixed tuning (not user-configurable) -----------------------------------
static constexpr uint8_t GW_MAX_CLIENTS = 1;       // only the platform poller
static constexpr uint32_t GW_TCP_TIMEOUT = 10000;  // ms idle before drop
static constexpr uint32_t GW_RTU_TIMEOUT = 1000;   // ms per RTU transaction

// ---- Persistent config (NVS namespace "mbgw"), edited via the web UI ---------
// Secure by default: the gateway is OFF and writes are DISABLED until an admin
// enables them. The HMAC secret is generated randomly on first boot.
struct GwConfig {
  bool enable = false;         // master enable for the whole gateway
  bool allow_write = false;    // permit the authenticated write endpoint
  uint16_t tcp_port = 502;     // Modbus-TCP listen port
  uint32_t baud = 9600;        // RS485 baud (GoodWe 247 / DEYE 1 are both 9600 8N1)
  String slave_ids = "247,1";  // CSV of RTU slave IDs exposed for reads
  String secret;               // HMAC secret (random hex, generated if empty)
  String ip_allow;             // CSV of client IPs allowed to WRITE; empty = allow all
};
static GwConfig g_cfg;

static ModbusClientRTU* g_master = nullptr;
static ModbusServerWiFi g_tcp;
static bool g_master_ready = false;  // RTU master + RS485 up, workers registered
static bool g_tcp_started = false;   // TCP server bound (only once WiFi is connected)

static String random_hex(int bytes) {
  static const char* hx = "0123456789abcdef";
  String s;
  s.reserve(bytes * 2);
  for (int i = 0; i < bytes; i++) {
    uint8_t b = static_cast<uint8_t>(esp_random());
    s += hx[b >> 4];
    s += hx[b & 0x0F];
  }
  return s;
}

// Config lives in the emulator's own settings store (BatteryEmulatorSettingsStore,
// keys GW*) so it is edited from the standard /settings page, persisted by
// /saveSettings, and wiped by factory reset — like every other setting. Changes
// take effect on reboot (same as the rest of the settings page). Keys:
//   GWENAB(bool) GWWRITE(bool) GWPORT(uint) GWBAUD(uint)
//   GWSLAVES(str) GWSECRET(str) GWIPALLOW(str)
static void cfg_load() {
  BatteryEmulatorSettingsStore s;
  g_cfg.enable = s.getBool("GWENAB", false);
  g_cfg.allow_write = s.getBool("GWWRITE", false);
  g_cfg.tcp_port = (uint16_t)s.getUInt("GWPORT", 502);
  g_cfg.baud = s.getUInt("GWBAUD", 9600);
  g_cfg.slave_ids = s.getString("GWSLAVES", "247,1");
  g_cfg.ip_allow = s.getString("GWIPALLOW", "");
  g_cfg.secret = s.getString("GWSECRET", "");
  if (g_cfg.secret.length() == 0) {
    g_cfg.secret = random_hex(16);  // 32 hex chars, generated once on first boot
    s.saveString("GWSECRET", g_cfg.secret.c_str());
  }
}

// Parse a CSV like "247, 1" into a small list of slave IDs.
static std::vector<uint8_t> parse_slave_ids(const String& csv) {
  std::vector<uint8_t> ids;
  int start = 0;
  while (start < (int)csv.length()) {
    int comma = csv.indexOf(',', start);
    if (comma < 0) comma = csv.length();
    String tok = csv.substring(start, comma);
    tok.trim();
    if (tok.length()) {
      int v = tok.toInt();
      if (v >= 1 && v <= 247) ids.push_back((uint8_t)v);
    }
    start = comma + 1;
  }
  return ids;
}

static bool ip_allowed(const String& ip) {
  if (g_cfg.ip_allow.length() == 0) return true;  // empty allowlist = allow all
  String csv = g_cfg.ip_allow;
  csv.replace(" ", "");
  return ("," + csv + ",").indexOf("," + ip + ",") >= 0;
}

// ---- In-RAM audit ring for write attempts -----------------------------------
struct AuditEntry {
  uint32_t ms = 0;
  String ip;
  uint8_t id = 0;
  uint16_t addr = 0;
  uint16_t val = 0;
  int code = 0;  // HTTP status of the attempt
};
static const int GW_AUDIT_N = 16;
static AuditEntry g_audit[GW_AUDIT_N];
static int g_audit_head = 0;

static void audit_add(const String& ip, uint8_t id, uint16_t addr, uint16_t val, int code) {
  g_audit[g_audit_head] = AuditEntry{millis(), ip, id, addr, val, code};
  g_audit_head = (g_audit_head + 1) % GW_AUDIT_N;
}

// Transparent read worker: forward the TCP request verbatim to the RTU master
// and return its response (or a device-failure error if the master is absent).
static ModbusMessage gw_forward(ModbusMessage request) {
  static uint32_t token = 0;
  if (!g_master) {
    ModbusMessage err;
    err.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
    return err;
  }
  return g_master->syncRequest(request, token++);
}

void modbus_gateway_init() {
  if (g_master_ready) {
    return;
  }

  cfg_load();  // load config (and generate the HMAC secret on first boot)

  if (!g_cfg.enable) {
    return;  // secure default: gateway stays off until enabled in the web UI
  }

  // Only boards that actually have an RS485 interface can host the gateway.
  // The base HAL returns GPIO_NUM_NC for RS485 pins; boards with RS485 override it.
  if (esp32hal->RS485_RX_PIN() == GPIO_NUM_NC || esp32hal->RS485_TX_PIN() == GPIO_NUM_NC) {
    return;  // no RS485 on this board — feature unsupported
  }

  // Free-bus gate: only claim RS485 when the inverter link is NOT RS485/Modbus.
  // A CAN inverter link (or no inverter selected) leaves RS485 idle and ours.
  if (inverter && inverter->interface_type() != InverterInterfaceType::Can) {
    return;  // RS485 busy serving the battery/inverter Modbus role; do not contend.
  }

  // Open UART2 for RS485 the same way every RS485 user does (sets pins and, on
  // boards with a DE pin, UART half-duplex mode).
  if (!rs485_begin("ModbusGateway", Serial2, g_cfg.baud, SERIAL_8N1)) {
    return;
  }

  g_master = new ModbusClientRTU(RS485_DE_PIN);
  g_master->setTimeout(GW_RTU_TIMEOUT);
  g_master->begin(Serial2, esp32hal->MODBUS_CORE());

  // Read-only over TCP: register only read function codes. Write FCs are
  // deliberately left unregistered, so the server answers ILLEGAL_FUNCTION to
  // any write over the raw TCP port. Writes go through the HMAC endpoint only.
  for (uint8_t id : parse_slave_ids(g_cfg.slave_ids)) {
    g_tcp.registerWorker(id, READ_HOLD_REGISTER, &gw_forward);
    g_tcp.registerWorker(id, READ_INPUT_REGISTER, &gw_forward);
  }

  // The TCP server is NOT started here: WiFi is brought up asynchronously by the
  // connectivity task and is not connected yet at setup() time. It is started
  // from modbus_gateway_loop() once WiFi reports connected.
  g_master_ready = true;
}

void modbus_gateway_loop() {
  // Bind the Modbus-TCP server once, after WiFi is up (WiFiServer needs an IP).
  if (g_master_ready && !g_tcp_started && WiFi.status() == WL_CONNECTED) {
    g_tcp.start(g_cfg.tcp_port, GW_MAX_CLIENTS, GW_TCP_TIMEOUT, esp32hal->MODBUS_CORE());
    g_tcp_started = true;
  }
}

// ---- Authenticated write path (HTTP + HMAC-SHA256, never over raw :502) ------

static String hmac_sha256_hex(const String& key, const String& msg) {
  uint8_t out[32];
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_hmac(info, reinterpret_cast<const uint8_t*>(key.c_str()), key.length(),
                  reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length(), out);
  static const char* hx = "0123456789abcdef";
  String s;
  s.reserve(64);
  for (int i = 0; i < 32; i++) {
    s += hx[out[i] >> 4];
    s += hx[out[i] & 0x0F];
  }
  return s;
}

// Constant-time-ish string compare (avoid leaking match length via early exit).
static bool ct_equal(const String& a, const String& b) {
  if (a.length() != b.length()) {
    return false;
  }
  uint8_t diff = 0;
  for (size_t i = 0; i < a.length(); i++) {
    diff |= static_cast<uint8_t>(a[i] ^ b[i]);
  }
  return diff == 0;
}

static String json_escape(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      o += '\\';
      o += c;
    } else if (c >= 0x20) {
      o += c;
    }
  }
  return o;
}

void modbus_gateway_register_routes(AsyncWebServer& server) {
  // ---- POST /modbus/write?id=&addr=&val=&nonce=&sig= --------------------------
  // sig = lowercase hex HMAC-SHA256(secret, "id:addr:val:nonce").
  // Anti-replay: nonce must strictly exceed the last accepted nonce (kept in NVS).
  server.on("/modbus/write", HTTP_POST, [](AsyncWebServerRequest* request) {
    auto getp = [&](const char* n) -> String {
      if (request->hasParam(n, true)) return request->getParam(n, true)->value();
      if (request->hasParam(n)) return request->getParam(n)->value();
      return String();
    };
    auto has = [&](const char* n) -> bool { return request->hasParam(n, true) || request->hasParam(n); };
    String ip = request->client() ? request->client()->remoteIP().toString() : String("?");
    auto deny = [&](int code, const char* txt, uint8_t id, uint16_t addr, uint16_t val) {
      audit_add(ip, id, addr, val, code);
      request->send(code, "text/plain", txt);
    };

    if (!g_cfg.allow_write) {
      deny(403, "writes disabled", 0, 0, 0);
      return;
    }
    if (!ip_allowed(ip)) {
      deny(403, "ip not allowed", 0, 0, 0);
      return;
    }
    if (!(has("id") && has("addr") && has("val") && has("nonce") && has("sig"))) {
      deny(400, "missing params", 0, 0, 0);
      return;
    }
    uint8_t id = static_cast<uint8_t>(strtoul(getp("id").c_str(), nullptr, 10));
    uint16_t addr = static_cast<uint16_t>(strtoul(getp("addr").c_str(), nullptr, 10));
    uint16_t val = static_cast<uint16_t>(strtoul(getp("val").c_str(), nullptr, 10));
    uint32_t nonce = strtoul(getp("nonce").c_str(), nullptr, 10);
    String sig = getp("sig");
    sig.toLowerCase();

    if (!g_master) {
      deny(503, "master not ready", id, addr, val);
      return;
    }

    Preferences prefs;
    prefs.begin("mbgw", false);
    uint32_t last_nonce = prefs.getULong("nonce", 0);
    if (nonce <= last_nonce) {
      prefs.end();
      deny(409, "stale nonce", id, addr, val);
      return;
    }
    String msg = String(id) + ":" + String(addr) + ":" + String(val) + ":" + String(nonce);
    if (!ct_equal(hmac_sha256_hex(g_cfg.secret, msg), sig)) {
      prefs.end();
      deny(401, "bad signature", id, addr, val);
      return;
    }
    prefs.putULong("nonce", nonce);  // burn the nonce only after auth succeeds
    prefs.end();

    // FC06 write single holding register through the RTU master.
    ModbusMessage resp = g_master->syncRequest(nonce, id, WRITE_HOLD_REGISTER, addr, val);
    Error err = resp.getError();
    if (err != SUCCESS) {
      deny(502, (String("rtu error 0x") + String(err, HEX)).c_str(), id, addr, val);
      return;
    }
    audit_add(ip, id, addr, val, 200);
    request->send(200, "text/plain", "OK");
  });

  // Config (enable / port / baud / slave IDs / secret / IP-allowlist / allow-write)
  // lives in the standard /settings page (BatteryEmulatorSettingsStore, GW* keys) —
  // no separate config page here.

  // ---- GET /modbus/audit : last write attempts as JSON ------------------------
  server.on("/modbus/audit", HTTP_GET, [](AsyncWebServerRequest* request) {
    String j = "[";
    bool first = true;
    for (int k = 0; k < GW_AUDIT_N; k++) {
      int idx = (g_audit_head - 1 - k + 2 * GW_AUDIT_N) % GW_AUDIT_N;  // newest first
      const AuditEntry& e = g_audit[idx];
      if (e.ms == 0) continue;
      if (!first) j += ",";
      first = false;
      j += "{\"ms\":" + String(e.ms) + ",\"ip\":\"" + json_escape(e.ip) + "\",\"id\":" + String(e.id) +
           ",\"addr\":" + String(e.addr) + ",\"val\":" + String(e.val) + ",\"code\":" + String(e.code) + "}";
    }
    j += "]";
    request->send(200, "application/json", j);
  });
}

#else  // SMALL_FLASH_DEVICE — feature compiled out entirely.

class AsyncWebServer;
void modbus_gateway_init() {}
void modbus_gateway_loop() {}
void modbus_gateway_register_routes(AsyncWebServer&) {}

#endif  // !SMALL_FLASH_DEVICE
