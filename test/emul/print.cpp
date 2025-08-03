#include "Print.h"
#include <stdarg.h>
#include <stdlib.h>

size_t Print::println(const char c[]) {
  size_t n = print(c);
  n += println();
  return n;
}

size_t Print::println(void) {
  return print("\r\n");
}

size_t Print::print(const char str[]) {
  return write(str);
}

size_t Print::printf(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  size_t ret = vprintf(format, arg);
  va_end(arg);
  return ret;
}

size_t Print::vprintf(const char* format, va_list arg) {
  char loc_buf[64];
  char* temp = loc_buf;
  va_list copy;
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if (len < 0) {
    va_end(arg);
    return 0;
  }
  if (len >= (int)sizeof(loc_buf)) {  // comparison of same sign type for the compiler
    temp = (char*)malloc(len + 1);
    if (temp == NULL) {
      va_end(arg);
      return 0;
    }
    len = vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
  len = write((uint8_t*)temp, len);
  if (temp != loc_buf) {
    free(temp);
  }
  return len;
}

size_t Print::print(unsigned long n, int base) {
  if (base == 0) {
    return write(n);
  } else {
    return printNumber(n, base);
  }
}

size_t Print::printNumber(unsigned long n, uint8_t base) {
  char buf[8 * sizeof(n) + 1];  // Assumes 8-bit chars plus zero byte.
  char* str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) {
    base = 10;
  }

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while (n);

  return write(str);
}

size_t Print::print(const String& s) {
  return write(s.c_str(), s.length());
}

size_t Print::println(const String& s) {
  size_t n = print(s);
  n += println();
  return n;
}
