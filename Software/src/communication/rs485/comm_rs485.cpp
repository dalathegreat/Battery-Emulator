#include "comm_rs485.h"
#include "../../include.h"

#include <list>

void init_rs485() {
#ifdef RS485_EN_PIN
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);
#endif  // RS485_EN_PIN
#ifdef RS485_SE_PIN
  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);
#endif  // RS485_SE_PIN
#ifdef PIN_5V_EN
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);
#endif  // PIN_5V_EN

  // Inverters and batteries are expected to initialize their serial port in their setup-function
  // for RS485 or Modbus comms.
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
