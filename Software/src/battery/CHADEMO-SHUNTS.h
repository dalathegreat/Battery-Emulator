#ifndef CHADEMO_SHUNTS_H
#define CHADEMO_SHUNTS_H

#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

uint16_t get_measured_voltage();
uint16_t get_measured_current();
inline void ISA_handler(CAN_frame* frame);
inline void ISA_handle521(CAN_frame* frame);
inline void ISA_handle522(CAN_frame* frame);
inline void ISA_handle523(CAN_frame* frame);
inline void ISA_handle524(CAN_frame* frame);
inline void ISA_handle525(CAN_frame* frame);
inline void ISA_handle526(CAN_frame* frame);
inline void ISA_handle527(CAN_frame* frame);
inline void ISA_handle528(CAN_frame* frame);

void transmit_can(CAN_frame* tx_frame, int interface);

#endif
