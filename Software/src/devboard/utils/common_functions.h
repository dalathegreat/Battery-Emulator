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
extern const uint8_t crctable_crc8_2f[256];

/**
 * @brief Calculate an AUTOSAR CRC8H2F (0x2F polynomial) checksum over a buffer.
 *
 * @param[in] data Pointer to the bytes to checksum
 * @param[in] length Number of bytes to checksum
 * @param[in] initial_value CRC start value (0xFF for the AUTOSAR default)
 * @param[in] final_xor_value Value XORed into the result (0xFF for the AUTOSAR default)
 *
 * @return uint8_t Calculated checksum
 *
 */
extern uint8_t Crc_CalculateCRC8H2F(const uint8_t* data, uint16_t length, uint8_t initial_value,
                                    uint8_t final_xor_value);
