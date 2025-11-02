#ifndef _OBD_H_
#define _OBD_H_

#include "comm_can.h"

void handle_obd_frame(CAN_frame& rx_frame);

void transmit_obd_can_frame(unsigned int address, int interface, bool canFD);

#endif  // _OBD_H_
