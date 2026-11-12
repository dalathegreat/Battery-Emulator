#pragma once
#include "esp_private/periph_ctrl.h"
#include "hal/twai_ll.h"

// Replacement for periph_module_enable/disable/reset(PERIPH_TWAI_MODULE), removed in IDF 6.
static inline void twai_periph_enable(int group) {
  PERIPH_RCC_ATOMIC() {
    twai_ll_enable_bus_clock(group, true);
    twai_ll_reset_register(group);
  }
}

static inline void twai_periph_disable(int group) {
  PERIPH_RCC_ATOMIC() {
    twai_ll_enable_bus_clock(group, false);
  }
}

static inline void twai_periph_reset(int group) {
  PERIPH_RCC_ATOMIC() {
    twai_ll_reset_register(group);
  }
}