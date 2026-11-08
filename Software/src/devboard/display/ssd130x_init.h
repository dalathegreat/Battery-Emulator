#pragma once

#include <cstddef>
#include <cstdint>

/* Shared 128x64 I2C OLED init for SSD1306 and SSD1309.
   Kept out of display.cpp so host unit tests can lock the sequence without
   linking ESP-IDF I2C. */

inline constexpr uint16_t SSD130X_I2C_ADDR_PRIMARY = 0x3c;
inline constexpr uint16_t SSD130X_I2C_ADDR_ALTERNATE = 0x3d;

inline constexpr uint8_t SSD130X_INIT_SEQUENCE[] = {
    0xae,    // display off
    0xd5,    // set display clock divider
    0x80,    //   /1, middle freq
    0xa8,    // set multiplex
    64 - 1,  //   height - 1
    0xd3,    // set display offset
    0x00,    //   no offset
    0x40,    // set start line at 0
    0x8d,    // charge pump
    0x14,    //   enable
    0x20,    // memory mode
    0x00,    //   horizontal addressing
    0xa1,    // seg remap = 1 (mirror x)
    0xc8,    // com scan = dec
    0xda,    // com pins hardware config
    0x12,    //   alternative COM
    0x81,    // contrast
    0xcf,    //   readable on larger panels; old 0x0f was nearly black
    0xd9,    // pre-charge period
    0xf1,    //   for internal charge pump
    0xdb,    // VCOMH deselect level
    0x40,
    0xa4,  // resume RAM content
    0xa6,  // normal (not inverted)
    0xaf,  // display on
};

/* Prefer 0x3C; use 0x3D when only the address-jumper ACK is present.
   Returns 0 if neither address ACKs. */
inline uint16_t ssd130x_select_i2c_address(bool ack_primary, bool ack_alternate) {
  if (ack_primary) {
    return SSD130X_I2C_ADDR_PRIMARY;
  }
  if (ack_alternate) {
    return SSD130X_I2C_ADDR_ALTERNATE;
  }
  return 0;
}

/* True when cmd is immediately followed by arg in the init stream. */
inline bool ssd130x_init_has_cmd_arg(uint8_t cmd, uint8_t arg) {
  for (size_t i = 0; i + 1 < sizeof(SSD130X_INIT_SEQUENCE); i++) {
    if (SSD130X_INIT_SEQUENCE[i] == cmd && SSD130X_INIT_SEQUENCE[i + 1] == arg) {
      return true;
    }
  }
  return false;
}

inline bool ssd130x_init_has_cmd(uint8_t cmd) {
  for (size_t i = 0; i < sizeof(SSD130X_INIT_SEQUENCE); i++) {
    if (SSD130X_INIT_SEQUENCE[i] == cmd) {
      return true;
    }
  }
  return false;
}
