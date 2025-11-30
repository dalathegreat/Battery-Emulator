#ifndef PRINT_H
#define PRINT_H

class Print {
 public:
  virtual void flush() {}
  void printf(const char* format, ...) {}
  virtual size_t write(uint8_t) { return 0; }
  virtual size_t write(const char* s) { return 0; }
  virtual size_t write(const uint8_t* buffer, size_t size) { return 0; }
};

#endif
