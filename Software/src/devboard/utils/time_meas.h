#ifndef TIME_MEAS_H_
#define TIME_MEAS_H_

#include "esp_timer.h"

/** Start time measurement in microseconds
 * Input parameter must be a unique "tag", e.g: START_TIME_MEASUREMENT(wifi);
 */
#ifdef FUNCTION_TIME_MEASUREMENT
#define START_TIME_MEASUREMENT(x) int64_t start_time_##x = esp_timer_get_time()
/** End time measurement in microseconds
 * Input parameters are the unique tag and the name of the ALREADY EXISTING
 * destination variable (int64_t), e.g: END_TIME_MEASUREMENT(wifi, my_wifi_time_int64_t);
 */
#define END_TIME_MEASUREMENT(x, y) y = esp_timer_get_time() - start_time_##x
/** End time measurement in microseconds, log maximum
 * Input parameters are the unique tag and the name of the ALREADY EXISTING
 * destination variable (int64_t), e.g: END_TIME_MEASUREMENT_MAX(wifi, my_wifi_time_int64_t);
 * 
 * This will log the maximum value in the destination variable.
 */
#define END_TIME_MEASUREMENT_MAX(x, y) y = MAX(y, esp_timer_get_time() - start_time_##x)
#else
#define START_TIME_MEASUREMENT(x) ;
#define END_TIME_MEASUREMENT(x, y) ;
#define END_TIME_MEASUREMENT_MAX(x, y) ;
#endif

#endif
