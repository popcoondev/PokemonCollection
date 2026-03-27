#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "ImageLoader.h"
#include <M5Unified.h>
#include "Config.h"
#include "DataManager.h"

class UIController {
public:
  UIController();
  ~UIController();
  bool begin();
  void drawBase();
  void drawHeader(const PokemonDetail& pk, bool searchPressed);
  void drawTabBar(TabType activeTab, int pressedTab);
  
  void drawAppearanceTab(const PokemonDetail& pk);
  void drawDescriptionTab(const PokemonDetail& pk);
  void drawBodyTab(const PokemonDetail& pk);
  void drawAbilityTab(const PokemonDetail& pk);
  void drawEvolutionTab(const PokemonDetail& pk);
  void drawDetailNavigation(bool prevPressed, bool nextPressed);
  void drawSearchScreen(
      uint16_t selectedId,
      bool minusPressed,
      bool plusPressed,
      bool cancelPressed,
      bool openPressed);
  
  void pushToDisplay();

private:
  LGFX_Sprite* sprite;
  ImageLoader imageLoader;
  void drawActionButton(
      int x,
      int y,
      int w,
      int h,
      const char* label,
      uint16_t fillColor,
      uint16_t textColor,
      bool pressed,
      uint16_t pressedFillColor,
      uint16_t borderColor);
  void drawNavigationButton(int cx, int cy, bool isLeft, bool pressed);
  void drawInfoRow(const char* label, const String& value, int y);
  void drawTypeBadge(const String& type, int x, int y);
  void drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines);
};

#endif
