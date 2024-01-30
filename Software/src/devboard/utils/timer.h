#ifndef __MYTIMER_H__
#define __MYTIMER_H__

#include <Arduino.h>

class MyTimer {
public:
  MyTimer(unsigned long interval);
  bool elapsed();

private:
  unsigned long interval;
  unsigned long previousMillis;
};

#endif  // __MYTIMER_H__