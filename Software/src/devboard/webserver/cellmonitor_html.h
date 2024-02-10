#ifndef CELLMONITOR_H
#define CELLMONITOR_H

extern uint16_t cell_max_voltage;   //mV,   0-4350
extern uint16_t cell_min_voltage;   //mV,   0-4350
extern uint16_t cellvoltages[120];  //mV    0-4350 per cell

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String cellmonitor_processor(const String& var);

#endif
