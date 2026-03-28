#include <M5Unified.h>
#include <SD.h>
#include "Config.h"
#include "DataManager.h"
#include "UIController.h"

DataManager dataMgr;
UIController ui;
uint16_t currentId = 1;
uint16_t searchId = 1;
uint16_t returnId = 1;
TabType currentTab = TAB_APPEARANCE;

enum ScreenMode {
  SCREEN_DETAIL = 0,
  SCREEN_SEARCH,
};

ScreenMode screenMode = SCREEN_DETAIL;

namespace {
bool hitTest(int tx, int ty, int x, int y, int w, int h, int pad = 0) {
  return tx >= (x - pad) && tx <= (x + w + pad)
      && ty >= (y - pad) && ty <= (y + h + pad);
}

enum PressedControl {
  PRESS_NONE = 0,
  PRESS_SEARCH_HEADER,
  PRESS_SEARCH_DIGIT_UP_0,
  PRESS_SEARCH_DIGIT_UP_1,
  PRESS_SEARCH_DIGIT_UP_2,
  PRESS_SEARCH_DIGIT_UP_3,
  PRESS_SEARCH_DIGIT_DOWN_0,
  PRESS_SEARCH_DIGIT_DOWN_1,
  PRESS_SEARCH_DIGIT_DOWN_2,
  PRESS_SEARCH_DIGIT_DOWN_3,
  PRESS_SEARCH_CANCEL,
  PRESS_SEARCH_OPEN,
  PRESS_NAV_PREV,
  PRESS_NAV_NEXT,
  PRESS_EVOLUTION_0,
  PRESS_EVOLUTION_1,
  PRESS_EVOLUTION_2,
  PRESS_TAB_0 = 32,
};

PressedControl getPressedControl(int tx, int ty, ScreenMode mode) {
  if (mode == SCREEN_SEARCH) {
    const int digitX[4] = {70, 115, 160, 205};
    for (int i = 0; i < 4; ++i) {
      if (hitTest(tx, ty, digitX[i], 66, 32, 28, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_UP_0 + i);
      if (hitTest(tx, ty, digitX[i], 148, 32, 28, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_DOWN_0 + i);
    }
    if (hitTest(tx, ty, 24, 196, 126, 34, 10)) return PRESS_SEARCH_CANCEL;
    if (hitTest(tx, ty, 170, 196, 126, 34, 10)) return PRESS_SEARCH_OPEN;
    return PRESS_NONE;
  }

  if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12)) return PRESS_SEARCH_HEADER;
  if (hitTest(tx, ty, 0, 0, 40, 204)) return PRESS_NAV_PREV;
  if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, 204)) return PRESS_NAV_NEXT;
  if (hitTest(tx, ty, 22, 80, 84, 80)) return PRESS_EVOLUTION_0;
  if (hitTest(tx, ty, 118, 80, 84, 80)) return static_cast<PressedControl>(PRESS_EVOLUTION_0 + 1);
  if (hitTest(tx, ty, 214, 80, 84, 80)) return static_cast<PressedControl>(PRESS_EVOLUTION_0 + 2);
  if (ty >= 194) return static_cast<PressedControl>(PRESS_TAB_0 + constrain(tx / (SCREEN_WIDTH / 5), 0, 4));
  return PRESS_NONE;
}

enum PendingActionType {
  ACTION_NONE = 0,
  ACTION_OPEN_SEARCH,
  ACTION_SEARCH_ADJUST,
  ACTION_SEARCH_CANCEL,
  ACTION_SEARCH_OPEN,
  ACTION_NAV_PREV,
  ACTION_NAV_NEXT,
  ACTION_OPEN_EVOLUTION,
  ACTION_SET_TAB,
};

struct PendingAction {
  PendingActionType type = ACTION_NONE;
  int value = 0;
};

PendingAction makePendingAction(PendingActionType type, int value = 0) {
  PendingAction action;
  action.type = type;
  action.value = value;
  return action;
}

uint16_t adjustSearchDigit(uint16_t currentValue, int placeValue, int direction) {
  const int digit = (currentValue / placeValue) % 10;
  const int nextDigit = (digit + direction + 10) % 10;
  return static_cast<uint16_t>(currentValue + ((nextDigit - digit) * placeValue));
}
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextFont(2);

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    M5.Display.print("SD Init Error");
    while(1) delay(100);
  }

  if (!dataMgr.begin()) {
    M5.Display.print("SD Error");
    while(1) delay(100);
  }

  if (!ui.begin()) {
    M5.Display.print("UI Error");
    while(1) delay(100);
  }
  dataMgr.loadPokemonDetail(currentId);
}

