#ifndef SIM_FREERTOS_H
#define SIM_FREERTOS_H

#include <cstdint>

using TickType_t = uint32_t;
using BaseType_t = int;
using UBaseType_t = unsigned;
using TaskFunction_t = void (*)(void*);

#define portMAX_DELAY 0xffffffffu
#define pdTRUE 1

#endif
