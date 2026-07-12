#include "precharge_control_i2c_g05.h"
#include <Arduino.h>
#include <Wire.h>
#include "../../devboard/hal/hal.h"

static TwoWire G05Wire = TwoWire(1);
static bool g05_initialized = false;
static uint8_t TPS55288_ADDR = 0x74;

#define TPS_REG_REF_LSB 0x00
#define TPS_REG_REF_MSB 0x01
#define TPS_REG_MODE 0x06

static bool tps_write(uint8_t reg, uint8_t value) {
  G05Wire.beginTransmission(TPS55288_ADDR);
  G05Wire.write(reg);
  G05Wire.write(value);
  return G05Wire.endTransmission() == 0;
}

static uint8_t tps_read(uint8_t reg) {
  G05Wire.beginTransmission(TPS55288_ADDR);
  G05Wire.write(reg);
  G05Wire.endTransmission(false);
  G05Wire.requestFrom(TPS55288_ADDR, (uint8_t)1);
  return G05Wire.available() ? G05Wire.read() : 0;
}

bool g05_is_initialized() {
  return g05_initialized;
}

void g05_enable_output() {
  uint8_t mode = tps_read(TPS_REG_MODE);
  mode |= 0x80;
  tps_write(TPS_REG_MODE, mode);
}

void g05_disable_output() {
  if (!g05_initialized) {
    return;
  }

  uint8_t mode = tps_read(TPS_REG_MODE);
  mode &= 0x7F;
  tps_write(TPS_REG_MODE, mode);
}

void g05_set_voltage(float voltage) {
  if (voltage < 0.8f)
    voltage = 0.8f;
  if (voltage > 20.0f)
    voltage = 20.0f;

  uint16_t code = roundf((voltage - 0.8f) / 0.020f);
  if (code > 960)
    code = 960;

  tps_write(TPS_REG_REF_LSB, code & 0xFF);
  tps_write(TPS_REG_REF_MSB, (code >> 8) & 0x03);
}

bool g05_i2c_init_if_needed() {
  if (g05_initialized)
    return true;

  auto sda = esp32hal->I2C_G05_SDA_PIN();
  auto scl = esp32hal->I2C_G05_SCL_PIN();

  if (sda == GPIO_NUM_NC || scl == GPIO_NUM_NC) {
    logging.printf("Precharge G05: I2C pins not configured for this hardware\n");
    return false;
  }

  G05Wire.begin((int)sda, (int)scl, 100000);

  const uint8_t addrs[] = {0x74, 0x75};
  for (uint8_t a : addrs) {
    G05Wire.beginTransmission(a);
    if (G05Wire.endTransmission() == 0) {
      TPS55288_ADDR = a;
      g05_initialized = true;
      logging.printf("Precharge G05: TPS55288 found at 0x%02X\n", a);
      return true;
    }
  }

  logging.printf("Precharge G05: TPS55288 not found\n");
  return false;
}
