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
 * "pure" tells the compiler this only reads memory and has no side effects.
 */
extern uint8_t Crc_CalculateCRC8H2F(const uint8_t* data, uint16_t length, uint8_t initial_value,
                                    uint8_t final_xor_value) __attribute__((pure));

/**
 * @brief Calculate the 16-bit checksum used by Hyundai/Kia CAN-FD frames (E-GMP platform).
 *
 * CRC-16 (polynomial 0x1021, initial value 0, no reflection) over data bytes 2..dlc-1, followed
 * by the low and high byte of the CAN ID, then XORed with a constant that depends on the frame
 * length (8: 0x5F29, 16: 0x041D, 24: 0x819D, 32: 0x9F5B). The result is stored little-endian in
 * bytes 0-1 of the frame; byte 2 holds an alive counter that increments on every transmission.
 *
 * Verified against logged frames, e.g. ID 0x25A DLC 32
 * "6D 44 12 4C 40 20 00 00 00 00 2E 00 77 2A 7F 00 00 C8 00 00 00 80 C0 70 02 0F 08 01 00 00 00 00" -> 0x446D
 * and ID 0x2C0 DLC 32 "CC CD A2 21 00 A1 00 00 40 00 00 00 00 00 7D 00 00 .. 00" -> 0xCDCC.
 *
 * @param[in] data Pointer to the full frame payload (bytes 0-1 are ignored)
 * @param[in] dlc Frame length in bytes (8, 16, 24 or 32)
 * @param[in] can_id CAN identifier of the frame
 *
 * @return uint16_t Checksum to store in data[0] (low byte) and data[1] (high byte)
 */
extern uint16_t crc16_hyundai_canfd(const uint8_t* data, uint8_t dlc, uint32_t can_id) __attribute__((pure));
