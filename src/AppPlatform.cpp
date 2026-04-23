#include "AppPlatform.h"

#ifdef POKEMONCOLLECTION_SIM
#include <SDL.h>
#include <cstdlib>
#include <ctime>
#else
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
