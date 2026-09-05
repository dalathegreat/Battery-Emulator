#pragma once

#include <stdint.h>

#include <vector>

// Arbitrary bus indices
#define HSPI 1
#define VSPI 2
#define FSPI 3

#define MSBFIRST 1
#define SPI_MODE0 0

class SPISettings {
 public:
  SPISettings(uint32_t, uint8_t, uint8_t) {}
};

// Records the bytes shifted out so tests can assert the exact command sequence
// a driver puts on the wire (chip-select edges are not modelled).
class SPIClass {
 public:
  std::vector<uint8_t> transferred;
  bool in_transaction = false;

  void beginTransaction(SPISettings) { in_transaction = true; }
  void endTransaction() { in_transaction = false; }
  uint8_t transfer(uint8_t data) {
    transferred.push_back(data);
    return 0;
  }
  uint16_t transfer16(uint16_t data) {
    transferred.push_back(static_cast<uint8_t>(data >> 8));
    transferred.push_back(static_cast<uint8_t>(data & 0xFF));
    return 0;
  }
};
