#include "ElegantOTA.h"

ElegantOTAClass::ElegantOTAClass(){}

void ElegantOTAClass::begin(ELEGANTOTA_WEBSERVER *server, const char * username, const char * password){
  _server = server;

  setAuth(username, password);

  #if defined(TARGET_RP2040)
    if (!__isPicoW) {
      ELEGANTOTA_DEBUG_MSG("RP2040: Not a Pico W, skipping OTA setup\n");
      return;
    }
  #endif

  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    _server->on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
      if(_authenticate && !request->authenticate(_username, _password)){
        return request->requestAuthentication();
      }
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", ELEGANT_HTML, sizeof(ELEGANT_HTML));
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });
  #else
    _server->on("/update", HTTP_GET, [&](){
      if (_authenticate && !_server->authenticate(_username, _password)) {
        return _server->requestAuthentication();
      }
      _server->sendHeader("Content-Encoding", "gzip");
      _server->send_P(200, "text/html", (const char*)ELEGANT_HTML, sizeof(ELEGANT_HTML));
    });
  #endif

  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    _server->on("/ota/start", HTTP_GET, [&](AsyncWebServerRequest *request) {
      if (_authenticate && !request->authenticate(_username, _password)) {
        return request->requestAuthentication();
      }

      // Get header x-ota-mode value, if present
      OTA_Mode mode = OTA_MODE_FIRMWARE;
      // Get mode from arg
      if (request->hasParam("mode")) {
        String argValue = request->getParam("mode")->value();
        if (argValue == "fs") {
          ELEGANTOTA_DEBUG_MSG("OTA Mode: Filesystem\n");
          mode = OTA_MODE_FILESYSTEM;
        } else {
          ELEGANTOTA_DEBUG_MSG("OTA Mode: Firmware\n");
          mode = OTA_MODE_FIRMWARE;
        }
      }

      // Get file MD5 hash from arg
      if (request->hasParam("hash")) {
        String hash = request->getParam("hash")->value();
        ELEGANTOTA_DEBUG_MSG(String("MD5: "+hash+"\n").c_str());
        if (!Update.setMD5(hash.c_str())) {
          ELEGANTOTA_DEBUG_MSG("ERROR: MD5 hash not valid\n");
          return request->send(400, "text/plain", "MD5 parameter invalid");
        }
      }

      #if UPDATE_DEBUG == 1
        // Serial output must be active to see the callback serial prints
        Serial.setDebugOutput(true);
      #endif

      // Pre-OTA update callback
      if (preUpdateCallback != NULL) preUpdateCallback();

      // Start update process
      #if defined(ESP8266)
        uint32_t update_size = mode == OTA_MODE_FILESYSTEM ? ((size_t)FS_end - (size_t)FS_start) : ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
        if (mode == OTA_MODE_FILESYSTEM) {
          close_all_fs();
        }
        Update.runAsync(true);
        if (!Update.begin(update_size, mode == OTA_MODE_FILESYSTEM ? U_FS : U_FLASH)) {
          ELEGANTOTA_DEBUG_MSG("Failed to start update process\n");
          // Save error to string
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _update_error_str += "\n";
          ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
        }
      #elif defined(ESP32)  
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, mode == OTA_MODE_FILESYSTEM ? U_SPIFFS : U_FLASH)) {
          ELEGANTOTA_DEBUG_MSG("Failed to start update process\n");
          // Save error to string
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _update_error_str += "\n";
          ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
        }        
      #endif

      return request->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
    });
  #else
    _server->on("/ota/start", HTTP_GET, [&]() {
      if (_authenticate && !_server->authenticate(_username, _password)) {
        return _server->requestAuthentication();
      }

      // Get header x-ota-mode value, if present
      OTA_Mode mode = OTA_MODE_FIRMWARE;
      // Get mode from arg
      if (_server->hasArg("mode")) {
        String argValue = _server->arg("mode");
        if (argValue == "fs") {
          ELEGANTOTA_DEBUG_MSG("OTA Mode: Filesystem\n");
          mode = OTA_MODE_FILESYSTEM;
        } else {
          ELEGANTOTA_DEBUG_MSG("OTA Mode: Firmware\n");
          mode = OTA_MODE_FIRMWARE;
        }
      }

      // Get file MD5 hash from arg
      if (_server->hasArg("hash")) {
        String hash = _server->arg("hash");
        ELEGANTOTA_DEBUG_MSG(String("MD5: "+hash+"\n").c_str());
        if (!Update.setMD5(hash.c_str())) {
          ELEGANTOTA_DEBUG_MSG("ERROR: MD5 hash not valid\n");
          return _server->send(400, "text/plain", "MD5 parameter invalid");
        }
      }

      #if UPDATE_DEBUG == 1
        // Serial output must be active to see the callback serial prints
        Serial.setDebugOutput(true);
      #endif

      // Pre-OTA update callback
      if (preUpdateCallback != NULL) preUpdateCallback();

      // Start update process
      #if defined(ESP8266)
        uint32_t update_size = mode == OTA_MODE_FILESYSTEM ? ((size_t)FS_end - (size_t)FS_start) : ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
        if (mode == OTA_MODE_FILESYSTEM) {
          close_all_fs();
        }
        Update.runAsync(true);
        if (!Update.begin(update_size, mode == OTA_MODE_FILESYSTEM ? U_FS : U_FLASH)) {
          ELEGANTOTA_DEBUG_MSG("Failed to start update process\n");
          // Save error to string
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _update_error_str += "\n";
          ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
        }
      #elif defined(ESP32)  
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, mode == OTA_MODE_FILESYSTEM ? U_SPIFFS : U_FLASH)) {
          ELEGANTOTA_DEBUG_MSG("Failed to start update process\n");
          // Save error to string
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _update_error_str += "\n";
          ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
        }
      #elif defined(TARGET_RP2040)
        uint32_t update_size = 0;
        // Gather FS Size
        if (mode == OTA_MODE_FILESYSTEM) {
          update_size = ((size_t)&_FS_end - (size_t)&_FS_start);
          LittleFS.end();
        } else {
          FSInfo64 i;
          LittleFS.begin();
          LittleFS.info64(i);
          update_size = i.totalBytes - i.usedBytes;
        }
        // Start update process
        if (!Update.begin(update_size, mode == OTA_MODE_FILESYSTEM ? U_FS : U_FLASH)) {
          ELEGANTOTA_DEBUG_MSG("Failed to start update process because there is not enough space\n");
          _update_error_str = "Not enough space";
          return _server->send(400, "text/plain", _update_error_str.c_str());
        }
      #endif

      return _server->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
    });
  #endif

  #if ELEGANTOTA_USE_ASYNC_WEBSERVER == 1
    _server->on("/ota/upload", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if(_authenticate && !request->authenticate(_username, _password)){
          return request->requestAuthentication();
        }
        // Post-OTA update callback
        if (postUpdateCallback != NULL) postUpdateCallback(!Update.hasError());
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        // Set reboot flag
        if (!Update.hasError()) {
          if (_auto_reboot) {
            _reboot_request_millis = millis();
            _reboot = true;
          }
        }
    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        if(_authenticate){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }

        if (!index) {
          // Reset progress size on first frame
          _current_progress_size = 0;
        }

        // Write chunked data to the free sketch space
        if(len){
            if (Update.write(data, len) != len) {
                return request->send(400, "text/plain", "Failed to write chunked data to free space");
            }
            _current_progress_size += len;
            // Progress update callback
            if (progressUpdateCallback != NULL) progressUpdateCallback(_current_progress_size, request->contentLength());
        }
            
        if (final) { // if the final flag is set then this is the last frame of data
            if (!Update.end(true)) { //true to set the size to the current progress
                // Save error to string
                StreamString str;
                Update.printError(str);
                _update_error_str = str.c_str();
                _update_error_str += "\n";
                ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
            }
        }else{
            return;
        }
    });
  #else
    _server->on("/ota/upload", HTTP_POST, [&](){
      if (_authenticate && !_server->authenticate(_username, _password)) {
        return _server->requestAuthentication();
      }
      // Post-OTA update callback
      if (postUpdateCallback != NULL) postUpdateCallback(!Update.hasError());
      _server->sendHeader("Connection", "close");
      _server->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
      // Set reboot flag
      if (!Update.hasError()) {
        if (_auto_reboot) {
          _reboot_request_millis = millis();
          _reboot = true;
        }
      }
    }, [&](){
      // Actual OTA Download
      HTTPUpload& upload = _server->upload();
      if (upload.status == UPLOAD_FILE_START) {
        // Check authentication
        if (_authenticate && !_server->authenticate(_username, _password)) {
          ELEGANTOTA_DEBUG_MSG("Authentication Failed on UPLOAD_FILE_START\n");
          return;
        }
        Serial.printf("Update Received: %s\n", upload.filename.c_str());
        _current_progress_size = 0;
      } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            #if UPDATE_DEBUG == 1
              Update.printError(Serial);
            #endif
          }

          _current_progress_size += upload.currentSize;
          // Progress update callback
          if (progressUpdateCallback != NULL) progressUpdateCallback(_current_progress_size, upload.totalSize);
      } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
              ELEGANTOTA_DEBUG_MSG(String("Update Success: "+String(upload.totalSize)+"\n").c_str());
          } else {
              ELEGANTOTA_DEBUG_MSG("[!] Update Failed\n");
              // Store error to string
              StreamString str;
              Update.printError(str);
              _update_error_str = str.c_str();
              _update_error_str += "\n";
              ELEGANTOTA_DEBUG_MSG(_update_error_str.c_str());
          }

          #if UPDATE_DEBUG == 1
            Serial.setDebugOutput(false);
          #endif
      } else {
        ELEGANTOTA_DEBUG_MSG(String("Update Failed Unexpectedly (likely broken connection): status="+String(upload.status)+"\n").c_str());
      }
    });
  #endif
}

