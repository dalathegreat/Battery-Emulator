#ifndef ARDUINO_H
#define ARDUINO_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <cmath>

#include "HardwareSerial.h"
#include "Print.h"
#include "Printable.h"
#include "esp-hal-gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"

#include "WString.h"

#include "esp32-hal-log.h"

typedef uint8_t byte;
typedef unsigned int word;

uint16_t makeWord(uint16_t w);
uint16_t makeWord(uint8_t h, uint8_t l);

#define word(...) makeWord(__VA_ARGS__)

#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

uint16_t analogRead(uint8_t pin);

long random(long);
long random(long, long);
void randomSeed(unsigned long);

// Can be previously declared as a macro in stupid eModbus
#undef millis
unsigned long millis();

uint64_t micros();

int max(int a, int b);
int min(int a, int b);

typedef int esp_err_t;

esp_err_t esp_task_wdt_add(TaskHandle_t task_handle);
esp_err_t esp_task_wdt_reset(void);

typedef struct {
  uint32_t timeout_ms; /**< TWDT timeout duration in milliseconds */
  uint32_t
      idle_core_mask; /**< Bitmask of the core whose idle task should be subscribed on initialization where 1 << i means that core i's idle task will be monitored by the TWDT */
  bool trigger_panic; /**< Trigger panic when timeout occurs */
} esp_task_wdt_config_t;

esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t* config);

void delay(uint32_t ms);

float temperatureRead();

uint32_t ledcWriteTone(uint8_t pin, uint32_t freq);
bool ledcWrite(uint8_t pin, uint32_t duty);
bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, uint8_t channel);

void delayMicroseconds(uint32_t us);

/*size_t strlen_P(const char *s);
int strncmp_P (const char *, const char *, size_t);
int	strcmp_P (const char *, const char *);
int	memcmp_P (const void *, const void *, size_t);
void *memcpy_P (void *, const void *, size_t);
*/

#define pgm_read_byte(addr) (*(const unsigned char*)(addr))

#define PROGMEM
#define PGM_P const char*
#define PSTR(s) (s)
#define __unused

#define snprintf_P snprintf
#define strlen_P strlen
#define memcpy_P memcpy
#define sprintf_P sprintf

char* strndup(const char* str, size_t size);

class EspClass {
 public:
  void restart();
  size_t getFlashChipSize() {
    // This is a placeholder for the actual implementation
    // that retrieves the flash chip size.
    return 4 * 1024 * 1024;  // Example: returning 4MB
  }
};

extern EspClass ESP;

struct tm* localtime_r(const time_t*, struct tm*);

int strcasecmp(const char*, const char*);

#endif
