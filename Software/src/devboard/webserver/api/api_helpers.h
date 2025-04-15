#ifndef API_HELPERS_H
#define API_HELPERS_H

#include <Arduino.h>

// Helper function to add a field to the JSON string
inline void addJsonField(String& json, const String& key, const String& value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  json += value;
  json += "\"";
  if (addComma) json += ",";
}

// Helper function to add a numeric field to the JSON string
template <typename T>
inline void addJsonNumber(String& json, const String& key, T value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += String(value);
  if (addComma) json += ",";
}

// Helper function to add a boolean field to the JSON string
inline void addJsonBool(String& json, const String& key, bool value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (addComma) json += ",";
}

#endif // API_HELPERS_H