#ifndef PREFERENCES
#define PREFERENCES

#include <WString.h>
#include <stdint.h>

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

  bool begin(const char* name, bool readOnly = false, const char* partition_label = NULL);
  void end();
  bool clear();

  size_t putInt(const char* key, int32_t value) { return 0; }
  size_t putUInt(const char* key, uint32_t value) { return 0; }
  size_t putBool(const char* key, bool value) { return 0; }
  size_t putString(const char* key, const char* value) { return 0; }
  size_t putString(const char* key, String value) { return 0; }

  bool isKey(const char* key) { return false; }
  PreferenceType getType(const char* key) { return PT_INVALID; }
  bool remove(const char* key) { return true; }

  int32_t getInt(const char* key, int32_t defaultValue = 0) { return 0; }
  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return 0; }
  bool getBool(const char* key, bool defaultValue = false) { return false; }
  size_t getString(const char* key, char* value, size_t maxLen) { return 0; }
  String getString(const char* key, String defaultValue = String()) { return String(); }
};
#endif
