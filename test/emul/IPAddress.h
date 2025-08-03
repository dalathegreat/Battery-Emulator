#ifndef EMUL_IPADDRESS_H
#define EMUL_IPADDRESS_H

#include "lwip/ip_addr.h"

#include <WString.h>
#include <stdint.h>

class IPAddress {
 private:
  ip4_addr_t addr;

 public:
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    addr.addr = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(c) << 8) |
                static_cast<uint32_t>(d);
  }
  String toString(bool includeZone = false) const {
    return String((addr.addr >> 24) & 0xFF) + '.' + String((addr.addr >> 16) & 0xFF) + '.' +
           String((addr.addr >> 8) & 0xFF) + '.' + String(addr.addr & 0xFF);
  }

  IPAddress(uint32_t a) { addr.addr = a; }

  IPAddress(const std::string& ipStr) {
    uint8_t bytes[4] = {0};
    int parsed = std::sscanf(ipStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1], &bytes[2], &bytes[3]);
    if (parsed != 4) {
      // Handle invalid input (fallback to 127.0.0.1 or zero)
      bytes[0] = 127;
      bytes[1] = 0;
      bytes[2] = 0;
      bytes[3] = 1;
    }
    uint32_t ip = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    addr.addr = ip;
  }

  bool operator==(const IPAddress& addr) const { return addr.addr.addr == this->addr.addr; }
  bool operator!=(const IPAddress& addr) const { return !(*this == addr); };

  operator uint32_t() const { return addr.addr; };
};

#endif
