#include "AppInput.h"

AppTouchSnapshot readPrimaryTouchSnapshot() {
  AppTouchSnapshot snapshot;
  if (M5.Touch.getCount() <= 0) {
    return snapshot;
  }

  auto detail = M5.Touch.getDetail(0);
  snapshot.active = true;
  snapshot.clicked = detail.wasClicked();
  snapshot.x = detail.x;
  snapshot.y = detail.y;
  return snapshot;
}

void configureDigitalButtonInputs(const gpio_num_t* pins, size_t pinCount) {
  if (pins == nullptr) {
    return;
  }
  for (size_t i = 0; i < pinCount; ++i) {
    pinMode(static_cast<int>(pins[i]), INPUT_PULLUP);
  }
}

bool readAnyDigitalButtonPressed(const gpio_num_t* pins, size_t pinCount) {
  if (pins == nullptr || pinCount == 0) {
    return false;
  }
  for (size_t i = 0; i < pinCount; ++i) {
    if (digitalRead(static_cast<int>(pins[i])) == LOW) {
      return true;
    }
  }
  return false;
}
