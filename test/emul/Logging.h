#pragma once

#include <cstdarg>
#include <cstdio>

namespace test {
class Logging {
 public:
  static void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }

  static void println(const char* message) { printf("%s\n", message); }
  static void print(const char* message) { printf("%s", message); }
};
}  // namespace test

using Logging = test::Logging;
extern Logging logging;
