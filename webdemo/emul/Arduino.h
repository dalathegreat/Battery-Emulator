#pragma once

#include "../../test/emul/Arduino.h"

#include "../../test/emul/WString.h"

#include "soc/gpio_num.h"

// where should these go?
#define IRAM_ATTR
#define ESP_ARDUINO_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | patch)
#define DR_REG_CAN_BASE 0x12345678
#define TWAI_TX_IDX 0
#define TWAI_RX_IDX 1
#define ETS_TWAI_INTR_SOURCE 0
#define PERIPH_TWAI_MODULE 0
typedef int periph_module_t;
typedef int intr_handle_t;
#define ESP_OK 0

static inline int getApbFrequency() {
  return 80000000;  // 80 MHz for ESP32
}
