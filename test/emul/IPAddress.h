#ifndef IPADDRESS_H
#define IPADDRESS_H

#include <stdint.h>
#include <stdio.h>
#include "WString.h"

class IPAddress {
  uint32_t _addr;

 public:
  IPAddress() : _addr(0) {}
  IPAddress(uint32_t addr) : _addr(addr) {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
      : _addr(((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d) {}

  operator uint32_t() const { return _addr; }

  String toString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (unsigned)((_addr >> 24) & 0xFF), (unsigned)((_addr >> 16) & 0xFF),
             (unsigned)((_addr >> 8) & 0xFF), (unsigned)(_addr & 0xFF));
    return String(buf);
  }
};

#endif
