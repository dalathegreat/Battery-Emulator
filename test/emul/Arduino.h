#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>

#include "HardwareSerial.h"
#include "Print.h"
#include "esp-hal-gpio.h"

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

// Can be previously declared as a macro in stupid eModbus
#undef millis
unsigned long millis();

uint64_t micros();

int max(int a, int b);

#endif
