#ifndef MD5Builder_h
#define MD5Builder_h

#include <stdint.h>
#include "Stream.h"
#include "WString.h"

class MD5Builder {
 public:
  void begin();
  void add(const uint8_t* data, size_t len);
  void addHexString(const char* data);
  bool addStream(Stream& stream, const size_t maxLen);
  void calculate();
  void getBytes(uint8_t* output);
  void getChars(char* output);
  String toString();

 private:
  void md5_init();
  void md5_update(const uint8_t* input, size_t length);
  void md5_final(uint8_t digest[16]);

  void encode(uint8_t* output, const uint32_t* input, size_t len);
  void decode(uint32_t* output, const uint8_t* input, size_t len);
  void transform(const uint8_t block[64]);

  uint32_t state[4];   // A, B, C, D
  uint64_t count;      // number of bits
  uint8_t buffer[64];  // input buffer
  uint8_t digest[16];  // result
  bool finalized = false;
};

#endif
