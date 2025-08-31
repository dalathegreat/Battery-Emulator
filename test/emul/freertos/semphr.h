#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <memory>
#include <mutex>
#include <thread>
#include "queue.h"

using SemaphoreHandle_t = QueueHandle_t<void*>;

//using SemaphoreHandle_t = std::shared_ptr<class ISemaphore>;

class RecursiveSemaphore {
 public:
  RecursiveSemaphore() {
    q = std::make_shared<Queue<void*>>(1);
    void* token = nullptr;
    q->send(token, TickType_t(0), queueSEND_TO_BACK);  // Make it initially available
  }

  SemaphoreHandle_t getHandle() { return q; }

  BaseType_t take(TickType_t timeout) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    auto id = std::this_thread::get_id();

    if (owner == id) {
      ++count;
      return pdTRUE;
    }

    void* dummy = nullptr;
    if (q->receive(dummy, timeout)) {
      owner = id;
      count = 1;
      return pdTRUE;
    }

    return pdFALSE;
  }

  BaseType_t give() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (owner != std::this_thread::get_id() || count == 0)
      return pdFALSE;

    --count;
    if (count == 0) {
      owner = {};
      void* dummy = nullptr;
      return q->send(dummy, TickType_t(0), queueSEND_TO_BACK);
    }
    return pdTRUE;
  }

 private:
  SemaphoreHandle_t q;
  std::recursive_mutex mtx;
  std::thread::id owner;
  int count = 0;
};

SemaphoreHandle_t xSemaphoreCreateRecursiveMutex();
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xMutex, TickType_t xTicksToWait);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex);

SemaphoreHandle_t xSemaphoreCreateMutex();
void vSemaphoreDelete(SemaphoreHandle_t& xSemaphore);

#define xSemaphoreTake(xSemaphore, xBlockTime) xQueueSemaphoreTake((xSemaphore), (xBlockTime))
#define xSemaphoreGive(xSemaphore) xQueueGenericSend((xSemaphore), NULL, semGIVE_BLOCK_TIME, queueSEND_TO_BACK)

#define semGIVE_BLOCK_TIME ((TickType_t)0U)

#endif
