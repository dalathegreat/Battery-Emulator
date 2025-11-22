#include <stdint.h>

/**
 * @brief Sign-extend the value from the original bit width up to 16 bits. This ensures that twos-complement negative values are correctly interpreted.
 * 
 * @param[in] 
 * 
 * @return int16_t Extended int16_t value
 * 
 */
extern int16_t sign_extend_to_int16(uint16_t input, unsigned input_bit_width);
