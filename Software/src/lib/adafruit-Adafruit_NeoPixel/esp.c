// Implements the RMT peripheral on Espressif SoCs
// Copyright (c) 2020 Lucian Copeland for Adafruit Industries

/* Uses code from Espressif RGB LED Strip demo and drivers
 * Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Arduino.h>
#include "driver/rmt.h"

// This code is adapted from the ESP-IDF v3.4 RMT "led_strip" example, altered
// to work with the Arduino version of the ESP-IDF (3.2)

#define WS2812_T0H_NS (400)
#define WS2812_T0L_NS (850)
#define WS2812_T1H_NS (800)
#define WS2812_T1L_NS (450)

static uint32_t t0h_ticks = 0;
static uint32_t t1h_ticks = 0;
static uint32_t t0l_ticks = 0;
static uint32_t t1l_ticks = 0;

static void IRAM_ATTR ws2812_rmt_adapter(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num,
                                         size_t* translated_size, size_t* item_num) {
  if (src == NULL || dest == NULL) {
    *translated_size = 0;
    *item_num = 0;
    return;
  }
  const rmt_item32_t bit0 = {{{t0h_ticks, 1, t0l_ticks, 0}}};  //Logical 0
  const rmt_item32_t bit1 = {{{t1h_ticks, 1, t1l_ticks, 0}}};  //Logical 1
  size_t size = 0;
  size_t num = 0;
  uint8_t* psrc = (uint8_t*)src;
  rmt_item32_t* pdest = dest;
  while (size < src_size && num < wanted_num) {
    for (int i = 0; i < 8; i++) {
      // MSB first
      if (*psrc & (1 << (7 - i))) {
        pdest->val = bit1.val;
      } else {
        pdest->val = bit0.val;
      }
      num++;
      pdest++;
    }
    size++;
    psrc++;
  }
  *translated_size = size;
  *item_num = num;
}

static bool rmt_initialized = false;

void espShow(uint8_t pin, uint8_t* pixels, uint32_t numBytes) {
  if (rmt_initialized == false) {
    // Reserve channel
    rmt_channel_t channel = 0;

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(pin, channel);
    config.clk_div = 2;

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    // Convert NS timings to ticks
    uint32_t counter_clk_hz = 0;

    rmt_get_counter_clock(channel, &counter_clk_hz);

    // NS to tick converter
    float ratio = (float)counter_clk_hz / 1e9;

    t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);

    // Initialize automatic timing translator
    rmt_translator_init(0, ws2812_rmt_adapter);
    rmt_initialized = true;
  }

  // Write and wait to finish
  rmt_write_sample(0, pixels, (size_t)numBytes, false);
}
