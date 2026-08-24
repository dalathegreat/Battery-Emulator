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
  SPISettings() {}
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) : clock(clock) {
    (void)bitOrder;
    (void)dataMode;
  }
  uint32_t clock = 0;
};

// Records every byte shifted out so tests can assert the exact command
// sequence a driver puts on the wire (chip-select edges are not modelled).
class SPIClass {
 public:
  std::vector<uint8_t> transferred;
  uint32_t transaction_clock = 0;
  bool in_transaction = false;

  void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1) {}
  void beginTransaction(SPISettings settings) {
    in_transaction = true;
    transaction_clock = settings.clock;
  }
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
