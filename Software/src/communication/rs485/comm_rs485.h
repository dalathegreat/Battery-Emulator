#ifndef _COMM_RS485_H_
#define _COMM_RS485_H_

#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdint.h>

/**
 * @brief Initialization of RS485 control pins.
 *
 * @param[in] void
 *
 * @return Safe to call even if rs485 is not used
 */
bool init_rs485();

/**
 * @brief Initialize a HardwareSerial port with the board-specific RS485 RX/TX pins.
 *
 * If the selected HAL exposes RS485_DE_PIN(), UART2 is configured in ESP-IDF
 * UART_MODE_RS485_HALF_DUPLEX so the UART driver controls RTS/DE around the
 * existing Serial2.write() calls.
 */
bool rs485_begin(const char* owner, HardwareSerial& serial, uint32_t baud, uint32_t config = SERIAL_8N1);

// Defines an interface for any object that needs to receive a signal to handle RS485 comm.
// Can be extended later for more complex operation.
class Rs485Receiver {
 public:
  virtual void receive() = 0;
};

// Forwards the call to all registered RS485 receivers
void receive_rs485();

// Registers the given object as a receiver.
void register_receiver(Rs485Receiver* receiver);

#endif
