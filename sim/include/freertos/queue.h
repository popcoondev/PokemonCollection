#ifndef SIM_FREERTOS_QUEUE_H
#define SIM_FREERTOS_QUEUE_H

#include "freertos/FreeRTOS.h"

using QueueHandle_t = void*;

inline QueueHandle_t xQueueCreate(unsigned, unsigned) {
  return reinterpret_cast<QueueHandle_t>(1);
}

inline int xQueueSend(QueueHandle_t, const void*, TickType_t) {
  return pdTRUE;
}

inline int xQueueOverwrite(QueueHandle_t, const void*) {
  return pdTRUE;
}

inline int xQueueReceive(QueueHandle_t, void*, TickType_t) {
  return 0;
}

#endif
