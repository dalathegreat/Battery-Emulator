#include "comm_rs485.h"
#include <Arduino.h>
#include "../../devboard/hal/hal.h"

#include <list>

bool init_rs485() {

  auto en_pin = esp32hal->RS485_EN_PIN();
  auto se_pin = esp32hal->RS485_SE_PIN();
  auto pin_5v_en = esp32hal->PIN_5V_EN();

  if (!esp32hal->alloc_pins_ignore_unused("RS485", en_pin, se_pin, pin_5v_en)) {
    DEBUG_PRINTF("Modbus failed to allocate pins\n");
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

  // Inverters and batteries are expected to initialize their serial port in their setup-function
  // for RS485 or Modbus comms.

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
