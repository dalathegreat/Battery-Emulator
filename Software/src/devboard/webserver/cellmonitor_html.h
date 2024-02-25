#ifndef CELLMONITOR_H
#define CELLMONITOR_H

#include <Arduino.h>
#include <stdint.h>

extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String cellmonitor_processor(const String& var);

#endif