void ElegantOTAClass::setAuth(const char * username, const char * password){
  if (strlen(username) > 0 && strlen(password) > 0) {
    strlcpy(_username, username, sizeof(_username));
    strlcpy(_password, password, sizeof(_password));
    _authenticate = true;
  }
}

void ElegantOTAClass::clearAuth(){
  _authenticate = false;
}

void ElegantOTAClass::setAutoReboot(bool enable){
  _auto_reboot = enable;
}

void ElegantOTAClass::loop() {
  // Check if 2 seconds have passed since _reboot_request_millis was set
  if (_reboot && millis() - _reboot_request_millis > 2000) {
    ELEGANTOTA_DEBUG_MSG("Rebooting...\n");
    #if defined(ESP8266) || defined(ESP32)
      ESP.restart();
    #elif defined(TARGET_RP2040)
      rp2040.reboot();
    #endif
    _reboot = false;
  }
}

void ElegantOTAClass::onStart(std::function<void()> callable){
    preUpdateCallback = callable;
}

void ElegantOTAClass::onProgress(std::function<void(size_t current, size_t final)> callable){
    progressUpdateCallback= callable;
}

void ElegantOTAClass::onEnd(std::function<void(bool success)> callable){
    postUpdateCallback = callable;
}


ElegantOTAClass ElegantOTA;
