#ifndef STREAM_H
#define STREAM_H

#include "Print.h"

class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual size_t readBytes(char* buffer, size_t length) {
    size_t count = 0;
    while (count < length) {
      int c = read();
      if (c < 0) {
        break;
      }
      *buffer++ = (char)c;
      count++;
    }
    return count;
  }
  virtual size_t readBytes(uint8_t* buffer, size_t length) { return readBytes((char*)buffer, length); }
};

#endif
