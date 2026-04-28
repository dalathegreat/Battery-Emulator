#ifndef STREAM_H
#define STREAM_H

#include "Print.h"

class Stream : public Print {
 public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual int readBytes(char* buffer, size_t length) { return 0; }
  virtual int readBytes(uint8_t* buffer, size_t length) { return 0; }
};

#endif
