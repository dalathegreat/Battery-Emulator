#include "driver/i2c_master.h"

struct i2c_master_bus_t {};
struct i2c_master_dev_t {};

static i2c_master_bus_t fake_bus{};
static i2c_master_dev_t fake_dev{};

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t* bus_config, i2c_master_bus_handle_t* ret_bus_handle) {
  (void)bus_config;
  *ret_bus_handle = &fake_bus;
  return ESP_OK;
}

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t* dev_config,
                                    i2c_master_dev_handle_t* ret_handle) {
  (void)bus_handle;
  (void)dev_config;
  *ret_handle = &fake_dev;
  return ESP_OK;
}

esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev_handle, const uint8_t* write_buffer, size_t write_size,
                              int xfer_timeout_ms) {
  (void)dev_handle;
  (void)write_buffer;
  (void)write_size;
  (void)xfer_timeout_ms;
  return ESP_OK;
}
