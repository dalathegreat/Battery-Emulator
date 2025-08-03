#ifndef Printable_h
#define Printable_h

#include <stdlib.h>

class Print;

class Printable {
 public:
  virtual ~Printable() {}
  virtual size_t printTo(Print& p) const = 0;
};

#endif
