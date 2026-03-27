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

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    if (t.wasClicked()) {
      if (screenMode == SCREEN_SEARCH) {
        if (t.x >= 24 && t.x <= 86 && t.y >= 72 && t.y <= 142 && searchId > MIN_POKEMON_ID) {
          searchId--;
          dataMgr.loadPokemonDetail(searchId);
        } else if (t.x >= 234 && t.x <= 296 && t.y >= 72 && t.y <= 142 && searchId < MAX_POKEMON_ID) {
          searchId++;
          dataMgr.loadPokemonDetail(searchId);
        } else if (t.x >= 24 && t.x <= 150 && t.y >= 202 && t.y <= 228) {
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(returnId);
        } else if (t.x >= 170 && t.x <= 296 && t.y >= 202 && t.y <= 228) {
          currentId = searchId;
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
        }
      } else {
        if (t.x >= 248 && t.y <= HEADER_H) {
          screenMode = SCREEN_SEARCH;
          returnId = currentId;
          searchId = currentId;
          dataMgr.loadPokemonDetail(searchId);
        } else if (t.y > 200) {
          currentTab = (TabType)(t.x / (320 / 5));
        } else if (t.x < 50 && currentId > MIN_POKEMON_ID) {
          currentId--;
          dataMgr.loadPokemonDetail(currentId);
        } else if (t.x > 270 && currentId < MAX_POKEMON_ID) {
          currentId++;
          dataMgr.loadPokemonDetail(currentId);
        }
      }
    }
  }

  ui.drawBase();
  const auto& pk = dataMgr.getCurrentPokemon();

  if (screenMode == SCREEN_SEARCH) {
    ui.drawSearchScreen(pk, searchId);
  } else {
    ui.drawHeader(pk);
    
    if (currentTab == TAB_APPEARANCE) ui.drawAppearanceTab(pk);
    else if (currentTab == TAB_DESCRIPTION) ui.drawDescriptionTab(pk);
    else if (currentTab == TAB_BODY) ui.drawBodyTab(pk);
    else if (currentTab == TAB_ABILITY) ui.drawAbilityTab(pk);
    else ui.drawEvolutionTab(pk);

    ui.drawTabBar(currentTab);
  }

  ui.pushToDisplay();
  delay(10);
}
