#ifndef SIM_FREERTOS_SEMPHR_H
#define SIM_FREERTOS_SEMPHR_H

#include "freertos/FreeRTOS.h"

using SemaphoreHandle_t = void*;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  return reinterpret_cast<SemaphoreHandle_t>(1);
}

inline int xSemaphoreTake(SemaphoreHandle_t, TickType_t) {
  return pdTRUE;
}

inline int xSemaphoreGive(SemaphoreHandle_t) {
  return pdTRUE;
}

#endif
