#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <stddef.h>
#include <stdint.h>
#include "../../Software/src/lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "WString.h"

class Preferences {
 private:
  JsonDocument doc;
  std::string filename;
  bool readonly;

 public:
  bool begin(const char* name, bool read_only = false);
  void end();

  bool clear() { return true; }
  bool remove(const char* key);

  size_t putChar(const char* key, int8_t value);
  size_t putUChar(const char* key, uint8_t value);
  size_t putShort(const char* key, int16_t value);
  size_t putUShort(const char* key, uint16_t value);
  size_t putInt(const char* key, int32_t value);
  size_t putUInt(const char* key, uint32_t value);
  size_t putLong(const char* key, int32_t value);
  size_t putULong(const char* key, uint32_t value);
  size_t putLong64(const char* key, int64_t value);
  size_t putULong64(const char* key, uint64_t value);
  size_t putBool(const char* key, bool value);
  size_t putString(const char* key, const char* value);
  size_t putString(const char* key, String value);
  size_t putBytes(const char* key, const void* value, size_t len);

  bool isKey(const char* key);
  int8_t getChar(const char* key, int8_t defaultValue = 0);
  uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
  int16_t getShort(const char* key, int16_t defaultValue = 0);
  uint16_t getUShort(const char* key, uint16_t defaultValue = 0);
  int32_t getInt(const char* key, int32_t defaultValue = 0);
  uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
  int32_t getLong(const char* key, int32_t defaultValue = 0);
  uint32_t getULong(const char* key, uint32_t defaultValue = 0);
  int64_t getLong64(const char* key, int64_t defaultValue = 0);
  uint64_t getULong64(const char* key, uint64_t defaultValue = 0);
  bool getBool(const char* key, bool defaultValue = false);
  size_t getString(const char* key, char* value, size_t maxLen);
  String getString(const char* key, String defaultValue = String()) { return String(); }
  size_t getBytesLength(const char* key);
  size_t getBytes(const char* key, void* buf, size_t maxLen);
  size_t freeEntries();

 private:
};

#endif
