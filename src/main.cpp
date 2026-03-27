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
  PRESS_SEARCH_MINUS,
  PRESS_SEARCH_PLUS,
  PRESS_SEARCH_CANCEL,
  PRESS_SEARCH_OPEN,
  PRESS_NAV_PREV,
  PRESS_NAV_NEXT,
  PRESS_TAB_0,
};

PressedControl getPressedControl(int tx, int ty, ScreenMode mode) {
  if (mode == SCREEN_SEARCH) {
    if (hitTest(tx, ty, 24, 72, 62, 70, 10)) return PRESS_SEARCH_MINUS;
    if (hitTest(tx, ty, 234, 72, 62, 70, 10)) return PRESS_SEARCH_PLUS;
    if (hitTest(tx, ty, 24, 196, 126, 34, 10)) return PRESS_SEARCH_CANCEL;
    if (hitTest(tx, ty, 170, 196, 126, 34, 10)) return PRESS_SEARCH_OPEN;
    return PRESS_NONE;
  }

  if (hitTest(tx, ty, 240, 6, 70, 36, 8)) return PRESS_SEARCH_HEADER;
  if (tx < 70 && ty >= 48 && ty <= 200) return PRESS_NAV_PREV;
  if (tx > 250 && ty >= 48 && ty <= 200) return PRESS_NAV_NEXT;
  if (ty >= 194) return static_cast<PressedControl>(PRESS_TAB_0 + constrain(tx / (SCREEN_WIDTH / 5), 0, 4));
  return PRESS_NONE;
}

enum PendingActionType {
  ACTION_NONE = 0,
  ACTION_OPEN_SEARCH,
  ACTION_SEARCH_MINUS,
  ACTION_SEARCH_PLUS,
  ACTION_SEARCH_CANCEL,
  ACTION_SEARCH_OPEN,
  ACTION_NAV_PREV,
  ACTION_NAV_NEXT,
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
        if (pressedControl == PRESS_SEARCH_MINUS) {
          pendingAction = makePendingAction(ACTION_SEARCH_MINUS);
        } else if (pressedControl == PRESS_SEARCH_PLUS) {
          pendingAction = makePendingAction(ACTION_SEARCH_PLUS);
        } else if (pressedControl == PRESS_SEARCH_CANCEL) {
          pendingAction = makePendingAction(ACTION_SEARCH_CANCEL);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN);
        }
      } else {
        if (pressedControl == PRESS_SEARCH_HEADER) {
          pendingAction = makePendingAction(ACTION_OPEN_SEARCH);
        } else if (pressedControl == PRESS_NAV_PREV && currentId > MIN_POKEMON_ID) {
          pendingAction = makePendingAction(ACTION_NAV_PREV);
        } else if (pressedControl == PRESS_NAV_NEXT && currentId < MAX_POKEMON_ID) {
          pendingAction = makePendingAction(ACTION_NAV_NEXT);
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
      ui.drawSearchScreen(
          searchId,
          visualControl == PRESS_SEARCH_MINUS,
          visualControl == PRESS_SEARCH_PLUS,
          visualControl == PRESS_SEARCH_CANCEL,
          visualControl == PRESS_SEARCH_OPEN);
    } else {
      ui.drawHeader(pk, visualControl == PRESS_SEARCH_HEADER);
      
      if (currentTab == TAB_APPEARANCE) ui.drawAppearanceTab(pk);
      else if (currentTab == TAB_DESCRIPTION) ui.drawDescriptionTab(pk);
      else if (currentTab == TAB_BODY) ui.drawBodyTab(pk);
      else if (currentTab == TAB_ABILITY) ui.drawAbilityTab(pk);
      else ui.drawEvolutionTab(pk);

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
      case ACTION_SEARCH_MINUS:
        searchId = (searchId <= MIN_POKEMON_ID) ? MAX_POKEMON_ID : (searchId - 1);
        break;
      case ACTION_SEARCH_PLUS:
        searchId = (searchId >= MAX_POKEMON_ID) ? MIN_POKEMON_ID : (searchId + 1);
        break;
      case ACTION_SEARCH_CANCEL:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_SEARCH_OPEN:
        currentId = searchId;
        returnId = currentId;
        screenMode = SCREEN_DETAIL;
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_NAV_PREV:
        currentId--;
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_NAV_NEXT:
        currentId++;
        dataMgr.loadPokemonDetail(currentId);
        break;
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
