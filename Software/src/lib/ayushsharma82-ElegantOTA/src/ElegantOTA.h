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
#include "elop.h"

#include "Update.h"
#include "StreamString.h"
#include "../../ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#define ELEGANTOTA_WEBSERVER AsyncWebServer

enum OTA_Mode {
    OTA_MODE_FIRMWARE = 0,
    OTA_MODE_FILESYSTEM = 1
};

class ElegantOTAClass{
  public:
    ElegantOTAClass();

    void begin(ELEGANTOTA_WEBSERVER *server);

    void loop();

    void onStart(std::function<void()> callable);
    void onProgress(std::function<void(size_t current, size_t final)> callable);
    void onEnd(std::function<void(bool success)> callable);
    
  private:
    ELEGANTOTA_WEBSERVER *_server;
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
