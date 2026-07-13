#pragma once

#include <cstddef>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "soc/gpio_num.h"

typedef int esp_err_t;
#define ESP_OK 0

typedef struct i2c_master_bus_t* i2c_master_bus_handle_t;
typedef struct i2c_master_dev_t* i2c_master_dev_handle_t;

typedef enum {
  I2C_CLK_SRC_DEFAULT = 0,
} i2c_clock_source_t;

typedef enum {
  I2C_ADDR_BIT_LEN_7 = 0,
  I2C_ADDR_BIT_LEN_10,
} i2c_addr_bit_len_t;

typedef struct {
  int i2c_port;
  gpio_num_t sda_io_num;
  gpio_num_t scl_io_num;
  i2c_clock_source_t clk_source;
  uint8_t glitch_ignore_cnt;
  int intr_priority;
  size_t trans_queue_depth;
  struct {
    uint32_t enable_internal_pullup : 1;
  } flags;
} i2c_master_bus_config_t;

typedef struct {
  i2c_addr_bit_len_t dev_addr_length;
  uint16_t device_address;
  uint32_t scl_speed_hz;
  uint32_t scl_wait_us;
  struct {
  } flags;
} i2c_device_config_t;

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t* bus_config, i2c_master_bus_handle_t* ret_bus_handle);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t* dev_config,
                                    i2c_master_dev_handle_t* ret_handle);
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev_handle, const uint8_t* write_buffer, size_t write_size,
                              int xfer_timeout_ms);
