#include "FreeRTOS.h"

extern "C" {
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t ulStackDepth,
                                   void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                   const BaseType_t xCoreID) {
  return 0;
}
void vTaskDelete(TaskHandle_t xTaskToDelete) {}
}
