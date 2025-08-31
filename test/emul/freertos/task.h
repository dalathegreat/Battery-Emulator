#ifndef FREERTOS_TASK_H
#define FREERTOS_TASK_H

#include <stdint.h>
#include <chrono>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;

struct tskTaskControlBlock;
typedef struct tskTaskControlBlock* TaskHandle_t;

typedef void (*TaskFunction_t)(void*);

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t ulStackDepth,
                                   void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                   const BaseType_t xCoreID);

BaseType_t xTaskCreateUniversal(TaskFunction_t pxTaskCode, const char* const pcName, const uint32_t usStackDepth,
                                void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask,
                                const BaseType_t xCoreID);

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char* const pcName, const uint16_t usStackDepth,
                       void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask);

#define CONFIG_FREERTOS_HZ 1000
#define configTICK_RATE_HZ CONFIG_FREERTOS_HZ

#define pdFALSE ((BaseType_t)0)
#define pdTRUE ((BaseType_t)1)

#define pdPASS (pdTRUE)
#define pdFAIL (pdFALSE)

/*#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) (std::chrono::milliseconds(xTimeInMs))
#endif*/
#define pdMS_TO_TICKS(xTimeInMs) (xTimeInMs)

#define portTICK_PERIOD_MS ((TickType_t)1000 / configTICK_RATE_HZ)

#define portMAX_DELAY (TickType_t)0xffffffffUL

void vTaskDelete(TaskHandle_t xTaskToDelete);

struct tskTaskControlBlock; /* The old naming convention is used to prevent breaking kernel aware debuggers. */
typedef struct tskTaskControlBlock* TaskHandle_t;
//typedef std::chrono::milliseconds TickType_t;
typedef uint32_t TickType_t;

TickType_t xTaskGetTickCount(void);

BaseType_t xTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement);

void vTaskDelay(const TickType_t xTicksToDelay);

/** @cond !DOC_EXCLUDE_HEADER_SECTION */

/*
 * vTaskDelayUntil() is the older version of xTaskDelayUntil() and does not
 * return a value.
 */
#define vTaskDelayUntil(pxPreviousWakeTime, xTimeIncrement)        \
  do {                                                             \
    (void)xTaskDelayUntil((pxPreviousWakeTime), (xTimeIncrement)); \
  } while (0)

#define tskNO_AFFINITY ((BaseType_t)0x7FFFFFFF)

inline void taskYIELD(void) {}

#endif
