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
  void drawHeader(const PokemonDetail& pk);
  void drawTabBar(TabType activeTab);
  
  void drawAppearanceTab(const PokemonDetail& pk);
  void drawDescriptionTab(const PokemonDetail& pk);
  void drawBodyTab(const PokemonDetail& pk);
  void drawAbilityTab(const PokemonDetail& pk);
  void drawEvolutionTab(const PokemonDetail& pk);
  
  void pushToDisplay();

private:
  LGFX_Sprite* sprite;
  ImageLoader imageLoader;
  void drawInfoRow(const char* label, const String& value, int y);
  void drawTypeBadge(const String& type, int x, int y);
  void drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines);
};

#endif
