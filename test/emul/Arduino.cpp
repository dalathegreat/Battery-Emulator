#include "Arduino.h"

#include "../../Software/src/communication/can/comm_can.h"

// Provide the definition that was previously in USER_SETTINGS.cpp
volatile CAN_Configuration can_config = {.battery = CAN_Interface::CAN_NATIVE,
                                         .inverter = CAN_Interface::CAN_NATIVE,
                                         .battery_double = CAN_Interface::CAN_NATIVE,
                                         .charger = CAN_Interface::CAN_NATIVE,
                                         .shunt = CAN_Interface::CAN_NATIVE};

void delay(unsigned long ms) {}
void delayMicroseconds(unsigned long us) {}
int digitalRead(uint8_t pin) {
  return 0;
}
void digitalWrite(uint8_t pin, uint8_t val) {}

unsigned long micros() {
  return 0;
}
void pinMode(uint8_t pin, uint8_t mode) {}

int max(int a, int b) {
  return (a > b) ? a : b;
}

bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel) {
  return true;
}
bool ledcWrite(uint8_t pin, uint32_t duty) {
  return true;
}

ESPClass ESP;
