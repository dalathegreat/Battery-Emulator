#include <stdint.h>
#include <chrono>
#include <cstdint>

unsigned long millis() {
  static const auto start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  return static_cast<unsigned long>(elapsed);
}

uint64_t get_timestamp(unsigned long millis) {
  return 0;
}

uint64_t millis64(void) {
  static const auto start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  return static_cast<uint64_t>(elapsed);
}
