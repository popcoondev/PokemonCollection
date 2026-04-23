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

struct AppButtonSnapshot {
  bool pressed = false;
  bool clicked = false;
};

struct DebouncedButtonState {
  bool stablePressed = false;
  bool lastRawPressed = false;
  unsigned long lastChangeAt = 0;
};

void appInputBeginFrame();
AppTouchSnapshot readPrimaryTouchSnapshot();
void configureDigitalButtonInputs(const gpio_num_t* pins, size_t pinCount);
bool readAnyDigitalButtonPressed(const gpio_num_t* pins, size_t pinCount);
AppButtonSnapshot readDebouncedDigitalButton(
    const gpio_num_t* pins,
    size_t pinCount,
    unsigned long now,
    uint32_t debounceMs,
    DebouncedButtonState& state);

#endif
