#ifndef CHADEMO_SHUNTS_H
#define CHADEMO_SHUNTS_H

#include <stdint.h>
#include "../devboard/utils/types.h"

uint16_t get_measured_voltage();
uint16_t get_measured_current();
void ISA_handleFrame(CAN_frame* frame);
inline void ISA_handle521(CAN_frame* frame);
inline void ISA_handle522(CAN_frame* frame);
inline void ISA_handle523(CAN_frame* frame);
inline void ISA_handle524(CAN_frame* frame);
inline void ISA_handle525(CAN_frame* frame);
inline void ISA_handle526(CAN_frame* frame);
inline void ISA_handle527(CAN_frame* frame);
inline void ISA_handle528(CAN_frame* frame);
void ISA_initialize();
void ISA_STOP();
void ISA_sendSTORE();
void ISA_START();
void ISA_RESTART();
void ISA_deFAULT();
void ISA_initCurrent();
void ISA_getCONFIG(uint8_t i);
void ISA_getCAN_ID(uint8_t i);
void ISA_getINFO(uint8_t i);

void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
