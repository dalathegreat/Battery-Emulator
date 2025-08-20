#ifndef ARDUINO_H
#define ARDUINO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "HardwareSerial.h"
#include "Print.h"

#include "esp-hal-gpio.h"

typedef uint8_t byte;

#define word(h, l) ((h & 0xff) << 8 | (l & 0xff))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

long random(long);
long random(long, long);
void randomSeed(unsigned long);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);

unsigned long micros();
// Can be previously declared as a macro in stupid eModbus
#undef millis
unsigned long millis();

void delay(unsigned long ms);
void delayMicroseconds(unsigned long us);
int max(int a, int b);

bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel);
bool ledcWrite(uint8_t pin, uint32_t duty);

#endif
