#include "Arduino.h"

#include <map>

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
// Records the last level written to each pin so unit tests can assert what the
// firmware actually drove (BMS power/ignition lines, contactor outputs, ...).
static std::map<uint8_t, uint8_t> g_emul_pin_levels;

void clear_pin_writes() {
  g_emul_pin_levels.clear();
}

// Returns the last level written to the pin, or -1 if it was never written.
int get_pin_level(uint8_t pin) {
  auto it = g_emul_pin_levels.find(pin);
  return (it == g_emul_pin_levels.end()) ? -1 : (int)it->second;
}

void digitalWrite(uint8_t pin, uint8_t val) {
  g_emul_pin_levels[pin] = val;
}

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
