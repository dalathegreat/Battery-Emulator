#include "HardwareSerial.h"
#include <stdio.h>

size_t HardwareSerial::write(uint8_t c) {
  putchar(c);
  return 1;
}

size_t HardwareSerial::write(const uint8_t* buffer, size_t size) {

  std::lock_guard<std::mutex> lock(writeMutex);

  size_t written = 0;
  for (size_t i = 0; i < size; ++i) {
    written += write(buffer[i]);
  }

  return written;
}

HardwareSerial::operator bool() const {
  return true;
}
