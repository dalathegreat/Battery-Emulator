#include <stdint.h>

/**
 * @brief Return ESP32's high-resolution timer in milliseconds, as a 64 bit value.
 * 
 * @param[in] void
 * 
 * @return uint64_t Timestamp in milliseconds
 * 
 */
extern uint64_t millis64(void);
