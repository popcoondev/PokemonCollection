#include "AppConcurrency.h"

QueueHandle_t appQueueCreate(size_t length, size_t itemSize) {
  return xQueueCreate(static_cast<UBaseType_t>(length), static_cast<UBaseType_t>(itemSize));
}

BaseType_t appQueueSend(QueueHandle_t queue, const void* item, TickType_t ticksToWait) {
  return xQueueSend(queue, item, ticksToWait);
}

BaseType_t appQueueOverwrite(QueueHandle_t queue, const void* item) {
  return xQueueOverwrite(queue, item);
}

BaseType_t appQueueReceive(QueueHandle_t queue, void* item, TickType_t ticksToWait) {
  return xQueueReceive(queue, item, ticksToWait);
}

SemaphoreHandle_t appMutexCreate() {
  return xSemaphoreCreateMutex();
}

BaseType_t appMutexTake(SemaphoreHandle_t mutex, TickType_t ticksToWait) {
  return xSemaphoreTake(mutex, ticksToWait);
}

BaseType_t appMutexGive(SemaphoreHandle_t mutex) {
  return xSemaphoreGive(mutex);
}

BaseType_t appTaskCreatePinnedToCore(
    TaskFunction_t taskCode,
    const char* name,
    uint32_t stackDepth,
    void* parameters,
    UBaseType_t priority,
    TaskHandle_t* taskHandle,
    BaseType_t coreId) {
  return xTaskCreatePinnedToCore(taskCode, name, stackDepth, parameters, priority, taskHandle, coreId);
}

TickType_t appMsToTicks(uint32_t ms) {
  return pdMS_TO_TICKS(ms);
}
