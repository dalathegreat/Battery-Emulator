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

#include "HardwareSerial.h"
#include "Print.h"
#include "esp-hal-gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

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

void delay(uint32_t ms);

typedef uint32_t TickType_t;

#endif
