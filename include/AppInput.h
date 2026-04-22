#ifndef APP_INPUT_H
#define APP_INPUT_H

#include <Arduino.h>
#include <M5Unified.h>

struct AppTouchSnapshot {
  bool active = false;
  bool clicked = false;
  int16_t x = 0;
  int16_t y = 0;
};

AppTouchSnapshot readPrimaryTouchSnapshot();
void configureDigitalButtonInputs(const gpio_num_t* pins, size_t pinCount);
bool readAnyDigitalButtonPressed(const gpio_num_t* pins, size_t pinCount);

#endif
