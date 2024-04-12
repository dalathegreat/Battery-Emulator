#ifndef _TYPES_H_
#define _TYPES_H_

// enum bms_status_enum { STANDBY = 0, INACTIVE = 1, DARKSTART = 2, ACTIVE = 3, FAULT = 4, UPDATING = 5 };
enum battery_chemistry_enum { NCA, NMC, LFP };
enum led_color { GREEN, YELLOW, RED, BLUE, RGB };

// Inverter definitions
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

#define DISCHARGING 1
#define CHARGING 2

#define INTERVAL_10_MS 10
#define INTERVAL_20_MS 20
#define INTERVAL_30_MS 30
#define INTERVAL_50_MS 50
#define INTERVAL_100_MS 100
#define INTERVAL_200_MS 200
#define INTERVAL_500_MS 500
#define INTERVAL_640_MS 640
#define INTERVAL_1_S 1000
#define INTERVAL_2_S 2000
#define INTERVAL_5_S 5000
#define INTERVAL_10_S 10000
#define INTERVAL_60_S 60000

#define INTERVAL_10_MS_DELAYED 15
#define INTERVAL_20_MS_DELAYED 30
#define INTERVAL_30_MS_DELAYED 40
#define INTERVAL_100_MS_DELAYED 120

#define MAX_CAN_FAILURES 500  // Amount of malformed CAN messages to allow before raising a warning

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
