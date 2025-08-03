#include "task.h"
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

struct tskTaskControlBlock {
  std::thread thread;
  tskTaskControlBlock(std::thread&& t) : thread(std::move(t)) {}
};

std::list<std::unique_ptr<tskTaskControlBlock>> taskRegistry;
std::mutex taskRegistryMutex;

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t ulStackDepth,
                                   void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                   const BaseType_t xCoreID) {

  std::thread t([=]() {
    try {
      pxTaskCode(pvParameters);
    } catch (...) {
      std::clog << "Unhandled exception in thread " << pcName << std::endl;
    }
  });

  auto tcb = std::make_unique<tskTaskControlBlock>(std::move(t));

  if (pxCreatedTask) {
    *pxCreatedTask = tcb.get();  // Return raw pointer as handle
  }

  std::lock_guard<std::mutex> lock(taskRegistryMutex);
  taskRegistry.push_back(std::move(tcb));  // Thread stays alive

  return pdPASS;
}

void vTaskDelete(TaskHandle_t task) {
  if (!task)
    return;

  std::lock_guard<std::mutex> lock(taskRegistryMutex);

  for (auto it = taskRegistry.begin(); it != taskRegistry.end(); ++it) {
    if (it->get() == task) {
      if (task->thread.joinable()) {
        task->thread.join();  // Ensure thread finishes cleanly
      }
      taskRegistry.erase(it);  // Destroys the tcb and thread
      break;
    }
  }
}

constexpr std::chrono::milliseconds TICK_DURATION(1);

TickType_t getCurrentTickCount() {
  static auto startTime = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
  return static_cast<TickType_t>(elapsed.count());
}

BaseType_t xTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement) {
  TickType_t nextWakeTime = *pxPreviousWakeTime + xTimeIncrement;
  TickType_t now = getCurrentTickCount();

  if (nextWakeTime > now) {
    TickType_t delayTicks = nextWakeTime - now;
    std::this_thread::sleep_for(std::chrono::milliseconds(delayTicks));
    *pxPreviousWakeTime = nextWakeTime;
    return pdTRUE;
  } else {
    // We're already past the expected time — no delay
    *pxPreviousWakeTime = now;
    return pdFALSE;
  }
}

BaseType_t xTaskCreateUniversal(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t usStackDepth,
                                void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                const BaseType_t xCoreID) {
#ifndef CONFIG_FREERTOS_UNICORE
  if (xCoreID >= 0 && xCoreID < 2) {
    return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID);
  } else {
#endif
    return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
#ifndef CONFIG_FREERTOS_UNICORE
  }
#endif
}

BaseType_t xTaskCreate(
    TaskFunction_t pxTaskCode,
    const char* const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
    const uint16_t usStackDepth, void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask) {
  return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask,
                                 tskNO_AFFINITY);
}

TickType_t xTaskGetTickCount(void) {
  return getCurrentTickCount();
}

void vTaskDelay(unsigned int duration) {
  std::this_thread::sleep_for(std::chrono::milliseconds(duration * TICK_DURATION.count()));
}
