#include "Preferences.h"

#include <fstream>

bool Preferences::begin(const char* name, bool read_only) {
  filename = name;
  readonly = read_only;

  std::ifstream file(filename);

  if (file) {
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    deserializeJson(doc, contents.c_str());  // Initialize with an empty JSON object
  }
  return true;
}

void Preferences::end() {
  if (!readonly) {
    std::ostringstream buffer;
    serializeJson(doc, buffer);
    std::ofstream file(filename);
    if (file) {
      file << buffer.str();
    }
  }
}

bool Preferences::getBool(const char* key, bool defaultValue) {
  if (doc.containsKey(key)) {
    return doc[key].as<bool>();
  }

  return defaultValue;
}

size_t Preferences::getString(const char* key, char* value, size_t maxLen) {
  if (doc.containsKey(key)) {
    String str = doc[key].as<String>();
    size_t len = str.length();
    if (len < maxLen) {
      strncpy(value, str.c_str(), len);
      value[len] = '\0';  // Null-terminate the string
      return len;
    }
  }
  return 0;
}

uint32_t Preferences::getUInt(const char* key, uint32_t defaultValue) {
  if (doc.containsKey(key)) {
    return doc[key].as<uint32_t>();
  }
  return defaultValue;
}

size_t Preferences::putBool(const char* key, bool value) {
  doc[key] = value;
  return 0;
}

size_t Preferences::putString(const char* key, String value) {
  doc[key] = value;
  return 0;
}

size_t Preferences::putUInt(const char* key, uint32_t value) {
  doc[key] = value;
  return 0;
}

size_t Preferences::putString(const char* key, const char* value) {
  doc[key] = value;
  return 0;
}
