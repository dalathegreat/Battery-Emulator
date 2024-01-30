#include "timer.h"

MyTimer::MyTimer(unsigned long interval) : interval(interval), previousMillis(0) {}

bool MyTimer::elapsed() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
