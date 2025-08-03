#ifndef EMUL_WIFI_H
#define EMUL_WIFI_H

#include <WString.h>
#include <stdint.h>
#include <functional>

#include "IPAddress.h"
#include "NetworkEvents.h"

typedef int arduino_event_info_t;

//#define WiFiEvent_t arduino_event_id_t
#define WiFiEventInfo_t arduino_event_info_t
#define WiFiEventId_t wifi_event_id_t

typedef arduino_event_id_t WiFiEvent_t;

typedef enum {
  WL_NO_SHIELD = 255,  // for compatibility with WiFi Shield library
  WL_STOPPED = 254,
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6
} wl_status_t;

typedef enum {
  WIFI_MODE_NULL = 0, /**< Null mode */
  WIFI_MODE_STA,      /**< Wi-Fi station mode */
  WIFI_MODE_AP,       /**< Wi-Fi soft-AP mode */
  WIFI_MODE_APSTA,    /**< Wi-Fi station + soft-AP mode */
  WIFI_MODE_NAN,      /**< Wi-Fi NAN mode */
  WIFI_MODE_MAX
} wifi_mode_t;

#define WiFiMode_t wifi_mode_t
#define WIFI_OFF WIFI_MODE_NULL
#define WIFI_STA WIFI_MODE_STA
#define WIFI_AP WIFI_MODE_AP
#define WIFI_AP_STA WIFI_MODE_APSTA

typedef int wifi_event_id_t;

struct arduino_event_t {
  arduino_event_id_t event_id;
  arduino_event_info_t event_info;
};

using WiFiEventCb = void (*)(arduino_event_id_t event);
using WiFiEventFuncCb = std::function<void(arduino_event_id_t event, arduino_event_info_t info)>;
using WiFiEventSysCb = void (*)(arduino_event_t* event);

typedef enum {
  WIFI_AUTH_OPEN = 0,     /**< Authenticate mode : open */
  WIFI_AUTH_WEP,          /**< Authenticate mode : WEP */
  WIFI_AUTH_WPA_PSK,      /**< Authenticate mode : WPA_PSK */
  WIFI_AUTH_WPA2_PSK,     /**< Authenticate mode : WPA2_PSK */
  WIFI_AUTH_WPA_WPA2_PSK, /**< Authenticate mode : WPA_WPA2_PSK */
  WIFI_AUTH_ENTERPRISE,   /**< Authenticate mode : Wi-Fi EAP security, treated the same as WIFI_AUTH_WPA2_ENTERPRISE */
  WIFI_AUTH_WPA2_ENTERPRISE = WIFI_AUTH_ENTERPRISE, /**< Authenticate mode : WPA2-Enterprise security */
  WIFI_AUTH_WPA3_PSK,                               /**< Authenticate mode : WPA3_PSK */
  WIFI_AUTH_WPA2_WPA3_PSK,                          /**< Authenticate mode : WPA2_WPA3_PSK */
  WIFI_AUTH_WAPI_PSK,                               /**< Authenticate mode : WAPI_PSK */
  WIFI_AUTH_OWE,                                    /**< Authenticate mode : OWE */
  WIFI_AUTH_WPA3_ENT_192,                           /**< Authenticate mode : WPA3_ENT_SUITE_B_192_BIT */
  WIFI_AUTH_WPA3_EXT_PSK, /**< This authentication mode will yield same result as WIFI_AUTH_WPA3_PSK and not recommended to be used. It will be deprecated in future, please use WIFI_AUTH_WPA3_PSK instead. */
  WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE, /**< This authentication mode will yield same result as WIFI_AUTH_WPA3_PSK and not recommended to be used. It will be deprecated in future, please use WIFI_AUTH_WPA3_PSK instead.*/
  WIFI_AUTH_DPP,                     /**< Authenticate mode : DPP */
  WIFI_AUTH_WPA3_ENTERPRISE,         /**< Authenticate mode : WPA3-Enterprise Only Mode */
  WIFI_AUTH_WPA2_WPA3_ENTERPRISE,    /**< Authenticate mode : WPA3-Enterprise Transition Mode */
  WIFI_AUTH_WPA_ENTERPRISE,          /**< Authenticate mode : WPA-Enterprise security */
  WIFI_AUTH_MAX
} wifi_auth_mode_t;

typedef enum {
  WIFI_CIPHER_TYPE_NONE = 0,    /**< The cipher type is none */
  WIFI_CIPHER_TYPE_WEP40,       /**< The cipher type is WEP40 */
  WIFI_CIPHER_TYPE_WEP104,      /**< The cipher type is WEP104 */
  WIFI_CIPHER_TYPE_TKIP,        /**< The cipher type is TKIP */
  WIFI_CIPHER_TYPE_CCMP,        /**< The cipher type is CCMP */
  WIFI_CIPHER_TYPE_TKIP_CCMP,   /**< The cipher type is TKIP and CCMP */
  WIFI_CIPHER_TYPE_AES_CMAC128, /**< The cipher type is AES-CMAC-128 */
  WIFI_CIPHER_TYPE_SMS4,        /**< The cipher type is SMS4 */
  WIFI_CIPHER_TYPE_GCMP,        /**< The cipher type is GCMP */
  WIFI_CIPHER_TYPE_GCMP256,     /**< The cipher type is GCMP-256 */
  WIFI_CIPHER_TYPE_AES_GMAC128, /**< The cipher type is AES-GMAC-128 */
  WIFI_CIPHER_TYPE_AES_GMAC256, /**< The cipher type is AES-GMAC-256 */
  WIFI_CIPHER_TYPE_UNKNOWN,     /**< The cipher type is unknown */
} wifi_cipher_type_t;

#define WIFI_AP_DEFAULT_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define WIFI_AP_DEFAULT_CIPHER WIFI_CIPHER_TYPE_CCMP

class WiFiClass {
 public:
  wl_status_t begin(const char* ssid, const char* passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL,
                    bool connect = true) {
    return WL_CONNECTED;
  }

  const char* getHostname();

  wl_status_t status() { return WL_CONNECTED; }

  String SSID() const { return String("EmulatedSSID"); }

  int8_t RSSI(void) { return 0; }

  int32_t channel(void) {
    return 1;  // Default channel for emulation
  }

  IPAddress localIP();

  bool setHostname(const char* hostname) { return true; }

  bool mode(wifi_mode_t m) {
    return true;  // Simulate setting WiFi mode
  }

  bool setAutoReconnect(bool autoReconnect) { return true; }

  wifi_event_id_t onEvent(WiFiEventCb cbEvent, arduino_event_id_t event) { return 0; }

  wifi_event_id_t onEvent(WiFiEventFuncCb cbEvent, arduino_event_id_t event) { return 0; }

  wifi_event_id_t onEvent(WiFiEventSysCb cbEvent, arduino_event_id_t event) { return 0; }

  bool softAP(const String& ssid, const String& passphrase = emptyString, int channel = 1, int ssid_hidden = 0,
              int max_connection = 4, bool ftm_responder = false,
              wifi_auth_mode_t auth_mode = WIFI_AP_DEFAULT_AUTH_MODE,
              wifi_cipher_type_t cipher = WIFI_AP_DEFAULT_CIPHER) {
    return true;
  }

  IPAddress softAPIP() {
    return IPAddress(192, 168, 4, 1);  // Default soft AP IP for emulation
  }

  bool disconnect(bool wifioff = false, bool eraseap = false, unsigned long timeoutLength = 100) { return true; }

  bool reconnect() { return true; }

  String macAddress(void) {
    return "00:00:00:00:00:00";  // Emulated MAC address
  }
};

extern WiFiClass WiFi;
#endif
