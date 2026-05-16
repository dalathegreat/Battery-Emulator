#include "comm_rs485.h"
#include <Arduino.h>
#include "../../devboard/hal/hal.h"

#include <list>

static gpio_num_t rs485_de_pin = GPIO_NUM_NC;
static bool rs485_de_active_high = true;
static bool rs485_control_pins_initialized = false;

static uint8_t rs485_tx_level() {
  return rs485_de_active_high ? HIGH : LOW;
}

static uint8_t rs485_rx_level() {
  return rs485_de_active_high ? LOW : HIGH;
}

bool init_rs485() {

  auto en_pin = esp32hal->RS485_EN_PIN();
  auto se_pin = esp32hal->RS485_SE_PIN();
  auto pin_5v_en = esp32hal->PIN_5V_EN();
  rs485_de_pin = esp32hal->RS485_DE_PIN();
  rs485_de_active_high = esp32hal->RS485_DE_ACTIVE_HIGH();

  if (!esp32hal->alloc_pins_ignore_unused("RS485", en_pin, se_pin, pin_5v_en)) {
    DEBUG_PRINTF("RS485 failed to allocate static enable pins\n");
    return true;  //Early return, we do not set the pins
  }

  if (rs485_de_pin != GPIO_NUM_NC && !esp32hal->alloc_pins("RS485 DE", rs485_de_pin)) {
    DEBUG_PRINTF("RS485 failed to allocate DE pin\n");
    return true;  //Early return, we do not set the pins
  }

  if (en_pin != GPIO_NUM_NC) {
    pinMode(en_pin, OUTPUT);
    digitalWrite(en_pin, HIGH);
  }

  if (se_pin != GPIO_NUM_NC) {
    pinMode(se_pin, OUTPUT);
    digitalWrite(se_pin, HIGH);
  }

  if (pin_5v_en != GPIO_NUM_NC) {
    pinMode(pin_5v_en, OUTPUT);
    digitalWrite(pin_5v_en, HIGH);
  }

  if (rs485_de_pin != GPIO_NUM_NC) {
    pinMode(rs485_de_pin, OUTPUT);
    digitalWrite(rs485_de_pin, rs485_rx_level());
  }

  rs485_control_pins_initialized = true;

  // Inverters and batteries initialize the serial port in their setup-function.
  return true;
}

bool rs485_begin(const char* owner, HardwareSerial& serial, uint32_t baud, uint32_t config) {
  auto rx_pin = esp32hal->RS485_RX_PIN();
  auto tx_pin = esp32hal->RS485_TX_PIN();

  if (!esp32hal->alloc_pins(owner, rx_pin, tx_pin)) {
    return false;
  }

  serial.begin(baud, config, rx_pin, tx_pin);
  rs485_set_direction(false);
  return true;
}

void rs485_set_direction(bool transmit) {
  if (!rs485_control_pins_initialized || rs485_de_pin == GPIO_NUM_NC) {
    return;
  }

  digitalWrite(rs485_de_pin, transmit ? rs485_tx_level() : rs485_rx_level());
}

size_t rs485_write(const uint8_t* data, size_t len) {
  if (rs485_de_pin == GPIO_NUM_NC) {
    return Serial2.write(data, len);
  }

  rs485_set_direction(true);
  delayMicroseconds(10);
  size_t written = Serial2.write(data, len);
  Serial2.flush();
  delayMicroseconds(10);
  rs485_set_direction(false);
  return written;
}

size_t rs485_write(const char* data, size_t len) {
  return rs485_write(reinterpret_cast<const uint8_t*>(data), len);
}

static std::list<Rs485Receiver*> receivers;

void receive_rs485() {
  for (auto& receiver : receivers) {
    receiver->receive();
  }
}

void register_receiver(Rs485Receiver* receiver) {
  receivers.push_back(receiver);
}
