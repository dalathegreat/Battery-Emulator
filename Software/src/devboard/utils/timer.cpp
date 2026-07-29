#include "timer.h"

MyTimer::MyTimer(uint32_t interval) : interval(interval) {
  previous_millis = millis();
}

bool MyTimer::elapsed(void) {
  uint32_t current_millis = millis();
  if (current_millis - previous_millis >= interval) {
    previous_millis = current_millis;
    return true;
  }
  return false;
}

void MyTimer::reset(void) {
  previous_millis = millis();
}

void MyTimer::set_interval(uint32_t interval) {
  this->interval = interval;
  reset();
}
