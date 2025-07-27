#ifndef EMUL_FREERTOS_H
#define EMUL_FREERTOS_H

#include "task.h"

#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;

struct tskTaskControlBlock;
typedef struct tskTaskControlBlock* TaskHandle_t;

typedef void (*TaskFunction_t)(void*);

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t ulStackDepth,
                                   void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                   const BaseType_t xCoreID);

#define CONFIG_FREERTOS_HZ 1000
#define configTICK_RATE_HZ CONFIG_FREERTOS_HZ

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) \
  ((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000U))
#endif

#endif
