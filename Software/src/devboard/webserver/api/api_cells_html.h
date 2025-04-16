#ifndef API_CELLS_HTML_H
#define API_CELLS_HTML_H

#include <Arduino.h>

/**
 * @brief Creates JSON string with cell voltage data
 *
 * @return String JSON-formatted string with cell data
 */
String api_cells_processor();

// Helper function to add a cell entry to the JSON
void addCellEntry(String& json, int index, int voltage, bool addComma = true);

#endif
