#ifndef MDNS_H
#define MDNS_H

#include <stdint.h>
#include "WString.h"

class MDNSResponder {
 public:
  bool addService(String service, String proto, uint16_t port) { return true; }

  bool begin(const String& hostName) { return true; }
};

extern MDNSResponder MDNS;

#endif
