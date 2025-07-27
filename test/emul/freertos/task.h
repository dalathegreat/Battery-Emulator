#ifndef FREERTOS_TASK_H
#define FREERTOS_TASK_H

#include <stdint.h>

struct tskTaskControlBlock; /* The old naming convention is used to prevent breaking kernel aware debuggers. */
typedef struct tskTaskControlBlock* TaskHandle_t;
typedef uint32_t TickType_t;

TickType_t xTaskGetTickCount(void);

#endif
