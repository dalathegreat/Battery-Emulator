#include "Arduino.h"
#include <cctype>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "esp_system.h"

int max(int a, int b) {
  return (a > b) ? a : b;
}

int min(int a, int b) {
  return (a < b) ? a : b;
}

void wait_milliseconds(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void delay(uint32_t ms) {
  wait_milliseconds(ms);
}

float temperatureRead(void) {
  return 25.0f;  // Return a dummy temperature value
}

esp_reset_reason_t esp_reset_reason(void) {
  return ESP_RST_POWERON;
}

void pinMode(uint8_t pin, uint8_t mode) {}

void digitalWrite(uint8_t pin, uint8_t val) {}

int digitalRead(uint8_t pin) {
  return 0;
}

uint32_t ledcWriteTone(uint8_t pin, uint32_t freq) {
  return 0;
}

bool ledcWrite(uint8_t pin, uint32_t duty) {
  return true;
}

bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, uint8_t channel) {
  return true;
}

uint64_t micros() {
  using namespace std::chrono;
  static const auto start = high_resolution_clock::now();
  auto now = high_resolution_clock::now();
  return duration_cast<microseconds>(now - start).count();
}

void delayMicroseconds(uint32_t us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

extern "C" void espShow(uint8_t pin, uint8_t* pixels, uint32_t numBytes) {}

esp_err_t esp_task_wdt_add(TaskHandle_t task_handle) {
  // Simulate adding a task to the watchdog timer
  return 0;  // ESP_OK
}

esp_err_t esp_task_wdt_reset(void) {
  // Simulate resetting the watchdog timer
  return 0;  // ESP_OK
}

void EspClass::restart() {
  // Simulate a restart by throwing an exception
  throw std::runtime_error("Simulated restart");
}

int strcasecmp(const char* s1, const char* s2) {
  while (*s1 && *s2) {
    unsigned char c1 = std::tolower(static_cast<unsigned char>(*s1));
    unsigned char c2 = std::tolower(static_cast<unsigned char>(*s2));
    if (c1 != c2)
      return c1 - c2;
    ++s1;
    ++s2;
  }
  return std::tolower(static_cast<unsigned char>(*s1)) - std::tolower(static_cast<unsigned char>(*s2));
}

char* strdup(const char* s) {
  if (!s)
    return nullptr;
  size_t len = strlen(s) + 1;
  char* copy = new char[len];
  if (copy) {
    memcpy(copy, s, len);
  }
  return copy;
}

char* strndup(char const* str, size_t size) {
  if (!str)
    return nullptr;
  size_t len = strnlen(str, size);
  char* copy = new char[len + 1];
  if (copy) {
    memcpy(copy, str, len);
    copy[len] = '\0';
  }
  return copy;
}

struct tm* localtime_r(const time_t*, struct tm*) {
  // This function is not implemented in the emulation, return nullptr
  return nullptr;
}

void log_printf(const char* format, ...) {
  constexpr size_t INITIAL_BUFFER_SIZE = 1024;

  // Start processing variable arguments
  va_list args;
  va_start(args, format);

  // Attempt to format into a fixed-size buffer first
  std::vector<char> buffer(INITIAL_BUFFER_SIZE);
  int requiredSize = vsnprintf(buffer.data(), buffer.size(), format, args);

  va_end(args);  // Clean up va_list before reuse
  if (requiredSize < 0) {
    std::clog << "[log_printf] Formatting error.\n";
    return;
  }

  // Resize buffer if needed and format again
  if (static_cast<size_t>(requiredSize) >= buffer.size()) {
    buffer.resize(requiredSize + 1);  // +1 for null terminator
    va_start(args, format);           // Reuse va_list
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
  }

  std::clog << buffer.data();
}

unsigned short makeWord(unsigned char high, unsigned char low) {
  return (static_cast<unsigned short>(high) << 8) | low;
}

unsigned short analogRead(unsigned char) {
  return 0;
}

long random(long min, long max) {
  return min + (std::rand() % (max - min + 1));
}

void randomSeed(unsigned long seed) {
  std::srand(static_cast<unsigned int>(seed));
}

EspClass ESP;
