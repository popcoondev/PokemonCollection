#include "AppStorage.h"

#ifdef POKEMONCOLLECTION_SIM
#include "SD.h"
bool appStorageBegin() {
  return SD.begin();
}
#else
#include <FS.h>
#include <SD.h>
#include <SPI.h>

bool appStorageBegin() {
  return SD.begin(GPIO_NUM_4, SPI, 25000000);
}
#endif
