#ifndef ARDUINO_H
#define ARDUINO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "HardwareSerial.h"
#include "Print.h"

#include "esp-hal-gpio.h"

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

unsigned long micros();
// Can be previously declared as a macro in stupid eModbus
#undef millis
unsigned long millis();

void delay(unsigned long ms);
void delayMicroseconds(unsigned long us);
int max(int a, int b);

#endif
