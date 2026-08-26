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

/**
 * @brief Highest cell voltage present in the given chemistry's built-in OCV curve. Voltages at or
 * above this are clamped to 100.00% SOC by soc_from_cell_voltage().
 *
 * @param[in] chemistry Cell chemistry, selects which OCV curve to use
 *
 * @return uint16_t Top of the curve, in millivolts
 */
uint16_t cell_voltage_table_max_mV(battery_chemistry_enum chemistry);

/**
 * @brief Lowest cell voltage present in the given chemistry's built-in OCV curve. Voltages at or
 * below this are clamped to 0.00% SOC by soc_from_cell_voltage().
 *
 * @param[in] chemistry Cell chemistry, selects which OCV curve to use
 *
 * @return uint16_t Bottom of the curve, in millivolts
 */
uint16_t cell_voltage_table_min_mV(battery_chemistry_enum chemistry);

/**
 * @brief Sanity-check a configured cell design voltage range against the given chemistry's
 * built-in OCV curve, allowing some margin above/below the curve for protection setpoints that
 * are intentionally set a bit past the curve's 100%/0% points. Use this to guard against running
 * SOC estimation with a chemistry that clearly does not match the configured cells (e.g. LFP cells
 * with a chemistry setting whose curve tops out at 4.2V).
 *
 * @param[in] min_cell_voltage_mV Configured minimum cell design voltage, in millivolts
 * @param[in] max_cell_voltage_mV Configured maximum cell design voltage, in millivolts
 * @param[in] chemistry Cell chemistry, selects which OCV curve to check against
 *
 * @return true if the configured range is plausible for the given chemistry
 */
bool cell_voltage_range_matches_chemistry(uint16_t min_cell_voltage_mV, uint16_t max_cell_voltage_mV,
                                          battery_chemistry_enum chemistry);

#endif
