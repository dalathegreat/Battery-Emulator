#include "semphr.h"
#include "task.h"

#include <mutex>

#include <unordered_map>

std::mutex g_recursiveRegistryMutex;
std::unordered_map<SemaphoreHandle_t, std::shared_ptr<RecursiveSemaphore>> g_recursiveRegistry;

SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() {
  auto wrapper = std::make_shared<RecursiveSemaphore>();
  SemaphoreHandle_t handle = wrapper->getHandle();

  {
    std::lock_guard<std::mutex> lock(g_recursiveRegistryMutex);
    g_recursiveRegistry[handle] = wrapper;
  }

  return handle;
}

BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) {
  if (!xSemaphore)
    return pdFALSE;

  std::lock_guard<std::mutex> lock(g_recursiveRegistryMutex);
  auto it = g_recursiveRegistry.find(xSemaphore);
  if (it == g_recursiveRegistry.end())
    return pdFALSE;

  return it->second->take(xTicksToWait);
}

BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xSemaphore) {
  if (!xSemaphore)
    return pdFALSE;

  std::lock_guard<std::mutex> lock(g_recursiveRegistryMutex);
  auto it = g_recursiveRegistry.find(xSemaphore);
  if (it == g_recursiveRegistry.end())
    return pdFALSE;

  return it->second->give();
}

void vSemaphoreDelete(SemaphoreHandle_t& xSemaphore) {
  if (!xSemaphore)
    return;

  {
    std::lock_guard<std::mutex> lock(g_recursiveRegistryMutex);
    g_recursiveRegistry.erase(xSemaphore);
  }

  xSemaphore.reset();
}

SemaphoreHandle_t xSemaphoreCreateMutex() {
  auto q = std::make_shared<Queue<void*>>(1);
  void* token = nullptr;
  q->send(token, TickType_t(0), queueSEND_TO_BACK);  // Initially unlocked
  return q;
}
