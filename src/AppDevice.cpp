#include "AppDevice.h"

#ifndef POKEMONCOLLECTION_SIM
#include <M5Unified.h>
#endif

void appDeviceBegin() {
#ifndef POKEMONCOLLECTION_SIM
  auto cfg = M5.config();
  cfg.internal_spk = true;
  cfg.internal_mic = false;
  M5.begin(cfg);
#endif
}
