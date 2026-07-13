#include "api_routes.h"
#include "../../../communication/nvm/comm_nvm.h"

// The brotli-compressed SPA bundle is generated at build time by
// frontend/scripts/generate-cpp.mjs (see scripts/build_frontend.py).
// SMALL_FLASH_DEVICE images (4 MB) can't fit both UIs: by default they keep
// the legacy UI ("/api/*" stays available, "/" serves the legacy pages).
// With -DNEW_WEBUI_ONLY it's the other way around: the legacy HTML pages are
// not registered (the linker garbage-collects them) and the SPA is embedded.
#if __has_include("../app_bundle.h") && (!defined(SMALL_FLASH_DEVICE) || defined(NEW_WEBUI_ONLY))
#include "../app_bundle.h"
#define APP_BUNDLE_AVAILABLE 1
#else
#define APP_BUNDLE_AVAILABLE 0
#endif

static bool new_webui_enabled_flag = false;

bool new_webui_enabled() {
  return new_webui_enabled_flag;
}

// Minimal HTML shell that mounts the SPA. Mirrors frontend/index.html.
static const char SPA_SHELL_HTML[] = R"rawliteral(<!doctype html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta name="theme-color" content="#0b1215" />
    <title>Battery Emulator</title>
    <style>
      html, body { margin: 0; height: 100%; background: #0b1215; }
    </style>
  </head>
  <body>
    <lit-app></lit-app>
    <script type="module" src="/app.bundle.js"></script>
  </body>
</html>
)rawliteral";

static void serve_spa_shell(AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response = request->beginResponse(200, "text/html", SPA_SHELL_HTML);
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

static void init_spa_routes(AsyncWebServer& server) {
#if APP_BUNDLE_AVAILABLE
  server.on("/app.bundle.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response =
        request->beginResponse(200, "application/javascript", app_bundle_js_br, app_bundle_js_br_len);
    response->addHeader("Content-Encoding", "br");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });

  // SPA entry point. Registered before the legacy routes (init_api_routes runs
  // first in init_webserver), so in new-UI mode these take precedence over the
  // legacy HTML pages sharing the same paths. The SPA uses history routing, so
  // every page path must serve the same shell for deep links/reloads to work.
  def_route_with_auth("/", server, HTTP_GET, serve_spa_shell);
  const char* spa_paths[] = {"/settings", "/cellmonitor", "/events", "/canlog", "/debuglog", "/advanced"};
  for (const char* path : spa_paths) {
    def_route_with_auth(path, server, HTTP_GET, serve_spa_shell);
  }
#endif  // APP_BUNDLE_AVAILABLE
}

void init_api_routes(AsyncWebServer& server) {
#ifdef NEW_WEBUI_ONLY
  // Legacy HTML pages are not part of this build; the SPA is the only UI.
  new_webui_enabled_flag = true;
#else
  {
    BatteryEmulatorSettingsStore settings(true);
    new_webui_enabled_flag = settings.getBool("NEWWEBUI", false);
  }

#if !APP_BUNDLE_AVAILABLE
  // Without an embedded bundle we can only serve the legacy UI.
  new_webui_enabled_flag = false;
#endif
#endif  // NEW_WEBUI_ONLY

  // Runtime switch between the new SPA and the legacy HTML UI.
  // Takes effect after reboot (routes cannot be re-registered at runtime).
  def_route_with_auth("/enableNewWebUI", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "Bad Request");
      return;
    }
#if defined(NEW_WEBUI_ONLY)
    if (request->getParam("value")->value().toInt() != 0) {
      request->send(200, "text/plain", "New web UI is already the only UI in this build.");
    } else {
      request->send(400, "text/plain", "Legacy web UI is not included in this build.");
    }
#elif APP_BUNDLE_AVAILABLE
    bool enable = request->getParam("value")->value().toInt() != 0;
    BatteryEmulatorSettingsStore settings;
    settings.saveBool("NEWWEBUI", enable);
    request->send(200, "text/plain", enable ? "New web UI enabled. Reboot to apply." : "Legacy web UI enabled. Reboot to apply.");
#else
    request->send(400, "text/plain", "New web UI not available in this build (no embedded bundle).");
#endif
  });

  if (new_webui_enabled_flag) {
    init_spa_routes(server);
  }

  init_api_dashboard_routes(server);
  init_api_battery_routes(server);
  init_api_settings_routes(server);
  init_api_events_routes(server);
  init_api_debuglog_routes(server);
  init_api_canlog_routes(server);
}
