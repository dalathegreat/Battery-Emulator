#ifndef _COMM_RS485_H_
#define _COMM_RS485_H_

#include <Arduino.h>
#include <HardwareSerial.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialization of RS485
 *
 * @param[in] void
 *
 * @return Safe to call even if rs485 is not used
 */
bool init_rs485();

/**
 * @brief Initialize a HardwareSerial port with the board-specific RS485 RX/TX pins.
 *
 * This also leaves the optional half-duplex DE pin in receive mode.
 */
bool rs485_begin(const char* owner, HardwareSerial& serial, uint32_t baud, uint32_t config = SERIAL_8N1);

/**
 * @brief Set the RS485 driver direction. The eModbus library uses this as RTS callback.
 *
 * @param transmit true = transmit mode, false = receive mode
 */
void rs485_set_direction(bool transmit);

/**
 * @brief Write a complete frame to Serial2, toggling half-duplex DE when available.
 */
size_t rs485_write(const uint8_t* data, size_t len);
size_t rs485_write(const char* data, size_t len);


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
