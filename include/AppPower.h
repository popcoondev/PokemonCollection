#ifndef APP_POWER_H
#define APP_POWER_H

#include <Arduino.h>

void appSetVibration(uint8_t level);
int appGetBatteryLevel();
bool appIsCharging();

#endif
