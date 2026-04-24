#include "AppInput.h"

#ifdef POKEMONCOLLECTION_SIM
#include "SimPlatform.h"
#endif

void appInputBeginFrame() {
#ifndef POKEMONCOLLECTION_SIM
  M5.update();
#endif
}

AppTouchSnapshot readPrimaryTouchSnapshot() {
  AppTouchSnapshot snapshot;
#ifdef POKEMONCOLLECTION_SIM
  const SimTouchState simTouch = SimPlatform::getTouchState();
  snapshot.active = simTouch.active;
  snapshot.clicked = simTouch.clicked;
  snapshot.x = simTouch.x;
  snapshot.y = simTouch.y;
  return snapshot;
#else
  if (M5.Touch.getCount() <= 0) {
    return snapshot;
  }

  auto detail = M5.Touch.getDetail(0);
  snapshot.active = true;
  snapshot.clicked = detail.wasClicked();
  snapshot.x = detail.x;
  snapshot.y = detail.y;
  return snapshot;
#endif
}

void configureDigitalButtonInputs(const gpio_num_t* pins, size_t pinCount) {
#ifdef POKEMONCOLLECTION_SIM
  (void)pins;
  (void)pinCount;
#else
  if (pins == nullptr) {
    return;
  }
  for (size_t i = 0; i < pinCount; ++i) {
    pinMode(static_cast<int>(pins[i]), INPUT_PULLUP);
  }
#endif
}

bool readAnyDigitalButtonPressed(const gpio_num_t* pins, size_t pinCount) {
#ifdef POKEMONCOLLECTION_SIM
  (void)pins;
  (void)pinCount;
  return SimPlatform::isDigitalButtonPressed();
#else
  if (pins == nullptr || pinCount == 0) {
    return false;
  }
  for (size_t i = 0; i < pinCount; ++i) {
    if (digitalRead(static_cast<int>(pins[i])) == LOW) {
      return true;
    }
  }
  return false;
#endif
}

AppButtonSnapshot readDebouncedDigitalButton(
    const gpio_num_t* pins,
    size_t pinCount,
    unsigned long now,
    uint32_t debounceMs,
    DebouncedButtonState& state) {
  AppButtonSnapshot snapshot;
  const bool rawPressed = readAnyDigitalButtonPressed(pins, pinCount);
  if (rawPressed != state.lastRawPressed) {
    state.lastRawPressed = rawPressed;
    state.lastChangeAt = now;
  }

  if ((now - state.lastChangeAt) >= debounceMs && state.stablePressed != rawPressed) {
    state.stablePressed = rawPressed;
    if (state.stablePressed) {
      snapshot.clicked = true;
    }
  }

  snapshot.pressed = state.stablePressed;
  return snapshot;
}
