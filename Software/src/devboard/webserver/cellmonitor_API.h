#ifndef CELLMONITOR_A
#define CELLMONITOR_A

#include <Arduino.h>
#include <stdint.h>
#include "../config.h"  // Needed for defines
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/AsyncJson.h"

extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV

/**
 * @brief Converts an array of cell voltages into a JSON array to be returned as part of an api endpoint response
 *
 * @param[in] var
 *
 * @return AsyncJsonResponse pointer
 */
AsyncJsonResponse* cellmonitorAPI_GET();

#endif
