#ifndef _COMM_RS485_H_
#define _COMM_RS485_H_

/**
 * @brief Initialization of RS485
 *
 * @param[in] void
 *
 * @return void
 */
void init_rs485();

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
