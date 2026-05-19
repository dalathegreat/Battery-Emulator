#include "comm_rs485.h"
#include <Arduino.h>
#include "../../devboard/hal/hal.h"
#ifndef UNIT_TEST
#include "driver/uart.h"
#endif

#include <list>

// All current Battery-Emulator RS485 users use Serial2/UART2. For boards with
// a real DE/~RE pin (for example Waveshare ESP32-S3-RS485-CAN with SP3485EN),
// we let the ESP-IDF UART driver control RTS in UART_MODE_RS485_HALF_DUPLEX.
// This keeps the existing Serial2.write() path and the Arduino/ESP32 UART
// locking behavior intact instead of wrapping writes in a separate function.
//
// Host unit tests do not have ESP-IDF UART headers or hardware, so the actual
// UART RS485 mode setup is compiled only for firmware builds.
#ifndef UNIT_TEST
static constexpr uart_port_t RS485_UART_NUM = UART_NUM_2;
#endif

static gpio_num_t rs485_de_pin = GPIO_NUM_NC;
static bool rs485_de_active_high = true;

bool init_rs485() {

  auto en_pin = esp32hal->RS485_EN_PIN();
  auto se_pin = esp32hal->RS485_SE_PIN();
  auto pin_5v_en = esp32hal->PIN_5V_EN();
  rs485_de_pin = esp32hal->RS485_DE_PIN();
  rs485_de_active_high = esp32hal->RS485_DE_ACTIVE_HIGH();

  if (!esp32hal->alloc_pins_ignore_unused("RS485", en_pin, se_pin, pin_5v_en)) {
    DEBUG_PRINTF("RS485 failed to allocate static enable pins\n");
    // Leave RS485 DE disabled rather than bypassing HAL pin ownership later in rs485_begin().
    rs485_de_pin = GPIO_NUM_NC;
    return true;  // Safe to call even when RS485 is not used.
  }

  if (rs485_de_pin != GPIO_NUM_NC && !esp32hal->alloc_pins("RS485 DE", rs485_de_pin)) {
    DEBUG_PRINTF("RS485 failed to allocate DE pin\n");
    // Do not let uart_set_pin() claim a pin that the HAL allocator rejected.
    rs485_de_pin = GPIO_NUM_NC;
    return true;  // Safe to call even when RS485 is not used.
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
    // Idle receive state until the UART driver takes over this pin as RTS/DE
    // in rs485_begin().  UART_MODE_RS485_HALF_DUPLEX asserts RTS high while the
    // UART FIFO is transmitting and deasserts it after the last bit was sent.
    pinMode(rs485_de_pin, OUTPUT);
    digitalWrite(rs485_de_pin, rs485_de_active_high ? LOW : HIGH);
  }

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

  if (rs485_de_pin != GPIO_NUM_NC) {
    // The current Battery-Emulator RS485 stack is wired to Serial2/UART2.
    // Keep this explicit because UART_MODE_RS485_HALF_DUPLEX configures a UART port,
    // not the HardwareSerial object itself.
    if (&serial != &Serial2) {
      DEBUG_PRINTF("RS485 half-duplex DE is only supported on Serial2/UART2\n");
      return false;
    }

    if (!rs485_de_active_high) {
      DEBUG_PRINTF("RS485 active-low DE is not supported by UART RS485 mode\n");
      return false;
    }

#ifndef UNIT_TEST
    // Configure UART2 RTS as RS485 driver-enable.  This is intentionally done
    // after Serial2.begin(), because Serial2.begin() configures the UART pins.
    const esp_err_t pin_result = uart_set_pin(RS485_UART_NUM, tx_pin, rx_pin, rs485_de_pin, UART_PIN_NO_CHANGE);
    const esp_err_t mode_result = uart_set_mode(RS485_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);

    if (pin_result != ESP_OK || mode_result != ESP_OK) {
      DEBUG_PRINTF("RS485 UART half-duplex setup failed, pin_result=%d, mode_result=%d\n", static_cast<int>(pin_result),
                   static_cast<int>(mode_result));
      return false;
    }
#else
    // Host unit tests only verify that the common RS485 initialization compiles
    // and keeps the existing Serial2.begin()/Serial2.write() path. The ESP-IDF
    // UART driver owns the RTS/DE timing in firmware builds.
    DEBUG_PRINTF("RS485 UART half-duplex setup skipped in UNIT_TEST\n");
#endif
  }

  return true;
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
