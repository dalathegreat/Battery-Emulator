#ifndef __MYTIMER_H__
#define __MYTIMER_H__

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

class MyTimer {
 public:
  /** Default constructor */
  MyTimer() : interval(0), previous_millis(0) {}

  /** interval in ms */
  MyTimer(unsigned long interval);
  /** Returns true and resets the timer if it has elapsed */
  bool elapsed(void);
  void reset(void);
  void set_interval(unsigned long interval);

  unsigned long interval;
  unsigned long previous_millis;

 private:
};

#endif  // __MYTIMER_H__
