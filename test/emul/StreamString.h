#ifndef STREAMSTRING_H_
#define STREAMSTRING_H_
#include "Stream.h"
#include "WString.h"

class StreamString : public Stream, public String {
 public:
  size_t write(const uint8_t* buffer, size_t size) override {
    if (buffer == nullptr || size == 0) {
      return 0;
    }
    size_t oldSize = this->length();
    append((const char*)buffer, size);
    return this->length() - oldSize;
  }
  size_t write(uint8_t data) override { return write(&data, 1); }

  size_t readBytes(char* buffer, size_t length) {
    size_t count = 0;
    while (count < length && readIndex < this->length()) {
      buffer[count++] = this->data[readIndex++];
    }
    return count;
  }

  int available() override { return this->length() - readIndex; };
  int read() override {
    if (readIndex < this->length()) {
      return this->data[readIndex++];
    }
    return -1;  // EOF
  }
  int peek() override {
    if (readIndex < this->length()) {
      return this->data[readIndex];
    }
    return -1;  // EOF
  }
  void flush() override {
    // No-op for StreamString
  }

 private:
  int readIndex = 0;
};

#endif
