#ifndef _FREERTOS_H_
#define _FREERTOS_H_

#include "task.h"

#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;

const BaseType_t tskNO_AFFINITY = -1;

extern "C" {
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t ulStackDepth,
                                   void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                   const BaseType_t xCoreID);

void vTaskDelete(TaskHandle_t xTaskToDelete);
}

#endif
