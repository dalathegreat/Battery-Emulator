#ifndef __CAN_TX_BUFFER__
#define __CAN_TX_BUFFER__

#include "../../include.h"

#define CAN_TX_BUFFER_SIZE \
  (BATTERY_CAN_BUFFER_SIZE + INVERTER_CAN_BUFFER_SIZE + 2)  // Define the size of the buffer, plus margin

typedef struct {
  const CAN_frame_t* buffer[CAN_TX_BUFFER_SIZE];  // Array to hold pointers to objects
  int head;                                       // Index for the next element to be added
  int tail;                                       // Index for the next element to be removed
  int count;                                      // Number of elements in the buffer
} TRANSMIT_BUFFER_TYPE;

void can_tx_buffer_init(TRANSMIT_BUFFER_TYPE* buf);
bool can_tx_add_to_buffer(TRANSMIT_BUFFER_TYPE* buf, const CAN_frame_t* ptr);
const CAN_frame_t* can_tx_fetch_from_buffer(TRANSMIT_BUFFER_TYPE* buf);

extern TRANSMIT_BUFFER_TYPE can_tx_buf_global;

#endif
