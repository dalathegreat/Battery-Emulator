#pragma once

// Host-side stub for the arduino-esp32 IPAddress only to satisfy the compiler
class IPAddress {
 public:
  bool fromString(const char*) { return false; }
};
