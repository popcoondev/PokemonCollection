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
  
  void drawAppearanceTab(const PokemonDetail& pk, bool drawImage = true);
  void drawDescriptionTab(const PokemonDetail& pk);
  void drawBodyTab(const PokemonDetail& pk);
  void drawAbilityTab(const PokemonDetail& pk);
  void drawEvolutionTab(const PokemonDetail& pk, int pressedEvolutionIndex = -1);
  void drawDetailNavigation(bool prevPressed, bool nextPressed);
  void drawFullscreenPreview(bool drawImage = true, uint16_t pokemonId = 0);
  void blitAppearanceImageToCanvas(LGFX_Sprite& imageSprite);
  void pushAppearanceImageToDisplay(LGFX_Sprite& imageSprite);
  void redrawDetailNavigationToDisplay();
  void blitPreviewImageToCanvas(LGFX_Sprite& imageSprite);
  void pushPreviewImageToDisplay(LGFX_Sprite& imageSprite);
  void drawSearchScreen(
      uint16_t selectedId,
      const String& selectedName,
      int pressedDigitDelta,
      bool cancelPressed,
      bool openPressed);
  
  void pushToDisplay();

private:
  LGFX_Sprite* sprite;
  ImageLoader imageLoader;
  void drawPressedOverlay(int x, int y, int w, int h, int radius = 0);
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
  const lgfx::IFont* getFallbackFont(const lgfx::IFont* primaryFont) const;
  const lgfx::IFont* selectFontForCodepoint(uint16_t codepoint, const lgfx::IFont* primaryFont) const;
  size_t readUtf8Glyph(const String& text, size_t index, uint16_t& codepoint, String& glyph) const;
  void drawInfoRow(const char* label, const String& value, int y);
  void drawTypeBadge(const String& type, int x, int y);
  void drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines);
};

#endif
