#include "AppPower.h"

#ifdef POKEMONCOLLECTION_SIM

#include "SimPlatform.h"

void appSetVibration(uint8_t level) {
  (void)level;
}

int appGetBatteryLevel() {
  return SimPlatform::getBatteryLevel();
}

bool appIsCharging() {
  return SimPlatform::isCharging();
}

#else

#include <M5Unified.h>

void appSetVibration(uint8_t level) {
  M5.Power.setVibration(level);
}

int appGetBatteryLevel() {
  return M5.Power.getBatteryLevel();
}

bool appIsCharging() {
  return M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
}

#endif
