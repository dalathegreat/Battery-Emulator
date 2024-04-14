#ifndef CELLMONITOR_H
#define CELLMONITOR_H

#include "../../include.h"

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
