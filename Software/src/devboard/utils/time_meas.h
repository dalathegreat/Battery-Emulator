#ifndef TIME_MEAS_H_
#define TIME_MEAS_H_

#include "../../datalayer/datalayer.h"
#include "esp_timer.h"

/* The measurement macros below are gated on the "Performance profiling on main
 * page" setting (datalayer.system.info.performance_measurement_active, read once
 * from NVS at boot). Gating inside the macros instead of at every call site keeps
 * the timestamping out of the time critical loops when profiling is disabled:
 * esp_timer_get_time() is a real function call with a hardware poll inside, and
 * it is not declared pure, so the compiler cannot elide it even when the result
 * is never used.
 */

/** Start time measurement in microseconds
 * Input parameter must be a unique "tag", e.g: START_TIME_MEASUREMENT(wifi);
 * Takes no timestamp when performance profiling is disabled.
 */
#define START_TIME_MEASUREMENT(x)                  \
  int64_t start_time_##x __attribute__((unused)) = \
      datalayer.system.info.performance_measurement_active ? esp_timer_get_time() : 0
/** End time measurement in microseconds
 * Input parameters are the unique tag and the name of the ALREADY EXISTING
 * destination variable (int64_t), e.g: END_TIME_MEASUREMENT(wifi, my_wifi_time_int64_t);
 * The destination is left untouched when performance profiling is disabled.
 */
#define END_TIME_MEASUREMENT(x, y)                              \
  do {                                                          \
    if (datalayer.system.info.performance_measurement_active) { \
      (y) = esp_timer_get_time() - start_time_##x;              \
    }                                                           \
  } while (0)
/** End time measurement in microseconds, log maximum
 * Input parameters are the unique tag and the name of the ALREADY EXISTING
 * destination variable (int64_t), e.g: END_TIME_MEASUREMENT_MAX(wifi, my_wifi_time_int64_t);
 *
 * This will log the maximum value in the destination variable.
 * The destination is left untouched when performance profiling is disabled.
 */
#define END_TIME_MEASUREMENT_MAX(x, y)                             \
  do {                                                             \
    if (datalayer.system.info.performance_measurement_active) {    \
      int64_t elapsed_##x = esp_timer_get_time() - start_time_##x; \
      if (elapsed_##x > (y)) {                                     \
        (y) = elapsed_##x;                                         \
      }                                                            \
    }                                                              \
  } while (0)

#endif
