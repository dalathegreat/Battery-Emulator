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

static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                 const void *primary_data, size_t data_size,
                                 rmt_encode_state_t *ret_state);
static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder);
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder);

static size_t IRAM_ATTR rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                           const void *primary_data, size_t data_size,
                                           rmt_encode_state_t *ret_state) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;
    
    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                              sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 0;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = 0;
    return ESP_OK;
}

static esp_err_t rmt_new_led_strip_encoder(rmt_encoder_handle_t *ret_encoder) {
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    if (!led_encoder) return ESP_ERR_NO_MEM;

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = (WS2812_T0H_NS + 50) / 100,
            .level1 = 0,
            .duration1 = (WS2812_T0L_NS + 50) / 100,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = (WS2812_T1H_NS + 50) / 100,
            .level1 = 0,
            .duration1 = (WS2812_T1L_NS + 50) / 100,
        },
        .flags.msb_first = 1
    };
    if ((ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder)) != ESP_OK) {
        goto err;
    }
    
    rmt_copy_encoder_config_t copy_encoder_config = {};
    if ((ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder)) != ESP_OK) {
        goto err;
    }
    
    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = (WS2812_RESET_US * 1000) / 100,
        .level1 = 0,
        .duration1 = 0,
    };
    
    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) rmt_del_encoder(led_encoder->bytes_encoder);
        if (led_encoder->copy_encoder) rmt_del_encoder(led_encoder->copy_encoder);
        free(led_encoder);
    }
    return ret;
}

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

    // optional: only wait if strict timing required
    // ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}
