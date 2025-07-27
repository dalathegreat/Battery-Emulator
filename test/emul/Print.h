#ifndef PRINT_H
#define PRINT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

class Print {
 public:
  size_t vprintf(const char* format, va_list arg);

  size_t printf(const char* format, ...);
  size_t println(const char[]);
};

#endif
