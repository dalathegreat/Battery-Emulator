#ifndef API_ROUTES_H
#define API_ROUTES_H

// JSON REST API ("/api/*") consumed by the new Lit web UI (frontend/).
//
// This module lives alongside the legacy HTML webserver implementation on
// purpose: the legacy pages are untouched so this branch merges cleanly.
// Removal of the legacy implementation will happen in follow-up PRs once
// the new UI is fully integrated.
//
// A runtime switch (NVM bool "NEWWEBUI", default false) decides which UI is
// served at "/". The /api/* endpoints and all legacy routes are registered in
// both modes. Changing the switch requires a reboot (routes cannot be
// re-registered at runtime with ESPAsyncWebServer).

#include <functional>
#include "../../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"

// Defined in webserver.cpp - registers a route guarded by HTTP Basic auth.
extern void def_route_with_auth(const char* uri, AsyncWebServer& serv, WebRequestMethodComposite method,
                                std::function<void(AsyncWebServerRequest*)> handler);

// True when the NVM switch "NEWWEBUI" is set: "/" serves the new SPA shell
// instead of the legacy dashboard HTML. Valid after init_api_routes().
bool new_webui_enabled();

// Registers all /api/* routes, the "/enableNewWebUI" toggle and (when the
// new UI mode is active) the SPA shell + /app.bundle.js routes.
// Must be called BEFORE the legacy routes are registered so the SPA shell
// takes precedence on shared paths (/settings, /events, ...).
void init_api_routes(AsyncWebServer& server);

// Individual registration helpers (implementation detail, one per file).
void init_api_dashboard_routes(AsyncWebServer& server);
void init_api_battery_routes(AsyncWebServer& server);
void init_api_settings_routes(AsyncWebServer& server);
void init_api_events_routes(AsyncWebServer& server);
void init_api_debuglog_routes(AsyncWebServer& server);
void init_api_canlog_routes(AsyncWebServer& server);

#endif  // API_ROUTES_H
