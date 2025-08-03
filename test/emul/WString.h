#ifndef WSTRING_H
#define WSTRING_H

#include <stdint.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int strcasecmp(const char* s1, const char* s2);

class __FlashStringHelper;
//#define FPSTR(str_pointer) (reinterpret_cast<const __FlashStringHelper *>(str_pointer))
#define FPSTR(str_pointer) (str_pointer)
#define F(string_literal) (FPSTR(PSTR(string_literal)))

class String {
 protected:
  std::string data;

 public:
  // Constructors
  String() = default;
  String(const char* s) : data(s ? s : "") {}
  String(const std::string& s) : data(s) {}
  String(const String& other) = default;
  String(String&& other) = default;
  String(const char* data, size_t len);

  // Numeric constructors (Arduino-style)
  String(uint64_t value) { data = std::to_string(value); }
  String(uint32_t value) { data = std::to_string(value); }
  String(int value) { data = std::to_string(value); }
  String(unsigned long value) { data = std::to_string(value); }
  String(long value) { data = std::to_string(value); }
  String(float value) { data = std::to_string(value); }
  String(double value) { data = std::to_string(value); }

  bool reserve(unsigned int size) {
    // In C++ std::string manages its own memory, so we don't need to implement this.
    // This is a no-op for std::string.
    return true;
  }

  String(float value, unsigned int decimalPlaces) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimalPlaces) << value;
    data = oss.str();
  }

  // Assignment operators
  String& operator=(const String& other) = default;
  String& operator=(String&& other) = default;

  bool operator!=(const String& other) const { return data != other.data; }

  // Conversion operator to std::string
  operator std::string() const { return data; }

  // Accessor
  const std::string& str() const { return data; }

  void clear() { data.clear(); }

  void toLowerCase() {
    for (auto& c : data) {
      c = static_cast<char>(tolower(c));
    }
  }

  char charAt(unsigned int index) const { return data[index]; }

  double toDouble() const { return std::stod(data); }

  bool operator<(const String& rhs) const { return data < rhs.data; }

  // Concatenation
  String operator+(const String& rhs) const { return String(data + rhs.data); }

  String operator+(const std::string& rhs) const { return String(data + rhs); }

  String operator+(const char* rhs) const { return String(data + std::string(rhs)); }

  String operator+(char c) const { return String(data + std::string(1, c)); }

  void replace(const String& find, const String& replace) {
    size_t start_pos = data.find(find.data);
    if (start_pos != std::string::npos) {
      data.replace(start_pos, find.length(), replace.data);
    }
  }
  // Append
  String& operator+=(const String& rhs) {
    data += rhs.data;
    return *this;
  }

  String& operator+=(const std::string& rhs) {
    data += rhs;
    return *this;
  }

  String& operator+=(const char* rhs) {
    data += rhs;
    return *this;
  }

  bool equals(const String& s2) const {
    if (!data.empty() && !s2.data.empty()) {
      return data == s2.data;
    }
    return data.empty() && s2.data.empty();
  }

  bool equals(const char* cstr) const {
    if (!data.empty() && cstr) {
      return data == cstr;
    }
    return data.empty() && !cstr;
  }

  bool equalsIgnoreCase(const String& s2) const {
    if (!data.empty() && !s2.data.empty()) {
      return strcasecmp(data.c_str(), s2.data.c_str()) == 0;
    }
    return data.empty() && s2.data.empty();
  }

  bool operator==(const String& rhs) const { return data == rhs.data; }

  bool startsWith(const String& prefix) const { return data.rfind(prefix.data, 0) == 0; }

  bool startsWith(const char* prefix) const {
    if (!data.empty() && prefix) {
      return data.rfind(prefix, 0) == 0;
    }
    return data.empty() && !prefix;
  }

  bool endsWith(const char* postfix) const {
    if (!data.empty() && postfix) {
      size_t len = strlen(postfix);
      return data.compare(data.size() - len, len, postfix) == 0;
    }
    return data.empty() && !postfix;
  }

  bool endsWith(const String& s2) const {
    if (s2.data.empty() || data.empty() || s2.data.size() > data.size()) {
      return false;
    }
    size_t pos = data.rfind(s2.data);
    return (pos != std::string::npos && pos == data.size() - s2.data.size());
  }

  int lastIndexOf(const String& s2) const {
    if (s2.data.empty() || data.empty() || s2.data.size() > data.size()) {
      return -1;
    }
    size_t pos = data.rfind(s2.data);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }

  bool isEmpty(void) const { return data.empty(); }

  bool operator==(const char* rhs) { return data == rhs; }

  char operator[](unsigned int index) { return data[index]; }

  int indexOf(char c) const { return data.find(c); }

  int indexOf(char ch, unsigned int fromIndex) {
    if (fromIndex >= data.length()) {
      return -1;  // Out of bounds
    }
    size_t pos = data.find(ch, fromIndex);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }
  int indexOf(const String& str) const { return data.find(str.data); }

  int indexOf(const String& str, unsigned int fromIndex) const {
    if (fromIndex >= data.length()) {
      return -1;  // Out of bounds
    }
    size_t pos = data.find(str.data, fromIndex);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }

  void trim(void) {
    data.erase(data.find_last_not_of(" \n\r\t") + 1);
    data.erase(0, data.find_first_not_of(" \n\r\t"));
  }

  long toInt(void) const { return std::stol(data); }
  float toFloat(void) const { return std::stof(data); }

  String substring(unsigned int left, unsigned int right) const {
    if (left >= data.length()) {
      return String();
    }
    if (right > data.length()) {
      right = data.length();
    }
    if (left >= right) {
      return String();
    }
    return String(data.substr(left, right - left));
  }

  String substring(unsigned int left) const {
    if (left >= data.length()) {
      return String();
    }
    return String(data.substr(left));
  }

  // Arduino-like methods (example)
  size_t length() const { return static_cast<int>(data.length()); }
  const char* c_str() const { return data.c_str(); }

  // Friend functions to allow std::string + String
  friend String operator+(const std::string& lhs, const String& rhs) { return String(lhs + rhs.data); }

  friend String operator+(const char* lhs, const String& rhs) { return String(std::string(lhs) + rhs.data); }

  friend std::ostream& operator<<(std::ostream& os, const String& s) { return os << s.data; }

  bool concat(const String& s) {
    data += s.data;
    return true;
  }
  bool concat(const char* cstr) {
    if (cstr) {
      data += cstr;
      return true;
    }
    return false;
  }
  bool concat(char c) {
    data += c;
    return true;
  }

  bool concat(int num) {
    data += std::to_string(num);
    return true;
  }

  bool concat(uint8_t d) {
    data += std::to_string(d);
    return true;
  }

  void append(const char* str, size_t len) {
    if (str && len > 0) {
      data.append(str, len);
    }
  }
};

extern const String emptyString;

#endif
