#ifndef VOLTAGE_SOC_GUARD_H
#define VOLTAGE_SOC_GUARD_H

#include <stdint.h>

/** Voltage-based SOC plausibility guard
 *
 * Estimates SOC from an NMC open-circuit-voltage lookup table using the
 * measured min/max cell voltages (compensated for IR drop via pack current),
 * then clamps the BMS-reported SOC (real_soc, before any SOC scaling) into a
 * plausibility band
 * around that estimate. Protects against BMS SOC drift on packs where the
 * coulomb counter recalibrates poorly (e.g. Pylontech-protocol DIY BMS).
 *
 * Only active for NMC chemistry. Configured via webserver settings:
 *   VSOCGUARD (bool)  - enable/disable, default off
 *   VSOCIR    (uint)  - per-cell internal resistance in tenths of mOhm, default 5 (=0.5 mOhm)
 *   VSOCBAND  (uint)  - allowed deviation band in integer-percent x 100, default 1000 (=10.00%)
 */

extern bool user_selected_voltage_soc_guard;
extern uint16_t user_selected_voltage_soc_cell_ir_dmohm;
extern uint16_t user_selected_voltage_soc_band_pptt;

/** Clamp datalayer.battery.status.real_soc based on cell-voltage SOC estimate.
 *  Call before SOC scaling in update_calculated_values(). */
void apply_voltage_soc_guard();

#endif  // VOLTAGE_SOC_GUARD_H
