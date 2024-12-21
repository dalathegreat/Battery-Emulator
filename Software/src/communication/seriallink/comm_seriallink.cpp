#include "comm_seriallink.h"
#include "../../include.h"

// Parameters

#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
#define SERIAL_LINK_BAUDRATE 112500
#endif

// Initialization functions

void init_serialDataLink() {
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
  Serial2.begin(SERIAL_LINK_BAUDRATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
#endif  // SERIAL_LINK_RECEIVER || SERIAL_LINK_TRANSMITTER
}

// Main functions

#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
void run_serialDataLink() {
  static unsigned long updateTime = 0;
  unsigned long currentMillis = millis();

  if ((currentMillis - updateTime) > 1) {  //Every 2ms
    updateTime = currentMillis;
#ifdef SERIAL_LINK_RECEIVER
    manageSerialLinkReceiver();
#endif
#ifdef SERIAL_LINK_TRANSMITTER
    manageSerialLinkTransmitter();
#endif
  }
}
#endif  // SERIAL_LINK_RECEIVER || SERIAL_LINK_TRANSMITTER
