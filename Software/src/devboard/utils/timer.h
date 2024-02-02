#ifndef __MYTIMER_H__
#define __MYTIMER_H__

#include <Arduino.h>

class MyTimer {
 public:
  /** interval in ms */
  MyTimer(unsigned long interval);
  /** Returns true and resets the timer if it has elapsed */
  bool elapsed();

 private:
  unsigned long interval;
  unsigned long previousMillis;
};

#endif  // __MYTIMER_H__
