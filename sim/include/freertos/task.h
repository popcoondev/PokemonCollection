#ifndef SIM_FREERTOS_TASK_H
#define SIM_FREERTOS_TASK_H

#include "freertos/FreeRTOS.h"

#include <thread>

using TaskHandle_t = std::thread*;

inline int xTaskCreatePinnedToCore(void (*task)(void*),
                                   const char*,
                                   unsigned,
                                   void* parameters,
                                   unsigned,
                                   TaskHandle_t* taskHandle,
                                   int) {
  if (task == nullptr) {
    return 0;
  }
  std::thread* worker = new std::thread([task, parameters] {
    task(parameters);
  });
  worker->detach();
  if (taskHandle != nullptr) {
    *taskHandle = worker;
  }
  return pdTRUE;
}

inline TickType_t pdMS_TO_TICKS(TickType_t ms) {
  return ms;
}

#endif
