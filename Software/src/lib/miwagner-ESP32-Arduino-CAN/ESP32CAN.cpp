#include "ESP32CAN.h"
#include <Arduino.h>
#include "../../devboard/utils/events.h"

int ESP32CAN::CANInit() {
  return CAN_init();
}
bool ESP32CAN::CANWriteFrame(const CAN_frame_t* p_frame) {
  static unsigned long start_time;
  bool result = false;
  if (tx_ok) {
    result = CAN_write_frame(p_frame);
    tx_ok = result;
    if (!tx_ok) {
      start_time = millis();
    }
  } else {
    if ((millis() - start_time) >= 20) {
      tx_ok = true;
    }
  }
  return result;
}

int ESP32CAN::CANStop() {
  return CAN_stop();
}
int ESP32CAN::CANConfigFilter(const CAN_filter_t* p_filter) {
  return CAN_config_filter(p_filter);
}

ESP32CAN ESP32Can;
