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
// Records every pin write so tests can assert what the firmware actually drove
// (contactor toggling, precharge PWM enable, ...).
std::vector<PinWrite> g_emul_pin_writes;

void clear_pin_writes() {
  g_emul_pin_writes.clear();
}

const std::vector<PinWrite>& get_pin_writes() {
  return g_emul_pin_writes;
}

void digitalWrite(uint8_t pin, uint8_t val) {
  g_emul_pin_writes.push_back({pin, val});
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
// Records the precharge PWM frequency the firmware asks for, so the regulation
// loop can be asserted on the value it actually drove rather than on internal
// state.
std::vector<ToneWrite> g_emul_tone_writes;

void clear_tone_writes() {
  g_emul_tone_writes.clear();
}

const std::vector<ToneWrite>& get_tone_writes() {
  return g_emul_tone_writes;
}

bool ledcWriteTone(uint8_t pin, uint32_t freq) {
  g_emul_tone_writes.push_back({pin, freq});
  return true;
}

ESPClass ESP;
