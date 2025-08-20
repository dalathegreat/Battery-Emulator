#include "Arduino.h"

void delay(unsigned long ms) {}
void delayMicroseconds(unsigned long us) {}
int digitalRead(uint8_t pin) {
  return 0;
}
void digitalWrite(uint8_t pin, uint8_t val) {}
int analogRead(uint8_t pin) {
  return 0;
}

unsigned long micros() {
  return 0;
}
void pinMode(uint8_t pin, uint8_t mode) {}

int max(int a, int b) {
  return (a > b) ? a : b;
}

bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel) {
  return true;
}
bool ledcWrite(uint8_t pin, uint32_t duty) {
  return true;
}

void randomSeed(unsigned long) {}
long random(long) {
  return 7;
}
long random(long low, long high) {
  return low;
}
