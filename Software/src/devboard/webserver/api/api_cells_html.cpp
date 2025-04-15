#include "api_cells_html.h"
#include "api_helpers.h"
#include "../../../datalayer/datalayer.h"

// Helper function to add a cell entry to the JSON
void addCellEntry(String& json, int index, int voltage, bool addComma) {
  json += "{";
  addJsonNumber(json, "index", index, true);
  addJsonNumber(json, "voltage", voltage, false);
  json += "}";
  if (addComma) json += ",";
}

String api_cells_processor() {
  String json = "{\"cells\":[";
  
  // Add all cell data
  int cellCount = datalayer.battery.info.number_of_cells;
  for (int i = 0; i < cellCount; i++) {
    // Get the cell voltage - adjust this based on how your cell data is stored
    int cellVoltage = datalayer.battery.status.cell_voltages_mV[i];
    addCellEntry(json, i + 1, cellVoltage, i < cellCount - 1);
  }
  
  json += "]}";
  return json;
}