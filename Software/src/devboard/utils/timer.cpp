#include "timer.h"

MyTimer::MyTimer(unsigned long interval) : interval(interval) {
  previous_millis = millis();
}

bool MyTimer::elapsed(void) {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= interval) {
    previous_millis = current_millis;
    return true;
  }
  return false;
}

void MyTimer::reset(void) {
  previous_millis = millis();
}

void MyTimer::set_interval(unsigned long interval) {
  this->interval = interval;
  reset();
}
