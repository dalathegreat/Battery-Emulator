#include "webserver_settings.h"
#include "settings.h"
#include "webserver_new.h"

#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"

void register_settings_route(AsyncWebServer& server) {
  server.on(
      "/api/internal/settings", HTTP_GET | HTTP_POST,
      [](AsyncWebServerRequest* request) {
        // GET handler - returns all settings in JSON format

        if (request->method() != HTTP_GET) {
          return;
        }
        if (webserver_auth_is_ready() && !request->authenticate(http_username.c_str(), http_password.c_str())) {
          return request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
        }

        BatteryEmulatorSettingsStore settings;

        JsonDocument doc;
        build_settings_json(doc, settings);

        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
      },
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        // Accumulate the whole JSON body (it can arrive as several TCP chunks).
        // Stored as a single malloc'd block so the request destructor's
        // free(_tempObject) stays consistent if the upload is aborted early.
        struct Buf {
          size_t size;
          size_t cap;
          char data[];
        };
        Buf* buf = static_cast<Buf*>(request->_tempObject);
        if (buf == nullptr) {
          size_t cap = total > 512 ? (total + 1) : 4096;
          buf = static_cast<Buf*>(malloc(sizeof(Buf) + cap));
          if (buf == nullptr) {
            request->send(400, "application/json", "{}");
            return;
          }
          buf->size = 0;
          buf->cap = cap;
          request->_tempObject = buf;
        }
        if (buf->size + len > buf->cap) {
          size_t ncap = buf->cap * 2 + 1;
          Buf* nb = static_cast<Buf*>(realloc(buf, sizeof(Buf) + ncap));
          if (nb == nullptr) {
            request->send(400, "application/json", "{}");
            return;
          }
          buf = nb;
          buf->cap = ncap;
          request->_tempObject = buf;
        }
        memcpy(buf->data + buf->size, data, len);
        buf->size += len;
        if (index + len < total) {
          return;  // wait for the remaining chunks
        }

        JsonDocument errors;
        BatteryEmulatorSettingsStore settings;
        JsonDocument doc;
        auto err = deserializeJson(doc, buf->data, buf->size);
        if (err) {
          free(buf);
          request->_tempObject = nullptr;
          request->send(400, "application/json", "{}");
          return;
        }
        bool reboot_required_saved = false;
        for (int attempt = 0; attempt < 2; attempt++) {
          apply_setting_updates(doc, errors, settings, attempt == 1, reboot_required_saved);
          if (errors.size()) {
            String payload;
            serializeJson(errors, payload);
            free(buf);
            request->_tempObject = nullptr;
            request->send(400, "application/json", payload);
            return;
          }
        }
        // Only RebootRequired settings demand a reboot; Instant and Volatile
        // settings take effect immediately, so they must not flag a reboot.
        if (reboot_required_saved) {
          settingsUpdated = true;
        }

        free(buf);
        request->_tempObject = nullptr;
        // The frontend calls response.json() on success, so return a JSON body.
        request->send(200, "application/json", "{}");
      });
}
