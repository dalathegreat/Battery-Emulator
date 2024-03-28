/**
_____ _                        _    ___ _____  _    
| ____| | ___  __ _  __ _ _ __ | |_ / _ \_   _|/ \   
|  _| | |/ _ \/ _` |/ _` | '_ \| __| | | || | / _ \  
| |___| |  __/ (_| | (_| | | | | |_| |_| || |/ ___ \ 
|_____|_|\___|\__, |\__,_|_| |_|\__|\___/ |_/_/   \_\
              |___/                                  
*/

/**
 * 
 * @name ElegantOTA
 * @author Ayush Sharma (ayush@softt.io)
 * @brief 
 * @version 3.0.0
 * @date 2023-08-30
 */

#ifndef ElegantOTA_h
#define ElegantOTA_h

#include "Arduino.h"
#include "stdlib_noniso.h"
#include "elop.h"

#ifndef ELEGANTOTA_USE_ASYNC_WEBSERVER
  #define ELEGANTOTA_USE_ASYNC_WEBSERVER 0
#endif

#ifndef ELEGANTOTA_DEBUG
  #define ELEGANTOTA_DEBUG 0
#endif

#ifndef UPDATE_DEBUG
  #define UPDATE_DEBUG 0
#endif

#if ELEGANTOTA_DEBUG
  #define ELEGANTOTA_DEBUG_MSG(x) Serial.printf("%s %s", "[ElegantOTA] ", x)
#else
  #define ELEGANTOTA_DEBUG_MSG(x)
#endif

#if defined(ESP8266)
  #include <functional>
  #include "FS.h"
  #include "LittleFS.h"
  #include "Updater.h"
  #include "StreamString.h"
  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    #include "ESPAsyncTCP.h"
    #include "ESPAsyncWebServer.h"
    #define ELEGANTOTA_WEBSERVER AsyncWebServer
  #else
    #include "ESP8266WiFi.h"
    #include "WiFiClient.h"
    #include "ESP8266WebServer.h"
    #define ELEGANTOTA_WEBSERVER ESP8266WebServer
  #endif
  #define HARDWARE "ESP8266"
#elif defined(ESP32)
  #include <functional>
  #include "FS.h"
  #include "Update.h"
  #include "StreamString.h"
  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    #include "AsyncTCP.h"
    #include "ESPAsyncWebServer.h"
    #define ELEGANTOTA_WEBSERVER AsyncWebServer
  #else
    #include "WiFi.h"
    #include "WiFiClient.h"
    #include "WebServer.h"
    #define ELEGANTOTA_WEBSERVER WebServer
  #endif
  #define HARDWARE "ESP32"
#elif defined(TARGET_RP2040)
  #include <functional>
  #include "Arduino.h"
  #include "FS.h"
  #include "LittleFS.h"
  #include "WiFiClient.h"
  #include "WiFiServer.h"
  #include "WebServer.h"
  #include "WiFiUdp.h"
  #include "StreamString.h"
  #include "Updater.h"
  #define HARDWARE              "RP2040"
  #define ELEGANTOTA_WEBSERVER  WebServer
  // Throw an error if async mode is enabled
  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    #error "Async mode is not supported on RP2040. Please set ELEGANTOTA_USE_ASYNC_WEBSERVER to 0."
  #endif
  extern uint8_t _FS_start;
  extern uint8_t _FS_end;
#endif

enum OTA_Mode {
    OTA_MODE_FIRMWARE = 0,
    OTA_MODE_FILESYSTEM = 1
};

class ElegantOTAClass{
  public:
    ElegantOTAClass();

    void begin(ELEGANTOTA_WEBSERVER *server, const char * username = "", const char * password = "");

    void setAuth(const char * username, const char * password);
    void clearAuth();
    void setAutoReboot(bool enable);
    void loop();

    void onStart(std::function<void()> callable);
    void onProgress(std::function<void(size_t current, size_t final)> callable);
    void onEnd(std::function<void(bool success)> callable);
    
  private:
    ELEGANTOTA_WEBSERVER *_server;

    bool _authenticate;
    char _username[64];
    char _password[64];

    bool _auto_reboot = true;
    bool _reboot = false;
    unsigned long _reboot_request_millis = 0;

    String _update_error_str = "";
    unsigned long _current_progress_size;

    std::function<void()> preUpdateCallback = NULL;
    std::function<void(size_t current, size_t final)> progressUpdateCallback = NULL;
    std::function<void(bool success)> postUpdateCallback = NULL;
};

extern ElegantOTAClass ElegantOTA;
#endif
