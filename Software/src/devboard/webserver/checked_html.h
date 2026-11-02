#ifndef CHECKED_HTML_H
#define CHECKED_HTML_H

#include <Arduino.h>
#include <utility>

// Retain the first allocation failure instead of returning partially built HTML.
//
// Appends accept text - const char*, String or a single char. Numbers are deliberately not
// accepted: wrap them as String(value, precision) so the formatting is visible at the call site.
//
// Declaring any operator+= here hides every one String defines, so each text form has to be
// named explicitly, and the numeric ones have to be deleted rather than merely left out. Without
// the deletions, `html += 5` would convert 5 to char and silently append a control character
// instead of failing to compile.
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
  CheckedHtml& operator+=(char value) {
    valid = valid && concat(value);
    return *this;
  }

  CheckedHtml& operator+=(unsigned char) = delete;
  CheckedHtml& operator+=(int) = delete;
  CheckedHtml& operator+=(unsigned int) = delete;
  CheckedHtml& operator+=(long) = delete;
  CheckedHtml& operator+=(unsigned long) = delete;
  CheckedHtml& operator+=(long long) = delete;
  CheckedHtml& operator+=(unsigned long long) = delete;
  CheckedHtml& operator+=(float) = delete;
  CheckedHtml& operator+=(double) = delete;
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
