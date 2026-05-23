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

extern const uint8_t crc8_table_SAE_J1850_ZER0[256];
extern const uint8_t crctable_nissan_leaf[256];
extern const uint8_t crctable_geely_geometryC[256];
