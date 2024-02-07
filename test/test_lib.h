#ifndef __TEST_LIB_H__
#define __TEST_LIB_H__

#include <stdint.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "config.h"
#include "microtest.h"

using namespace std;

class MySerial;

extern unsigned long testlib_millis;
extern MySerial Serial;
extern uint8_t bms_status;

/* Mock millis() */
static inline unsigned long millis(void) {
  return testlib_millis;
}

/* Mock Serial class */
class MySerial {
 public:
  size_t println(const char* s) {
    return print(s, true);  // Call print with newline argument true
  }

  size_t print(const char* s) {
    return print(s, false);  // Call print with newline argument false
  }

 private:
  size_t print(const char* s, bool newline) {
    size_t length = printf("%s", s);  // Print the string without newline
    if (newline) {
      printf("\n");  // Add a newline if specified
      length++;      // Increment length to account for the added newline character
    }
    return length;  // Return the total length printed
  }
};

#endif
