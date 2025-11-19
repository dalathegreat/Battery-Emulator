#include "common_functions.h"

int16_t sign_extend_to_int16(uint16_t input, unsigned input_bit_width) {
  // Sign-extend the value from the original bit width up to 16 bits. This
  // ensures that twos-complement negative values are correctly interpreted.

  // For example, -26 represented in 13 bits is 0x1FE6.
  // After sign extension to 16 bits, it becomes 0xFFE6, which is -26 in 16-bit signed integer.

  uint16_t mask = 1 << (input_bit_width - 1);
  return (input ^ mask) - mask;
}
