#include <cstdint>

// Test-only stand-in for esp.c's real RMT-peripheral transmit function. Unit tests build for the
// host (not ESP-IDF), so there's no RMT driver to link against; nothing here needs to inspect
// actual pixel data, just satisfy the linker for Adafruit_NeoPixel::show().
extern "C" void espShow(uint16_t pin, uint8_t* pixels, uint8_t numBytes) {
  (void)pin;
  (void)pixels;
  (void)numBytes;
}
