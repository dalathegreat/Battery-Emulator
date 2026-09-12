#ifndef CHECKED_HTML_H
#define CHECKED_HTML_H

#include <Arduino.h>
#include <utility>

// Retain the first allocation failure instead of returning partially built HTML.
// Appends accept text (const char* or String); convert numeric values explicitly.
class CheckedHtml : public String {
 public:
  bool reserve(unsigned int size) {
    valid = valid && String::reserve(size);
    return valid;
  }
  CheckedHtml& operator+=(const char* value) {
    valid = valid && concat(value);
    return *this;
  }
  CheckedHtml& operator+=(const String& value) {
    valid = valid && value && concat(value);
    return *this;
  }
  bool good() const { return valid; }
  String take() {
    if (!valid) {
      return String();
    }
    return std::move(static_cast<String&>(*this));
  }

 private:
  bool valid = true;
};

#endif
