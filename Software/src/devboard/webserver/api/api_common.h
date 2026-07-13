#ifndef API_COMMON_H
#define API_COMMON_H

// Shared helpers for the JSON REST API handlers.

#include "../../../datalayer/datalayer.h"
#include "../../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../../lib/bblanchon-ArduinoJson/ArduinoJson.h"

const char* api_bms_status_string(real_bms_status_enum status);
const char* api_balancing_status_string(balancing_status_enum status);
const char* api_system_status_string(system_status_enum status);
const char* api_emulator_status_string();

// Number of cells currently reported as balancing.
uint16_t api_count_balancing_cells(const DATALAYER_BATTERY_TYPE& b);

// Fills a JSON object with the BatteryPackStatus shape expected by the
// frontend (frontend/src/types/battery.types.ts).
void api_fill_pack_status(JsonObject o, const DATALAYER_BATTERY_TYPE& b, bool present);

// Serializes the document and sends it as application/json.
void api_send_json(AsyncWebServerRequest* request, const JsonDocument& doc);

#endif  // API_COMMON_H
