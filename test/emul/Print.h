#ifndef PRINT_H
#define PRINT_H

#include <stddef.h>
#include <stdint.h>

class Print {
 public:
  virtual void flush() {}
  virtual void print(const char* str) {}
  virtual void print(int num) {}
  virtual void printf(const char* format, ...) {}
  virtual void println(const char* str) {}
  virtual size_t write(uint8_t) { return 0; }
  virtual size_t write(const char* s) { return 0; }
  virtual size_t write(const uint8_t* buffer, size_t size) { return 0; }
};

#endif
