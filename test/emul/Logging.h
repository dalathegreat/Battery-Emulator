#ifndef LOGGING_H
#define LOGGING_H

#include <cstdarg>
#include <cstdio>

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

// Global logging instance
extern Logging logging;

#endif
