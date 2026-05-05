#ifndef PREFERENCES
#define PREFERENCES

#include <WString.h>
#include <stdint.h>

class Preferences {

 public:
  Preferences() {}

  bool begin(const char* name, bool readOnly = false, const char* partition_label = NULL);
  void end();
  bool clear();

  size_t putUInt(const char* key, uint32_t value) { return 0; }
  size_t putBool(const char* key, bool value) { return 0; }
  size_t putString(const char* key, const char* value) { return 0; }
  size_t putString(const char* key, String value) { return 0; }

  bool isKey(const char* key) { return false; }

  uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return 0; }
  bool getBool(const char* key, bool defaultValue = false) { return false; }
  size_t getString(const char* key, char* value, size_t maxLen) { return 0; }
  String getString(const char* key, String defaultValue = String()) { return String(); }
};
#endif
