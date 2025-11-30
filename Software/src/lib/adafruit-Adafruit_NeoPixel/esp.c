#include <Arduino.h>
#include "driver/rmt_tx.h"
// This code is adapted for the ESP-IDF v5.x RMT driver API
// WS2812 timing parameters (in nanoseconds)
#define WS2812_T0H_NS (400) 
#define WS2812_T0L_NS (850) 
#define WS2812_T1H_NS (800) 
#define WS2812_T1L_NS (450)
#define WS2812_RESET_US 280
#define WS2812_T0H_TICKS (WS2812_T0H_NS / 25)
#define WS2812_T0L_TICKS (WS2812_T0L_NS / 25)
#define WS2812_T1H_TICKS (WS2812_T1H_NS / 25)
#define WS2812_T1L_TICKS (WS2812_T1L_NS / 25)

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

void espShow(uint8_t pin, uint8_t* pixels, uint32_t numBytes) {
    static bool initialized = false;

    if (!initialized) {
        rmt_tx_channel_config_t tx_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = pin,
            .mem_block_symbols = 64,
            .resolution_hz = 40 * 1000 * 1000, // 40 MHz
            .trans_queue_depth = 4,
        };
        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

        rmt_bytes_encoder_config_t bytes_encoder_config = {
            .bit0 = { .level0 = 1, .duration0 = WS2812_T0H_NS / 25,
                      .level1 = 0, .duration1 = WS2812_T0L_NS / 25 },
            .bit1 = { .level0 = 1, .duration0 = WS2812_T1H_NS / 25,
                      .level1 = 0, .duration1 = WS2812_T1L_NS / 25 },
            .flags.msb_first = 1
        };
        ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder));

        ESP_ERROR_CHECK(rmt_enable(led_chan));
        initialized = true;
    }

    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, pixels, numBytes, &tx_config));
}
