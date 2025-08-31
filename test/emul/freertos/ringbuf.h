#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef void* RingbufHandle_t;

typedef enum {
  /**
     * No-split buffers will only store an item in contiguous memory and will
     * never split an item. Each item requires an 8 byte overhead for a header
     * and will always internally occupy a 32-bit aligned size of space.
     */
  RINGBUF_TYPE_NOSPLIT = 0,
  /**
     * Allow-split buffers will split an item into two parts if necessary in
     * order to store it. Each item requires an 8 byte overhead for a header,
     * splitting incurs an extra header. Each item will always internally occupy
     * a 32-bit aligned size of space.
     */
  RINGBUF_TYPE_ALLOWSPLIT,
  /**
     * Byte buffers store data as a sequence of bytes and do not maintain separate
     * items, therefore byte buffers have no overhead. All data is stored as a
     * sequence of byte and any number of bytes can be sent or retrieved each
     * time.
     */
  RINGBUF_TYPE_BYTEBUF,
  RINGBUF_TYPE_MAX,
} RingbufferType_t;

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType);

BaseType_t xRingbufferSend(RingbufHandle_t xRingbuffer, const void* pvItem, size_t xItemSize, TickType_t xTicksToWait);

void vRingbufferGetInfo(RingbufHandle_t xRingbuffer, UBaseType_t* uxFree, UBaseType_t* uxRead, UBaseType_t* uxWrite,
                        UBaseType_t* uxAcquire, UBaseType_t* uxItemsWaiting);

void vRingbufferDelete(RingbufHandle_t xRingbuffer);

size_t xRingbufferGetMaxItemSize(RingbufHandle_t xRingbuffer);

size_t xRingbufferGetCurFreeSize(RingbufHandle_t xRingbuffer);

void* xRingbufferReceiveUpTo(RingbufHandle_t xRingbuffer, size_t* pxItemSize, TickType_t xTicksToWait, size_t xMaxSize);

void vRingbufferReturnItem(RingbufHandle_t xRingbuffer, void* pvItem);
