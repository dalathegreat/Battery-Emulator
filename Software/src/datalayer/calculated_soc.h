#ifndef CALCULATED_SOC_H
#define CALCULATED_SOC_H

/**
 * @brief Derives the reported (inverter-facing) SOC and capacity from the real
 * values of every configured battery.
 *
 * Split out of update_calculated_values() in Software.cpp: this part is pure
 * datalayer arithmetic, while the rest of that function reads the CPU
 * temperature and free heap. Keeping it in its own translation unit is what
 * makes it reachable from the host test suite - Software.cpp pulls in the
 * webserver, wifi, mqtt, sdcard and display stacks, none of which this
 * calculation needs.
 */
void update_reported_soc_and_capacity();

#endif  // CALCULATED_SOC_H
