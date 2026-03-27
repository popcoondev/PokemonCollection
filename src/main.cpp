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
  PressedControl pressedControl = PRESS_NONE;

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    pressedControl = getPressedControl(t.x, t.y, screenMode);
    if (t.wasClicked()) {
      if (screenMode == SCREEN_SEARCH) {
        if (pressedControl == PRESS_SEARCH_MINUS && searchId > MIN_POKEMON_ID) {
          searchId--;
          dataMgr.loadPokemonDetail(searchId);
        } else if (pressedControl == PRESS_SEARCH_PLUS && searchId < MAX_POKEMON_ID) {
          searchId++;
          dataMgr.loadPokemonDetail(searchId);
        } else if (pressedControl == PRESS_SEARCH_CANCEL) {
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(returnId);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          currentId = searchId;
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
        }
      } else {
        if (pressedControl == PRESS_SEARCH_HEADER) {
          screenMode = SCREEN_SEARCH;
          returnId = currentId;
          searchId = currentId;
          dataMgr.loadPokemonDetail(searchId);
        } else if (pressedControl == PRESS_NAV_PREV && currentId > MIN_POKEMON_ID) {
          currentId--;
          dataMgr.loadPokemonDetail(currentId);
        } else if (pressedControl == PRESS_NAV_NEXT && currentId < MAX_POKEMON_ID) {
          currentId++;
          dataMgr.loadPokemonDetail(currentId);
        } else if (pressedControl >= PRESS_TAB_0 && pressedControl <= (PRESS_TAB_0 + 4)) {
          currentTab = static_cast<TabType>(pressedControl - PRESS_TAB_0);
        }
      }
    }
  }

  ui.drawBase();
  const auto& pk = dataMgr.getCurrentPokemon();

  if (screenMode == SCREEN_SEARCH) {
    ui.drawSearchScreen(
        pk,
        searchId,
        pressedControl == PRESS_SEARCH_MINUS,
        pressedControl == PRESS_SEARCH_PLUS,
        pressedControl == PRESS_SEARCH_CANCEL,
        pressedControl == PRESS_SEARCH_OPEN);
  } else {
    ui.drawHeader(pk, pressedControl == PRESS_SEARCH_HEADER);
    
    if (currentTab == TAB_APPEARANCE) ui.drawAppearanceTab(pk);
    else if (currentTab == TAB_DESCRIPTION) ui.drawDescriptionTab(pk);
    else if (currentTab == TAB_BODY) ui.drawBodyTab(pk);
    else if (currentTab == TAB_ABILITY) ui.drawAbilityTab(pk);
    else ui.drawEvolutionTab(pk);

    ui.drawDetailNavigation(pressedControl == PRESS_NAV_PREV, pressedControl == PRESS_NAV_NEXT);

    ui.drawTabBar(
        currentTab,
        (pressedControl >= PRESS_TAB_0 && pressedControl <= (PRESS_TAB_0 + 4))
            ? (pressedControl - PRESS_TAB_0)
            : -1);
  }

  ui.pushToDisplay();
  delay(10);
}
