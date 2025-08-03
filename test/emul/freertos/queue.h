#ifndef EMUL_FREERTOS_QUEUE_H
#define EMUL_FREERTOS_QUEUE_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include "task.h"

#define queueSEND_TO_BACK ((BaseType_t)0)
#define queueSEND_TO_FRONT ((BaseType_t)1)
#define queueOVERWRITE ((BaseType_t)2)

template <typename T>
class Queue {
 public:
  Queue(size_t maxLen) : maxLength(maxLen) {}

  BaseType_t send(const T& item, TickType_t timeout, BaseType_t xCopyPosition) {
    std::unique_lock<std::mutex> lock(mtx);

    bool success =
        cv_space.wait_for(lock, std::chrono::milliseconds(timeout), [this]() { return queue.size() < maxLength; });
    if (!success)
      return pdFALSE;

    if (xCopyPosition == queueSEND_TO_BACK) {
      queue.push_back(item);
    } else {
      queue.push_front(item);
    }

    cv_data.notify_one();
    return pdTRUE;
  }

  BaseType_t receive(T& out, TickType_t timeout) {
    std::unique_lock<std::mutex> lock(mtx);
    if (!cv_data.wait_for(lock, std::chrono::milliseconds(timeout), [this]() { return !queue.empty(); }))
      return pdFALSE;

    out = queue.front();
    queue.pop_front();
    cv_space.notify_one();
    return pdTRUE;
  }

 private:
  std::deque<T> queue;
  std::mutex mtx;
  std::condition_variable cv_data, cv_space;
  size_t maxLength;
};

template <typename T>
using QueueHandle_t = std::shared_ptr<class Queue<T>>;

//using QueueHandle_t = std::shared_ptr<class ISemaphore>;
//struct QueueDefinition; /* Using old naming convention so as not to break kernel aware debuggers. */
//typedef struct QueueDefinition   * QueueHandle_t;

/*BaseType_t xQueueSemaphoreTake( QueueHandle_t xQueue,
                                TickType_t xTicksToWait );

BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition );*/

template <typename T>
BaseType_t xQueueGenericSend(QueueHandle_t<T> xQueue, const void* const pvItemToQueue, TickType_t xTicksToWait,
                             BaseType_t xCopyPosition) {
  if (!xQueue)
    return pdFALSE;

  if constexpr (std::is_same<T, void*>::value) {
    void* token = nullptr;
    return xQueue->send(token, xTicksToWait, xCopyPosition);
  }

  if (!pvItemToQueue)
    return pdFALSE;

  const T* item = static_cast<const T*>(pvItemToQueue);
  return xQueue->send(*item, xTicksToWait, xCopyPosition);
}

template <typename T>
BaseType_t xQueueReceive(QueueHandle_t<T> xQueue, void* const pvBuffer, TickType_t xTicksToWait) {
  if (!xQueue || !pvBuffer)
    return pdFALSE;
  T* out = static_cast<T*>(pvBuffer);
  return xQueue->receive(*out, xTicksToWait);
}

inline BaseType_t xQueueSemaphoreTake(QueueHandle_t<void*> xSemaphore, TickType_t xTicksToWait) {
  if (!xSemaphore)
    return pdFALSE;
  void* dummy = nullptr;
  return xSemaphore->receive(dummy, xTicksToWait);
}

#endif
