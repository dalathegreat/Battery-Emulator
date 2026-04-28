#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE 0
#define configCPU_CLOCK_HZ ((unsigned long)1000000)
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configMAX_PRIORITIES (20)
#define configMINIMAL_STACK_SIZE ((unsigned short)128)
#define configMAX_TASK_NAME_LEN (16)
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_ALTERNATIVE_API 0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE 8
#define configUSE_QUEUE_SETS 0
#define configUSE_TIME_SLICING 1
#define configUSE_NEWLIB_REENTRANT 0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE ((size_t)(128 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP 0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS 1
#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE() 0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES (2)

/* Software timer related definitions. */
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH 10
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2)

/* Define to trap errors during development. */
#define configASSERT(x)         \
  if ((x) == 0) {               \
    printf("ASSERT: %s\n", #x); \
  }

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xResumeFromISR 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xEventGroupSetBitFromISR 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_xTaskAbortDelay 0
#define INCLUDE_xTaskGetHandle 0
#define INCLUDE_xTaskResumeFromISR 1

/* ESP32 Compatibility */
#define xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID) \
  xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask)
#define tskNO_AFFINITY ((BaseType_t) - 1)

#endif /* FREERTOS_CONFIG_H */
