#include <Arduino.h>

#ifdef CONFIG_IDF_TARGET_ESP32S3
// ESP32-S3 uses ESP-IDF 4.4.x with older RMT API
#include "driver/rmt.h"
#else
// ESP32 classic uses ESP-IDF v5.x RMT driver API
#include "driver/rmt_tx.h"
#endif
// WS2812 timing parameters (in nanoseconds)
#define WS2812_T0H_NS (400)
#define WS2812_T0L_NS (850)
#define WS2812_T1H_NS (800)
#define WS2812_T1L_NS (450)
#define WS2812_RESET_US 280

#ifdef CONFIG_IDF_TARGET_ESP32S3
// ========== ESP-IDF 4.4.x Implementation (ESP32-S3) ==========
// RMT tick calculations (80MHz / 8 = 10MHz = 100ns per tick)
#define RMT_CLK_DIV 8
#define WS2812_T0H_TICKS ((WS2812_T0H_NS + 50) / 100)
#define WS2812_T0L_TICKS ((WS2812_T0L_NS + 50) / 100)
#define WS2812_T1H_TICKS ((WS2812_T1H_NS + 50) / 100)
#define WS2812_T1L_TICKS ((WS2812_T1L_NS + 50) / 100)

static uint8_t rmt_channel = 0;
static bool rmt_initialized = false;

// Convert a single byte to RMT items (8 bits)
static void IRAM_ATTR ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
                                          size_t wanted_num, size_t *translated_size, size_t *item_num) {
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }

    const uint8_t *psrc = (const uint8_t *)src;
    rmt_item32_t bit0 = {{{ WS2812_T0H_TICKS, 1, WS2812_T0L_TICKS, 0 }}};
    rmt_item32_t bit1 = {{{ WS2812_T1H_TICKS, 1, WS2812_T1L_TICKS, 0 }}};

    size_t size = 0;
    size_t num = 0;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                dest[num++] = bit1;
            } else {
                dest[num++] = bit0;
            }
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

void espShow(uint8_t pin, uint8_t* pixels, uint32_t numBytes) {
    if (!rmt_initialized) {
        rmt_config_t config = {
            .rmt_mode = RMT_MODE_TX,
            .channel = (rmt_channel_t)rmt_channel,
            .gpio_num = (gpio_num_t)pin,
            .clk_div = RMT_CLK_DIV,
            .mem_block_num = 1,
            .tx_config = {
                .carrier_freq_hz = 38000,
                .carrier_level = RMT_CARRIER_LEVEL_HIGH,
                .idle_level = RMT_IDLE_LEVEL_LOW,
                .carrier_duty_percent = 33,
                .carrier_en = false,
                .loop_en = false,
                .idle_output_en = true,
            }
        };

        rmt_config(&config);
        rmt_driver_install(config.channel, 0, 0);
        rmt_translator_init(config.channel, ws2812_rmt_adapter);
        rmt_initialized = true;
    }

    rmt_write_sample((rmt_channel_t)rmt_channel, pixels, numBytes, true);
    rmt_wait_tx_done((rmt_channel_t)rmt_channel, portMAX_DELAY);
}

#else
// ========== ESP-IDF 5.x Implementation (ESP32 Classic) ==========
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

#endif  // CONFIG_IDF_TARGET_ESP32S3
