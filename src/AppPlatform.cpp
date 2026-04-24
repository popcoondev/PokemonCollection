#include "AppPlatform.h"

#ifdef POKEMONCOLLECTION_SIM
#include <SDL.h>
#include <cstdlib>
#include <ctime>
#include "SimPlatform.h"
#else
#include <esp_heap_caps.h>
#include <esp_system.h>
#endif

unsigned long appMillis() {
#ifdef POKEMONCOLLECTION_SIM
  return static_cast<unsigned long>(SDL_GetTicks());
#else
  return millis();
#endif
}

void appDelay(uint32_t ms) {
#ifdef POKEMONCOLLECTION_SIM
  SDL_Delay(ms);
#else
  delay(ms);
#endif
}

void appSeedRandom() {
#ifdef POKEMONCOLLECTION_SIM
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
#else
  randomSeed(static_cast<uint32_t>(esp_random()));
#endif
}

uint32_t appRandomU32() {
#ifdef POKEMONCOLLECTION_SIM
  return static_cast<uint32_t>(std::rand());
#else
  return static_cast<uint32_t>(esp_random());
#endif
}

void* appAllocateImageBuffer(size_t size) {
#ifdef POKEMONCOLLECTION_SIM
  return std::malloc(size);
#else
  return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
}

bool appConsumeExternalRedrawRequest() {
#ifdef POKEMONCOLLECTION_SIM
  return SimPlatform::consumeRedrawRequest();
#else
  return false;
#endif
}
