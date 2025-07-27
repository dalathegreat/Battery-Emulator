#ifndef HARDWARESERIAL_H
#define HARDWARESERIAL_H

#include <stdint.h>
#include <string.h>
#include "Stream.h"

enum SerialConfig {
  SERIAL_5N1 = 0x8000010,
  SERIAL_6N1 = 0x8000014,
  SERIAL_7N1 = 0x8000018,
  SERIAL_8N1 = 0x800001c,
  SERIAL_5N2 = 0x8000030,
  SERIAL_6N2 = 0x8000034,
  SERIAL_7N2 = 0x8000038,
  SERIAL_8N2 = 0x800003c,
  SERIAL_5E1 = 0x8000012,
  SERIAL_6E1 = 0x8000016,
  SERIAL_7E1 = 0x800001a,
  SERIAL_8E1 = 0x800001e,
  SERIAL_5E2 = 0x8000032,
  SERIAL_6E2 = 0x8000036,
  SERIAL_7E2 = 0x800003a,
  SERIAL_8E2 = 0x800003e,
  SERIAL_5O1 = 0x8000013,
  SERIAL_6O1 = 0x8000017,
  SERIAL_7O1 = 0x800001b,
  SERIAL_8O1 = 0x800001f,
  SERIAL_5O2 = 0x8000033,
  SERIAL_6O2 = 0x8000037,
  SERIAL_7O2 = 0x800003b,
  SERIAL_8O2 = 0x800003f
};

class HardwareSerial : public Stream {
 public:
  void begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1,
             bool invert = false, unsigned long timeout_ms = 20000UL, uint8_t rxfifo_full_thrhd = 120) {}

  size_t write(const uint8_t* buffer, size_t size);
  inline size_t write(const char* buffer, size_t size) { return write((uint8_t*)buffer, size); }
  inline size_t write(const char* s) { return write((uint8_t*)s, strlen(s)); }

  size_t setRxBufferSize(size_t new_size) { return new_size; };

  size_t setTxBufferSize(size_t new_size) { return new_size; };
};

extern HardwareSerial Serial0;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;

// similarly to Arduino
#define Serial Serial0

#endif
