#ifndef PREFERENCES
#define PREFERENCES

#include <WString.h>
#include <stdint.h>
#include <map>
#include <string>
#include <utility>

class Preferences {

 public:
  Preferences() {}

  bool begin(const char* name, bool readOnly = false, const char* partition_label = NULL) {
    namespace_name = name;
    read_only = readOnly;
    open = true;
    ++begin_calls;
    return true;
  }
  void end() { open = false; }
  bool clear() {
    if (!open || read_only) {
      return false;
    }
    for (auto it = integers.begin(); it != integers.end();) {
      if (it->first.first == namespace_name) {
        it = integers.erase(it);
      } else {
        ++it;
      }
    }
    return true;
  }

  size_t putInt(const char* key, int32_t value) { return putInteger(key, static_cast<uint32_t>(value), false); }
  size_t putUInt(const char* key, uint32_t value) { return putInteger(key, value, true); }
  size_t putBool(const char* key, bool value) { return putInteger(key, value ? 1U : 0U, true); }
  size_t putString(const char* key, const char* value) { return 0; }
  size_t putString(const char* key, String value) { return 0; }

  bool isKey(const char* key) { return open && integers.count({namespace_name, key}) != 0; }
  bool remove(const char* key) { return open && !read_only && integers.erase({namespace_name, key}) != 0; }

  int32_t getInt(const char* key, int32_t defaultValue = 0) {
    auto it = integers.find({namespace_name, key});
    return open && it != integers.end() && !it->second.is_unsigned ? static_cast<int32_t>(it->second.value)
                                                                   : defaultValue;
  }
  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
    auto it = integers.find({namespace_name, key});
    return open && it != integers.end() && it->second.is_unsigned ? it->second.value : defaultValue;
  }
  bool getBool(const char* key, bool defaultValue = false) {
  auto it = integers.find({namespace_name, key});
  return open && it != integers.end() ? it->second.value != 0 : defaultValue;
}
  size_t getString(const char* key, char* value, size_t maxLen) { return 0; }
  String getString(const char* key, String defaultValue = String()) { return String(); }

  // Test controls: numeric NVS survives destroying/recreating Preferences.
  static void reset() {
    integers.clear();
    writes.clear();
    begin_calls = 0;
  }
  static unsigned writeCount(const char* name, const char* key) { return writes[{name, key}]; }
  inline static unsigned begin_calls = 0;

 private:
  struct Integer {
    uint32_t value;
    bool is_unsigned;
  };
  using Key = std::pair<std::string, std::string>;
  inline static std::map<Key, Integer> integers;
  inline static std::map<Key, unsigned> writes;
  std::string namespace_name;
  bool open = false;
  bool read_only = false;

  size_t putInteger(const char* key, uint32_t value, bool is_unsigned) {
    if (!open || read_only) {
      return 0;
    }
    integers[{namespace_name, key}] = {value, is_unsigned};
    ++writes[{namespace_name, key}];
    return sizeof(value);
  }
};
#endif
