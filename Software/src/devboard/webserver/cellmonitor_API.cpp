#include "cellmonitor_API.h"
#include <Arduino.h>

AsyncJsonResponse* cellmonitorAPI_GET() {
	
    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
	
    for (uint8_t i = 0u; i < MAX_AMOUNT_CELLS; i++) {
      if (system_cellvoltages_mV[i] == 0) {
        continue;
      }
      root[String(i+1)] = system_cellvoltages_mV[i];
    }
	return response;
}
