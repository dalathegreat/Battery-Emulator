#ifndef STREAM_H
#define STREAM_H

#include "Print.h"

class Stream : public Print {
 public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  // flush() is already inherited from Print
};

#endif
