#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "ImageLoader.h"
#include <M5Unified.h>
#include <vector>
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
  void drawEvolutionTab(const PokemonDetail& pk, int pressedEvolutionIndex = -1, bool drawImages = true);
  void drawDetailNavigation(bool prevPressed, bool nextPressed);
  void drawFullscreenPreview(bool drawImage = true, uint16_t pokemonId = 0);
  void drawPreviewPocScreen(uint16_t pokemonId, int shiftX, int shiftY);
  void drawPreviewPocScreenCached(LGFX_Sprite& silhouetteSprite, LGFX_Sprite& iconSprite, int shiftX, int shiftY, uint16_t transparentColor);
  void drawPreviewPocScreenLayered(
      LGFX_Sprite* backgroundSprite,
      LGFX_Sprite& silhouetteSprite,
      LGFX_Sprite& iconSprite,
      int shiftX,
      int shiftY,
      uint16_t transparentColor);
  void drawMenuScreen(bool pokedexPressed, bool quizPressed, bool preview3dEnabled, bool preview3dPressed, int selectedVolumeIndex, int pressedVolumeIndex);
  void drawQuizScreen(bool answerSide, uint16_t pokemonId, const String& answerName);
  void blitAppearanceImageToCanvas(LGFX_Sprite& imageSprite);
  void pushAppearanceImageToDisplay(LGFX_Sprite& imageSprite);
  void redrawDetailNavigationToDisplay();
  void blitPreviewImageToCanvas(LGFX_Sprite& imageSprite);
  void pushPreviewImageToDisplay(LGFX_Sprite& imageSprite);
  void blitEvolutionImageToCanvas(LGFX_Sprite& imageSprite, int imageIndex);
  void pushEvolutionImageToDisplay(LGFX_Sprite& imageSprite, int imageIndex);
  void drawSearchScreen(
      bool nameMode,
      uint16_t selectedId,
      const String& selectedName,
      const String& nameQuery,
      const std::vector<uint16_t>& nameCandidateIds,
      const std::vector<String>& nameCandidateLabels,
      int pressedDigitDelta,
      bool menuPressed,
      bool modePressed,
      int pressedCandidateIndex,
      bool pagePrevPressed,
      bool pageNextPressed,
      bool cancelPressed,
      bool openPressed);
  void drawSearchInputScreen(
      const String& nameQuery,
      bool vowelMode,
      const String& selectedRowLabel,
      bool backPressed,
      bool clearPressed,
      bool deletePressed,
      int pressedKeyIndex);
  
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
  void drawQuizSilhouetteImage(uint16_t pokemonId, int x, int y, int w, int h);
  void drawQuizPokemonImage(uint16_t pokemonId, int x, int y, int w, int h);
};

#endif
