#ifndef APP_PLATFORM_H
#define APP_PLATFORM_H

#include <Arduino.h>

unsigned long appMillis();
void appDelay(uint32_t ms);
void appSeedRandom();

#endif