void loop() {
  M5.update();
  static PressedControl heldControl = PRESS_NONE;
  static PressedControl latchedControl = PRESS_NONE;
  static PendingAction pendingAction;
  static bool needsRedraw = true;
  static TabType lastRenderedTab = TAB_APPEARANCE;

  PressedControl pressedControl = PRESS_NONE;

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    pressedControl = getPressedControl(t.x, t.y, screenMode);
    if (pressedControl != heldControl) {
      heldControl = pressedControl;
      if (latchedControl == PRESS_NONE) {
        needsRedraw = true;
      }
    }
    if (t.wasClicked()) {
      latchedControl = pressedControl;
      needsRedraw = true;

      if (screenMode == SCREEN_SEARCH) {
        if (pressedControl >= PRESS_SEARCH_DIGIT_UP_0 && pressedControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, digitStep[pressedControl - PRESS_SEARCH_DIGIT_UP_0]);
        } else if (pressedControl >= PRESS_SEARCH_DIGIT_DOWN_0 && pressedControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, -digitStep[pressedControl - PRESS_SEARCH_DIGIT_DOWN_0]);
        } else if (pressedControl == PRESS_SEARCH_CANCEL) {
          pendingAction = makePendingAction(ACTION_SEARCH_CANCEL);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN);
        }
      } else {
        if (pressedControl == PRESS_SEARCH_HEADER) {
          pendingAction = makePendingAction(ACTION_OPEN_SEARCH);
        } else if (pressedControl == PRESS_NAV_PREV) {
          pendingAction = makePendingAction(ACTION_NAV_PREV);
        } else if (pressedControl == PRESS_NAV_NEXT) {
          pendingAction = makePendingAction(ACTION_NAV_NEXT);
        } else if (currentTab == TAB_EVOLUTION
            && pressedControl >= PRESS_EVOLUTION_0
            && pressedControl <= (PRESS_EVOLUTION_0 + 2)) {
          pendingAction = makePendingAction(ACTION_OPEN_EVOLUTION, pressedControl - PRESS_EVOLUTION_0);
        } else if (pressedControl >= PRESS_TAB_0 && pressedControl <= (PRESS_TAB_0 + 4)) {
          pendingAction = makePendingAction(ACTION_SET_TAB, pressedControl - PRESS_TAB_0);
        }
      }
    }
  } else if (heldControl != PRESS_NONE) {
    heldControl = PRESS_NONE;
    if (latchedControl == PRESS_NONE) {
      needsRedraw = true;
    }
  }

  const PressedControl visualControl = (latchedControl != PRESS_NONE) ? latchedControl : heldControl;
  const bool tabChanged = lastRenderedTab != currentTab;
  if (tabChanged) {
    needsRedraw = true;
    lastRenderedTab = currentTab;
  }

  if (needsRedraw) {
    ui.drawBase();
    const auto& pk = dataMgr.getCurrentPokemon();

    if (screenMode == SCREEN_SEARCH) {
      int pressedDigitDelta = 0;
      if (visualControl >= PRESS_SEARCH_DIGIT_UP_0 && visualControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = digitStep[visualControl - PRESS_SEARCH_DIGIT_UP_0];
      } else if (visualControl >= PRESS_SEARCH_DIGIT_DOWN_0 && visualControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = -digitStep[visualControl - PRESS_SEARCH_DIGIT_DOWN_0];
      }
      ui.drawSearchScreen(
          searchId,
          dataMgr.getPokemonName(searchId),
          pressedDigitDelta,
          visualControl == PRESS_SEARCH_CANCEL,
          visualControl == PRESS_SEARCH_OPEN);
    } else {
      ui.drawHeader(pk, visualControl == PRESS_SEARCH_HEADER);
      
      if (currentTab == TAB_APPEARANCE) ui.drawAppearanceTab(pk);
      else if (currentTab == TAB_DESCRIPTION) ui.drawDescriptionTab(pk);
      else if (currentTab == TAB_BODY) ui.drawBodyTab(pk);
      else if (currentTab == TAB_ABILITY) ui.drawAbilityTab(pk);
      else ui.drawEvolutionTab(
          pk,
          (visualControl >= PRESS_EVOLUTION_0 && visualControl <= (PRESS_EVOLUTION_0 + 2))
              ? (visualControl - PRESS_EVOLUTION_0)
              : -1);

      ui.drawDetailNavigation(visualControl == PRESS_NAV_PREV, visualControl == PRESS_NAV_NEXT);

      ui.drawTabBar(
          currentTab,
          (visualControl >= PRESS_TAB_0 && visualControl <= (PRESS_TAB_0 + 4))
              ? (visualControl - PRESS_TAB_0)
              : -1);
    }

    ui.pushToDisplay();
    needsRedraw = false;
  }

  if (pendingAction.type != ACTION_NONE && latchedControl != PRESS_NONE) {
    switch (pendingAction.type) {
      case ACTION_OPEN_SEARCH:
        screenMode = SCREEN_SEARCH;
        returnId = currentId;
        searchId = currentId;
        break;
      case ACTION_SEARCH_ADJUST:
        searchId = adjustSearchDigit(
            searchId,
            pendingAction.value > 0 ? pendingAction.value : -pendingAction.value,
            pendingAction.value > 0 ? 1 : -1);
        break;
      case ACTION_SEARCH_CANCEL:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_SEARCH_OPEN:
        if (searchId >= MIN_POKEMON_ID
            && searchId <= MAX_POKEMON_ID
            && dataMgr.getPokemonName(searchId).length() > 0) {
          currentId = searchId;
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      case ACTION_NAV_PREV:
        currentId = (currentId <= MIN_POKEMON_ID) ? MAX_POKEMON_ID : (currentId - 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_NAV_NEXT:
        currentId = (currentId >= MAX_POKEMON_ID) ? MIN_POKEMON_ID : (currentId + 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_OPEN_EVOLUTION: {
        const auto& pk = dataMgr.getCurrentPokemon();
        const int index = pendingAction.value;
        if (index >= 0 && index < static_cast<int>(pk.evolutions.size())) {
          currentId = pk.evolutions[index].id;
          currentTab = TAB_APPEARANCE;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      }
      case ACTION_SET_TAB:
        currentTab = static_cast<TabType>(pendingAction.value);
        break;
      case ACTION_NONE:
        break;
    }

    pendingAction = {};
    latchedControl = PRESS_NONE;
    if (heldControl == PRESS_NONE) {
      needsRedraw = true;
    }
  }

  delay(10);
}
