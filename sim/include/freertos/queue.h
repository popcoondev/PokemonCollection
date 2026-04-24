#ifndef SIM_FREERTOS_QUEUE_H
#define SIM_FREERTOS_QUEUE_H

#include "freertos/FreeRTOS.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <vector>

struct SimQueue {
  SimQueue(unsigned length, unsigned itemSize)
      : capacity(length == 0 ? 1u : length), itemSize(itemSize) {}

  const unsigned capacity;
  const unsigned itemSize;
  std::mutex mutex;
  std::condition_variable cv;
  std::deque<std::vector<uint8_t>> items;
};

using QueueHandle_t = SimQueue*;

inline QueueHandle_t xQueueCreate(unsigned length, unsigned itemSize) {
  return new SimQueue(length, itemSize);
}

inline int xQueueSend(QueueHandle_t queue, const void* item, TickType_t) {
  if (queue == nullptr || item == nullptr) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(queue->mutex);
  if (queue->items.size() >= queue->capacity) {
    return 0;
  }
  queue->items.emplace_back(queue->itemSize);
  std::memcpy(queue->items.back().data(), item, queue->itemSize);
  queue->cv.notify_one();
  return pdTRUE;
}

inline int xQueueOverwrite(QueueHandle_t queue, const void* item) {
  if (queue == nullptr || item == nullptr) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(queue->mutex);
  if (queue->capacity == 1) {
    queue->items.clear();
  } else if (queue->items.size() >= queue->capacity) {
    queue->items.pop_front();
  }
  queue->items.emplace_back(queue->itemSize);
  std::memcpy(queue->items.back().data(), item, queue->itemSize);
  queue->cv.notify_one();
  return pdTRUE;
}

inline int xQueueReceive(QueueHandle_t queue, void* item, TickType_t ticksToWait) {
  if (queue == nullptr || item == nullptr) {
    return 0;
  }
  std::unique_lock<std::mutex> lock(queue->mutex);
  if (queue->items.empty()) {
    if (ticksToWait == 0) {
      return 0;
    }
    if (ticksToWait == portMAX_DELAY) {
      queue->cv.wait(lock, [&] { return !queue->items.empty(); });
    } else {
      queue->cv.wait_for(lock, std::chrono::milliseconds(ticksToWait), [&] { return !queue->items.empty(); });
      if (queue->items.empty()) {
        return 0;
      }
    }
  }
  std::memcpy(item, queue->items.front().data(), queue->itemSize);
  queue->items.pop_front();
  return pdTRUE;
}

#endif
