#include "api_full_html.h"
#include "api_helpers.h"
#include "api_status_html.h"
#include "api_cells_html.h"
#include "api_events_html.h"
#include "../../../datalayer/datalayer.h"

String api_full_processor() {
  // Start with the opening brace
  String json = "{";
  
  // Get the individual API responses (without the outer braces)
  String statusJson = api_status_processor();
  String cellsJson = api_cells_processor();
  String eventsJson = api_events_processor();
  
  // Remove the outer braces from each JSON string
  statusJson = statusJson.substring(1, statusJson.length() - 1);
  cellsJson = cellsJson.substring(1, cellsJson.length() - 1);
  eventsJson = eventsJson.substring(1, eventsJson.length() - 1);
  
  // Combine the JSON strings
  json += statusJson;
  json += ",";
  json += cellsJson;
  json += ",";
  json += eventsJson;
  
  // Add the closing brace
  json += "}";
  
  return json;
}