#ifndef WSTRING_H
#define WSTRING_H

#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class String {
 private:
  std::string data;

 public:
  // Constructors
  String() = default;
  String(const char* s) : data(s) {}
  String(const std::string& s) : data(s) {}
  String(const String& other) = default;
  String(String&& other) = default;

  // Numeric constructors (Arduino-style)
  String(uint64_t value) { data = std::to_string(value); }
  String(int value) { data = std::to_string(value); }
  String(unsigned int value) { data = std::to_string(value); }
  String(long value) { data = std::to_string(value); }
  String(float value) { data = std::to_string(value); }
  String(double value) { data = std::to_string(value); }

  String(float value, unsigned int decimalPlaces) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimalPlaces) << value;
    data = oss.str();
  }

  // Assignment operators
  String& operator=(const String& other) = default;
  String& operator=(String&& other) = default;

  // Conversion operator to std::string
  operator std::string() const { return data; }

  // Accessor
  const std::string& str() const { return data; }

  // Comparison operators
  bool operator==(const String& rhs) const { return data == rhs.data; }

  // Concatenation
  String operator+(const String& rhs) const { return String(data + rhs.data); }

  String operator+(const std::string& rhs) const { return String(data + rhs); }

  String operator+(const char* rhs) const { return String(data + std::string(rhs)); }

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

  // Arduino-like methods (example)
  int length() const { return static_cast<int>(data.length()); }
  const char* c_str() const { return data.c_str(); }

  // Friend functions to allow std::string + String
  friend String operator+(const std::string& lhs, const String& rhs) { return String(lhs + rhs.data); }

  friend String operator+(const char* lhs, const String& rhs) { return String(std::string(lhs) + rhs.data); }

  friend std::ostream& operator<<(std::ostream& os, const String& s) { return os << s.data; }
};
#endif
