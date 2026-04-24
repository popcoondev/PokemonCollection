#ifndef SIM_FREERTOS_TASK_H
#define SIM_FREERTOS_TASK_H

#include "freertos/FreeRTOS.h"

using TaskHandle_t = void*;

inline int xTaskCreatePinnedToCore(void (*)(void*), const char*, unsigned, void*, unsigned, TaskHandle_t*, int) {
  return 0;
}

inline TickType_t pdMS_TO_TICKS(TickType_t ms) {
  return ms;
}

#endif
