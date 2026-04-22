#pragma once

class SPISettings {
 public:
  SPISettings() = default;
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {}
};

class SPIClass {
 public:
  SPIClass() = default;
  void begin() {}
  void end() {}
  void beginTransaction(const SPISettings& settings) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t data) { return data; }
};
