#ifndef _SETTINGS_VALIDATION_H_
#define _SETTINGS_VALIDATION_H_

#include <stdint.h>

/* SOC window validation.
 *
 * The SOC window (min_percentage / max_percentage) rescales the SOC reported
 * to the inverter: at real SOC == min the inverter sees 0%, at real SOC == max
 * it sees 100%. All values are in pptt (percent * 100), e.g. 2000 = 20.00%.
 *
 * The same rules are enforced at every entry point (webserver live routes and
 * boot-time load of stored settings), so a value can never be accepted in one
 * place and rejected in another. */

/* Lowest allowed minimum. Advanced users can set a negative minimum so the
 * inverter never sees a completely discharged battery. Matches the floor the
 * settings UI has always offered (-10.0%). */
constexpr int32_t SOC_WINDOW_MIN_FLOOR_PPTT = -1000;

constexpr int32_t SOC_WINDOW_MAX_CEIL_PPTT = 10000;

/* Lowest allowed maximum. Keeps max away from 0, which NVS storage uses as
 * its "never stored" sentinel for MAXPERCENTAGE — a stored 0.00% maximum
 * would otherwise be silently replaced by the compiled default at boot. */
constexpr int32_t SOC_WINDOW_MAX_FLOOR_PPTT = 100;

/* Minimum gap between min and max. Enforces min < max and keeps the SOC
 * scaling divisor (max - min) from getting pathologically small; the scaling
 * code itself only guards against a zero divisor. */
constexpr int32_t SOC_WINDOW_MIN_GAP_PPTT = 100;

/* True when the pair satisfies the window rules. Does not modify anything. */
bool validate_soc_window(int32_t min_pptt, int32_t max_pptt);

/* Validates and applies the pair to datalayer.battery.settings.
 * Returns false and writes nothing when the pair is invalid. */
bool set_soc_window(int32_t min_pptt, int32_t max_pptt);

#endif
