#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "../../include.h"

/** MIN macro, returns the smallest of two values
 * Warning: Side effects. MUST be called using values and/or variables only, e.g.:
 * int x = MIN(temp1, temp2);
 * int x = MIN(reported_temp, 25);
 * Do NOT use this macro with operators or function calls, e.g.:
 * int x = MIN(y++, my_func());
*/
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/** MAX macro, returns the largest of two values
 * Warning: Side effects. MUST be called using values and/or variables only, e.g.:
 * int x = MAX(temp1, temp2);
 * int x = MAX(reported_temp, 10);
 * Do NOT use this macro with operators or function calls, e.g.:
 * int x = MAX(y++, my_func());
*/
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
/** CONSTRAIN macro, limits a value to a provided range
 * Warning: Side effects. MUST be called using values and/or variables only, e.g.:
 * int x = CONSTRAIN(temp, min_temp, max_temp);
 * int x = CONSTRAIN(reported_temp, 10, 30);
 * Do NOT use this macro with operators or function calls, e.g.:
 * int x = CONSTRAIN(y++, my_min_func(), max_value--);
*/
#define CONSTRAIN(val, min, max) (MIN(max, MAX(min, val)))

static inline float map_float(float val, float in_min, float in_max, float out_min, float out_max) {
  if (val <= in_min) {
    return out_min;
  } else if (val > in_max) {
    return out_max;
  } else {
    return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

static inline uint8_t map_uint8(uint8_t val, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max) {
  if (val <= in_min) {
    return out_min;
  } else if (val >= in_max) {
    return out_max;
  } else {
    // Ensure the multiplication does not overflow by promoting to a larger type (uint16_t)
    // And ensure the division happens last to minimize precision loss
    return static_cast<uint8_t>((static_cast<uint16_t>(val - in_min) * (out_max - out_min)) / (in_max - in_min) +
                                out_min);
  }
}

static inline uint16_t map_uint16(uint16_t val, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  if (val <= in_min) {
    return out_min;
  } else if (val >= in_max) {
    return out_max;
  } else {
    // Promote to uint32_t to prevent overflow during multiplication
    return static_cast<uint16_t>((static_cast<uint32_t>(val - in_min) * (out_max - out_min)) / (in_max - in_min) +
                                 out_min);
  }
}

#endif
