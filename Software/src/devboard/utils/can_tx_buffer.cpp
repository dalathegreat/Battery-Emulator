#include "can_tx_buffer.h"

void can_tx_buffer_init(TRANSMIT_BUFFER_TYPE* buf) {
  buf->head = 0;
  buf->tail = 0;
  buf->count = 0;
}

// Function to add a pointer to the buffer
bool can_tx_add_to_buffer(TRANSMIT_BUFFER_TYPE* buf, const CAN_frame_t* ptr) {
  // Check if the buffer is full
  if (buf->count == CAN_TX_BUFFER_SIZE) {
    return false;  // Buffer is full
  }

  // Add the pointer to the buffer
  buf->buffer[buf->head] = ptr;
  buf->head = (buf->head + 1) % CAN_TX_BUFFER_SIZE;
  buf->count++;

  return true;
}

// Function to retrieve a pointer from the buffer
const CAN_frame_t* can_tx_fetch_from_buffer(TRANSMIT_BUFFER_TYPE* buf) {
  // Check if the buffer is empty
  if (buf->count == 0) {
    return nullptr;  // Buffer is empty
  }

  // Retrieve the pointer from the buffer
  const CAN_frame_t* ptr = buf->buffer[buf->tail];
  buf->tail = (buf->tail + 1) % CAN_TX_BUFFER_SIZE;
  buf->count--;

  return ptr;
}

TRANSMIT_BUFFER_TYPE can_tx_buf_global;
