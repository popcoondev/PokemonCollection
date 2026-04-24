#ifndef APP_CONCURRENCY_H
#define APP_CONCURRENCY_H

#include <cstddef>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

QueueHandle_t appQueueCreate(size_t length, size_t itemSize);
BaseType_t appQueueSend(QueueHandle_t queue, const void* item, TickType_t ticksToWait);
BaseType_t appQueueOverwrite(QueueHandle_t queue, const void* item);
BaseType_t appQueueReceive(QueueHandle_t queue, void* item, TickType_t ticksToWait);

SemaphoreHandle_t appMutexCreate();
BaseType_t appMutexTake(SemaphoreHandle_t mutex, TickType_t ticksToWait);
BaseType_t appMutexGive(SemaphoreHandle_t mutex);

BaseType_t appTaskCreatePinnedToCore(
    TaskFunction_t taskCode,
    const char* name,
    uint32_t stackDepth,
    void* parameters,
    UBaseType_t priority,
    TaskHandle_t* taskHandle,
    BaseType_t coreId);

TickType_t appMsToTicks(uint32_t ms);

#endif
