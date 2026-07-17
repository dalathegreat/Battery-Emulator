#pragma once

#include "Print.h"

#include <stddef.h>

class Printable {
 public:
  virtual ~Printable() {}
  virtual size_t printTo(Print& p) const = 0;
};
