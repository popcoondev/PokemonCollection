#ifndef SIM_FREERTOS_SEMPHR_H
#define SIM_FREERTOS_SEMPHR_H

#include "freertos/FreeRTOS.h"

#include <chrono>
#include <mutex>

struct SimMutex {
  std::timed_mutex mutex;
};

using SemaphoreHandle_t = SimMutex*;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  return new SimMutex();
}

inline int xSemaphoreTake(SemaphoreHandle_t handle, TickType_t ticks) {
  if (handle == nullptr) {
    return 0;
  }
  if (ticks == 0) {
    return handle->mutex.try_lock() ? pdTRUE : 0;
  }
  if (ticks == portMAX_DELAY) {
    handle->mutex.lock();
    return pdTRUE;
  }
  return handle->mutex.try_lock_for(std::chrono::milliseconds(ticks)) ? pdTRUE : 0;
}

inline int xSemaphoreGive(SemaphoreHandle_t handle) {
  if (handle == nullptr) {
    return 0;
  }
  handle->mutex.unlock();
  return pdTRUE;
}

#endif
