#include "ringbuf.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

struct ItemHeader {
  size_t size;
};

struct RingBuffer {
  explicit RingBuffer(size_t size, RingbufferType_t type)
      : buffer(size), type(type), head(0), tail(0), full(false), acquired(0), waiting_items(0) {}

  std::vector<uint8_t> buffer;
  RingbufferType_t type;

  size_t head;
  size_t tail;
  bool full;
  size_t acquired;
  size_t waiting_items;

  std::mutex mtx;
  std::condition_variable data_available;
  std::condition_variable space_available;

  size_t capacity() const { return buffer.size(); }

  size_t free_space() const {
    if (full)
      return 0;
    if (head >= tail)
      return capacity() - (head - tail);
    return tail - head;
  }

  size_t used_space() const { return capacity() - free_space(); }

  bool has_enough_space(size_t item_size) const { return free_space() >= item_size + sizeof(ItemHeader); }

  void advance(size_t& index, size_t amount) { index = (index + amount) % capacity(); }
};

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType) {
  return new RingBuffer(xBufferSize, xBufferType);
}

BaseType_t xRingbufferSend(RingbufHandle_t xRingbuffer, const void* pvItem, size_t xItemSize, TickType_t xTicksToWait) {
  auto& rb = *(RingBuffer*)xRingbuffer;
  std::unique_lock<std::mutex> lock(rb.mtx);

  size_t total_size = xItemSize + sizeof(ItemHeader);
  auto timeout = std::chrono::milliseconds(xTicksToWait);

  if (!rb.space_available.wait_for(lock, timeout, [&] { return rb.has_enough_space(xItemSize); })) {
    return 0;  // timeout
  }

  // Write header
  ItemHeader header{xItemSize};
  for (size_t i = 0; i < sizeof(ItemHeader); ++i)
    rb.buffer[rb.head + i % rb.capacity()] = reinterpret_cast<uint8_t*>(&header)[i];
  rb.advance(rb.head, sizeof(ItemHeader));

  // Write payload
  for (size_t i = 0; i < xItemSize; ++i)
    rb.buffer[rb.head + i % rb.capacity()] = reinterpret_cast<const uint8_t*>(pvItem)[i];
  rb.advance(rb.head, xItemSize);

  if (rb.head == rb.tail)
    rb.full = true;
  rb.waiting_items++;
  rb.data_available.notify_one();
  return 1;
}

void* xRingbufferReceiveUpTo(RingbufHandle_t xRingbuffer, size_t* pxItemSize, TickType_t xTicksToWait,
                             size_t xMaxSize) {
  auto& rb = *(RingBuffer*)xRingbuffer;
  std::unique_lock<std::mutex> lock(rb.mtx);

  auto timeout = std::chrono::milliseconds(xTicksToWait);

  if (!rb.data_available.wait_for(lock, timeout, [&] { return rb.head != rb.tail || rb.full; })) {
    return nullptr;  // timeout
  }

  // Peek header
  ItemHeader header;
  for (size_t i = 0; i < sizeof(ItemHeader); ++i)
    reinterpret_cast<uint8_t*>(&header)[i] = rb.buffer[(rb.tail + i) % rb.capacity()];

  if (header.size > xMaxSize)
    return nullptr;

  rb.advance(rb.tail, sizeof(ItemHeader));
  void* item_ptr = std::malloc(header.size);
  for (size_t i = 0; i < header.size; ++i)
    reinterpret_cast<uint8_t*>(item_ptr)[i] = rb.buffer[(rb.tail + i) % rb.capacity()];
  rb.advance(rb.tail, header.size);

  rb.full = false;
  rb.acquired += header.size;
  rb.waiting_items--;
  rb.space_available.notify_one();
  *pxItemSize = header.size;
  return item_ptr;
}

void vRingbufferReturnItem(RingbufHandle_t xRingbuffer, void* pvItem) {
  auto rb = (RingBuffer*)xRingbuffer;
  std::free(pvItem);
  std::unique_lock<std::mutex> lock(rb->mtx);
  rb->acquired = 0;
  rb->space_available.notify_one();
}

void vRingbufferGetInfo(RingbufHandle_t xRingbuffer, UBaseType_t* uxFree, UBaseType_t* uxRead, UBaseType_t* uxWrite,
                        UBaseType_t* uxAcquire, UBaseType_t* uxItemsWaiting) {
  auto rb = (RingBuffer*)xRingbuffer;
  std::unique_lock<std::mutex> lock(rb->mtx);
  if (uxFree)
    *uxFree = rb->free_space();
  if (uxRead)
    *uxRead = rb->tail;
  if (uxWrite)
    *uxWrite = rb->head;
  if (uxAcquire)
    *uxAcquire = rb->acquired;
  if (uxItemsWaiting)
    *uxItemsWaiting = rb->waiting_items;
}

void vRingbufferDelete(RingbufHandle_t xRingbuffer) {
  delete xRingbuffer;
}

size_t xRingbufferGetMaxItemSize(RingbufHandle_t xRingbuffer) {
  auto rb = (RingBuffer*)xRingbuffer;
  return rb->capacity() - sizeof(ItemHeader);
}

size_t xRingbufferGetCurFreeSize(RingbufHandle_t xRingbuffer) {
  auto rb = (RingBuffer*)xRingbuffer;
  std::unique_lock<std::mutex> lock(rb->mtx);
  return rb->free_space();
}
