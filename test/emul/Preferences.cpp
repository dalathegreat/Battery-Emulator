#include "Preferences.h"

bool Preferences::begin(const char* name, bool read_only) {
  return true;
}

void Preferences::end() {}

bool Preferences::getBool(const char* key, bool defaultValue) {
  return defaultValue;
}

size_t Preferences::getString(const char* key, char* value, size_t maxLen) {
  return 0;
}

uint32_t Preferences::getUInt(const char* key, uint32_t defaultValue) {
  return defaultValue;
}

size_t Preferences::putBool(const char* key, bool value) {
  return 0;
}

size_t Preferences::putString(const char* key, String value) {
  return 0;
}

size_t Preferences::putUInt(const char* key, uint32_t value) {
  return 0;
}
