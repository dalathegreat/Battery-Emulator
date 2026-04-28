#ifndef PREFERENCES
#define PREFERENCES

#include <WString.h>
#include <stdint.h>
#include <cstring>
#include <map>

class Preferences {

 public:
  Preferences() {}

  bool begin(const char* name, bool readOnly = false, const char* partition_label = NULL) {
    (void)name;
    (void)readOnly;
    (void)partition_label;
    return true;
  }
  void end() {}
  bool clear();

  size_t putInt(const char* key, int32_t value) {
    storage[key] = String(value);
    return 4;
  }
  size_t putUInt(const char* key, uint32_t value) {
    storage[key] = String(value);
    return 4;
  }
  size_t putBool(const char* key, bool value) {
    storage[key] = String(value);
    return 1;
  }
  size_t putString(const char* key, const char* value) {
    storage[key] = String(value);
    return storage[key].length();
  }
  size_t putString(const char* key, String value) {
    storage[key] = value;
    return value.length();
  }

  bool isKey(const char* key) { return storage.find(key) != storage.end(); }

  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return getInt(key, defaultValue); }
  uint32_t getInt(const char* key, int32_t defaultValue = 0) {
    auto it = storage.find(key);
    if (it != storage.end()) {
      try {
        return std::stol(it->second.str());
      } catch (...) {
        return defaultValue;
      }
    }
    return defaultValue;
  }
  bool getBool(const char* key, bool defaultValue = false) {
    auto it = storage.find(key);
    if (it != storage.end()) {
      std::string val = it->second.str();
      if (val == "true" || val == "1")
        return true;
      if (val == "false" || val == "0")
        return false;
    }
    return defaultValue;
  }
  size_t getString(const char* key, char* value, size_t maxLen) {
    auto it = storage.find(key);
    if (it != storage.end()) {
      String strVal = it->second;
      size_t copyLen = std::min((size_t)strVal.length(), maxLen - 1);
      strncpy(value, strVal.c_str(), copyLen);
      value[copyLen] = '\0';
      return copyLen;
    }
    if (maxLen > 0)
      value[0] = '\0';
    return 0;
  }
  String getString(const char* key, String defaultValue = String()) {
    auto it = storage.find(key);
    if (it != storage.end()) {
      return it->second;
    }
    return defaultValue;
  }

  //private:
  static std::map<String, String> storage;
};
#endif
