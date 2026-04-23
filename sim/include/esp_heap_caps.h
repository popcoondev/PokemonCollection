#ifndef SIM_ESP_HEAP_CAPS_H
#define SIM_ESP_HEAP_CAPS_H

#include <cstdlib>

inline constexpr int MALLOC_CAP_SPIRAM = 0x1;
inline constexpr int MALLOC_CAP_8BIT = 0x2;

inline void* heap_caps_malloc(size_t size, int caps) {
  (void)caps;
  return std::malloc(size);
}

#endif
