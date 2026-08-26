#ifndef PREFERENCES
#define PREFERENCES

#include <WString.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include <string>

/* In-memory stand-in for the ESP32 NVS Preferences API.
 *
 * This used to discard every write and return zero from every read, which made
 * anything that stores a setting untestable: a round-trip could not be observed
 * at all. It now keeps the values in a static map that survives for the process
 * - real NVS survives a reboot, so a store opened, closed and reopened must see
 * what the previous one wrote.
 *
 * Namespaces are kept apart the way NVS keeps them apart. emul_nvs_reset()
 * gives a test a clean device.
 */

namespace emul_nvs {

struct Value {
  enum Type { INT, UINT, BOOL, STRING } type;
  int32_t i32;
  uint32_t u32;
  bool b;
  String s;
};

// namespace -> key -> value
inline std::map<std::string, std::map<std::string, Value>>& store() {
  static std::map<std::string, std::map<std::string, Value>> s;
  return s;
}

}  // namespace emul_nvs

// NVS keys are capped at 15 characters plus a terminator; a longer one is
// rejected by the real driver. Modelled here so a key that would fail only on
// hardware fails in the suite instead.
constexpr size_t kNvsMaxKeyLength = 15;

inline bool emul_nvs_key_is_valid(const char* key) {
  return key != nullptr && key[0] != '\0' && strlen(key) <= kNvsMaxKeyLength;
}

// Wipes every namespace, as if the device had never been written to.
inline void emul_nvs_reset() {
  emul_nvs::store().clear();
}

// Number of keys held in a namespace, for asserting that a save actually wrote.
inline size_t emul_nvs_key_count(const char* ns) {
  auto it = emul_nvs::store().find(ns);
  return it == emul_nvs::store().end() ? 0 : it->second.size();
}

typedef enum {
  PT_I8,
  PT_U8,
  PT_I16,
  PT_U16,
  PT_I32,
  PT_U32,
  PT_I64,
  PT_U64,
  PT_STR,
  PT_BLOB,
  PT_INVALID
} PreferenceType;

class Preferences {
 public:
  Preferences() {}

  bool begin(const char* name, bool readOnly = false, const char* partition_label = NULL) {
    ns_ = name ? name : "";
    read_only_ = readOnly;
    open_ = true;
    return true;
  }

  void end() { open_ = false; }

  bool clear() {
    emul_nvs::store()[ns_].clear();
    return true;
  }

  // The per-entry type tag, as real NVS keeps it: putBool writes a u8 on the
  // device, so BOOL maps to PT_U8. The boot audit reads this.
  PreferenceType getType(const char* key) {
    const emul_nvs::Value* v = find(key);
    if (v == nullptr) {
      return PT_INVALID;
    }
    switch (v->type) {
      case emul_nvs::Value::INT:
        return PT_I32;
      case emul_nvs::Value::UINT:
        return PT_U32;
      case emul_nvs::Value::BOOL:
        return PT_U8;
      case emul_nvs::Value::STRING:
        return PT_STR;
    }
    return PT_INVALID;
  }

  size_t putInt(const char* key, int32_t value) {
    if (read_only_ || !emul_nvs_key_is_valid(key)) {
      return 0;
    }
    emul_nvs::Value v;
    v.type = emul_nvs::Value::INT;
    v.i32 = value;
    emul_nvs::store()[ns_][key] = v;
    return sizeof(int32_t);
  }

  size_t putUInt(const char* key, uint32_t value) {
    if (read_only_ || !emul_nvs_key_is_valid(key)) {
      return 0;
    }
    emul_nvs::Value v;
    v.type = emul_nvs::Value::UINT;
    v.u32 = value;
    emul_nvs::store()[ns_][key] = v;
    return sizeof(uint32_t);
  }

  size_t putBool(const char* key, bool value) {
    if (read_only_ || !emul_nvs_key_is_valid(key)) {
      return 0;
    }
    emul_nvs::Value v;
    v.type = emul_nvs::Value::BOOL;
    v.b = value;
    emul_nvs::store()[ns_][key] = v;
    return sizeof(bool);
  }

  size_t putString(const char* key, const char* value) { return putString(key, String(value)); }

  size_t putString(const char* key, String value) {
    if (read_only_ || !emul_nvs_key_is_valid(key)) {
      return 0;
    }
    emul_nvs::Value v;
    v.type = emul_nvs::Value::STRING;
    v.s = value;
    emul_nvs::store()[ns_][key] = v;
    return value.length();
  }

  bool isKey(const char* key) {
    auto ns = emul_nvs::store().find(ns_);
    return ns != emul_nvs::store().end() && ns->second.count(key) > 0;
  }

  bool remove(const char* key) {
    auto ns = emul_nvs::store().find(ns_);
    if (ns == emul_nvs::store().end()) {
      return false;
    }
    return ns->second.erase(key) > 0;
  }

  /* Typed reads honor the tag: real NVS refuses a getInt on a key stored via
   * putUInt and hands back the caller's default. The settings shadow audit and
   * the accessor layer both lean on exactly this, so the emulation has to get
   * it right or their tests prove nothing. */
  int32_t getInt(const char* key, int32_t defaultValue = 0) {
    const emul_nvs::Value* v = find(key);
    return (v != nullptr && v->type == emul_nvs::Value::INT) ? v->i32 : defaultValue;
  }

  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
    const emul_nvs::Value* v = find(key);
    return (v != nullptr && v->type == emul_nvs::Value::UINT) ? v->u32 : defaultValue;
  }

  bool getBool(const char* key, bool defaultValue = false) {
    const emul_nvs::Value* v = find(key);
    return (v != nullptr && v->type == emul_nvs::Value::BOOL) ? v->b : defaultValue;
  }

  size_t getString(const char* key, char* value, size_t maxLen) {
    const emul_nvs::Value* v = find(key);
    if (!v || v->type != emul_nvs::Value::STRING || value == nullptr || maxLen == 0) {
      return 0;
    }
    size_t n = v->s.length() < maxLen - 1 ? v->s.length() : maxLen - 1;
    memcpy(value, v->s.c_str(), n);
    value[n] = '\0';
    return n;
  }

  String getString(const char* key, String defaultValue = String()) {
    const emul_nvs::Value* v = find(key);
    return (v != nullptr && v->type == emul_nvs::Value::STRING) ? v->s : defaultValue;
  }

 private:
  const emul_nvs::Value* find(const char* key) {
    auto ns = emul_nvs::store().find(ns_);
    if (ns == emul_nvs::store().end()) {
      return nullptr;
    }
    auto it = ns->second.find(key);
    return it == ns->second.end() ? nullptr : &it->second;
  }

  std::string ns_;
  bool read_only_ = false;
  bool open_ = false;
};
#endif
