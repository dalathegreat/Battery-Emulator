#ifndef PRINT_H
#define PRINT_H

#include <WString.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Minimal copy of Arduino Print class

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print {
 public:
  size_t vprintf(const char* format, va_list arg);

  virtual size_t write(uint8_t) = 0;
  size_t write(const char* str) {
    if (str == NULL) {
      return 0;
    }
    return write((const uint8_t*)str, strlen(str));
  }
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) {
      n += write(*buffer++);
    }
    return n;
  }
  size_t write(const char* buffer, size_t size) { return write((const uint8_t*)buffer, size); }

  size_t print(const String& s);

  size_t println(int a, int b = DEC) { return print((long)a, b); }
  size_t println(const String& s);
  size_t print(const char[]);
  size_t print(unsigned long, int = DEC);
  size_t printf(const char* format, ...);
  size_t println(const char[]);
  size_t println(void);

  virtual void flush() {}

 private:
  size_t printNumber(unsigned long n, uint8_t base);
};

#endif
