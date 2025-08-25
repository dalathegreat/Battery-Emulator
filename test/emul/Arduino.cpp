#include "Arduino.h"

void delay(unsigned long ms) {}
void delayMicroseconds(unsigned long us) {}
int digitalRead(uint8_t pin) {
  return 0;
}
void digitalWrite(uint8_t pin, uint8_t val) {}
unsigned long micros() {
  return 0;
}
void pinMode(uint8_t pin, uint8_t mode) {}

int max(int a, int b) {
  return (a > b) ? a : b;
}

ESPClass ESP;
