#include <Arduino.h>
#include "driver/rmt_tx.h"
// This code is adapted for the ESP-IDF v5.x RMT driver API
// WS2812 timing parameters (in nanoseconds)
#define T0H_TICKS 16  // 400ns / 25ns
#define T0L_TICKS 34  // 850ns / 25ns
#define T1H_TICKS 32  // 800ns / 25ns
#define T1L_TICKS 18  // 450ns / 25ns

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
} rmt_led_strip_encoder_t;

void espShow(uint8_t pin, uint8_t* pixels, uint32_t numBytes) {
    static bool initialized = false;

    if (!initialized) {
        rmt_tx_channel_config_t tx_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = pin,
            .mem_block_symbols = 64,
            .resolution_hz = 40000000,
            .trans_queue_depth = 4,
        };
        if (rmt_new_tx_channel(&tx_chan_config, &led_chan) != ESP_OK) return;

        rmt_bytes_encoder_config_t bytes_encoder_config = {
            .bit0 = {
                .level0 = 1,
                .duration0 = T0H_TICKS,
                .level1 = 0,
                .duration1 = T0L_TICKS
            },
            .bit1 = {
                .level0 = 1,
                .duration0 = T1H_TICKS,
                .level1 = 0,
                .duration1 = T1L_TICKS
            },
            .flags.msb_first = 1
        };
        if (rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder) != ESP_OK) {
            rmt_del_channel(led_chan);
            return;
        }

        if (rmt_enable(led_chan) != ESP_OK) {
            rmt_del_encoder(led_encoder);
            rmt_del_channel(led_chan);
            return;
        }
        initialized = true;
    }

    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    rmt_transmit(led_chan, led_encoder, pixels, numBytes, &tx_config);
}
