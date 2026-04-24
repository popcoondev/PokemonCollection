#ifndef APP_PLATFORM_H
#define APP_PLATFORM_H

#include <Arduino.h>

unsigned long appMillis();
void appDelay(uint32_t ms);
void appSeedRandom();
uint32_t appRandomU32();
void* appAllocateImageBuffer(size_t size);

#endif
