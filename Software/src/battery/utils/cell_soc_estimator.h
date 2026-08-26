#ifndef CELL_SOC_ESTIMATOR_H
#define CELL_SOC_ESTIMATOR_H

#include <stdint.h>
#include "../../devboard/utils/types.h"

/**
 * @brief Estimate SOC (0-10000, i.e. 0.00%-100.00%) from a single cell's voltage, using a
 * built-in open-circuit-voltage curve for the given chemistry. Values above/below the curve
 * are clamped to 100.00%/0.00%.
 *
 * @param[in] cell_voltage_mV Cell voltage in millivolts
 * @param[in] chemistry Cell chemistry, selects which OCV curve to use
 *
 * @return uint16_t Estimated SOC, 0-10000
 */
uint16_t soc_from_cell_voltage(uint16_t cell_voltage_mV, battery_chemistry_enum chemistry);

/**
 * @brief Estimate a conservative pack-level SOC from the pack's lowest and highest cell voltages.
 * Above 50% SOC the highest cell is trusted (so charging tapers off before that cell is
 * overcharged); below 50% SOC the lowest cell is trusted (so discharging stops before that cell
 * is over-discharged).
 *
 * @param[in] min_cell_voltage_mV Lowest cell voltage in the pack, in millivolts
 * @param[in] max_cell_voltage_mV Highest cell voltage in the pack, in millivolts
 * @param[in] chemistry Cell chemistry, selects which OCV curve to use
 *
 * @return uint16_t Estimated SOC, 0-10000
 */
uint16_t soc_from_min_max_cell_voltage(uint16_t min_cell_voltage_mV, uint16_t max_cell_voltage_mV,
                                       battery_chemistry_enum chemistry);

#endif
