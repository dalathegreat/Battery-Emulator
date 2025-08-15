#ifndef STREAM_H
#define STREAM_H

#include "Print.h"

class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
};

#endif
