#include "Arduino.h"

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

// Mock implementation for OBD
#include "../../Software/src/communication/can/obd.h"
void handle_obd_frame(CAN_frame& frame) {
  (void)frame;
}
void transmit_obd_can_frame(unsigned int address, int interface, bool canFD) {
  (void)interface;
}

void start_bms_reset() {}

#include "../../Software/src/communication/rs485/comm_rs485.h"

// Mock implementation
void register_receiver(Rs485Receiver* receiver) {
  (void)receiver;  // Silence unused parameter warning
}

ESPClass ESP;
