#include "UIController.h"
#include <SD.h>

namespace {
constexpr int kAppearanceImageX = 6;
constexpr int kAppearanceImageY = 54;
constexpr int kAppearanceImageW = 140;
constexpr int kAppearanceImageH = 140;
constexpr int kEvolutionCardX[3] = {22, 118, 214};
constexpr int kEvolutionCardY = 80;
constexpr int kEvolutionImageOffsetX = 17;
constexpr int kEvolutionImageOffsetY = 6;
constexpr int kEvolutionImageW = 50;
constexpr int kEvolutionImageH = 32;
constexpr const char* kQuizBackgroundPath = "/pokemon/quiz/backgrounds/quiz_bg.png";
constexpr const char* kWakeSplashPath = "/pokemon/splash/wake_splash.png";
constexpr uint16_t kWakeSplashAccentColor = 0x67BF;

struct UIThemePalette {
  uint16_t bg;
  uint16_t surface;
  uint16_t text;
  uint16_t sub;
  uint16_t border;
  uint16_t accent;
  uint16_t highlight;
  uint16_t pressedOverlay;
  uint16_t statusBarBg;
  uint16_t statusBarDivider;
  uint16_t buttonPressedFill;
  uint16_t invertedText;
  uint8_t buttonRadius;
};

const UIThemePalette& getThemePalette(UIThemeStyle theme) {
  static constexpr UIThemePalette kClassicTheme = {
      COLOR_PK_BG,
      COLOR_PK_CARD,
      COLOR_PK_TEXT,
      COLOR_PK_SUB,
      COLOR_PK_BORDER,
      COLOR_PK_RED,
      COLOR_PK_BAR,
      0xDEFB,
      COLOR_PK_CARD,
      COLOR_PK_BORDER,
      COLOR_PK_TEXT,
      COLOR_PK_CARD,
      10,
  };
  static constexpr UIThemePalette kCyberTheme = {
      0x10C4,
      0x192B,
      0xE73C,
      0x7DF7,
      0x3A10,
      0x051F,
      0x05BF,
      0x2D95,
      0x08A6,
      0x3A10,
      0x0430,
      0xFFFF,
      4,
  };

  switch (theme) {
    case UI_THEME_CYBER:
      return kCyberTheme;
    case UI_THEME_CLASSIC:
    default:
      return kClassicTheme;
  }
}

void drawBatteryIcon(LGFX_Sprite* sprite, const UIThemePalette& theme, int x, int y, int level, bool charging) {
  if (sprite == nullptr) {
    return;
  }
  const int width = 18;
  const int height = 10;
  const uint16_t outlineColor = theme.sub;
  uint16_t fillColor = theme.sub;
  if (level >= 60) {
    fillColor = 0x6E4D;
  } else if (level >= 25) {
    fillColor = 0xFDC0;
  } else if (level >= 0) {
    fillColor = theme.accent;
  }

  sprite->drawRoundRect(x, y, width, height, 2, outlineColor);
  sprite->fillRect(x + width, y + 3, 2, 4, outlineColor);

  const int clampedLevel = constrain(level, 0, 100);
  const int innerWidth = ((width - 4) * clampedLevel) / 100;
  if (innerWidth > 0) {
    sprite->fillRect(x + 2, y + 2, innerWidth, height - 4, fillColor);
  }

  if (charging) {
    sprite->setFont(&fonts::Font0);
    sprite->setTextColor(theme.text);
    sprite->drawString("+", x + 6, y + 1);
  }
}

uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha) {
  const uint8_t fgR = (fg >> 11) & 0x1F;
  const uint8_t fgG = (fg >> 5) & 0x3F;
  const uint8_t fgB = fg & 0x1F;
  const uint8_t bgR = (bg >> 11) & 0x1F;
  const uint8_t bgG = (bg >> 5) & 0x3F;
  const uint8_t bgB = bg & 0x1F;

  const uint8_t outR = ((fgR * alpha) + (bgR * (255 - alpha))) / 255;
  const uint8_t outG = ((fgG * alpha) + (bgG * (255 - alpha))) / 255;
  const uint8_t outB = ((fgB * alpha) + (bgB * (255 - alpha))) / 255;
  return (outR << 11) | (outG << 5) | outB;
}

uint16_t typeBadgeColor(const String& type) {
  if (type == "ノーマル") return 0xA4B0;
  if (type == "ほのお") return 0xFBE0;
  if (type == "みず") return 0x3D9F;
  if (type == "でんき") return 0xFEE0;
  if (type == "くさ") return 0x6E4D;
  if (type == "こおり") return 0x8F7D;
  if (type == "かくとう") return 0xB1E8;
  if (type == "どく") return 0xA1B3;
  if (type == "じめん") return 0xD5C9;
  if (type == "ひこう") return 0xA59F;
  if (type == "エスパー") return 0xFC9C;
  if (type == "むし") return 0xA445;
  if (type == "いわ") return 0xBC68;
  if (type == "ゴースト") return 0x6A95;
  if (type == "ドラゴン") return 0x6B5F;
  if (type == "あく") return 0x62EC;
  if (type == "はがね") return 0xA534;
  if (type == "フェアリー") return 0xFDB8;
  return COLOR_PK_TEXT;
}

struct EvolutionCardLayout {
  int x;
  int y;
  int w;
  int h;
  int imageX;
  int imageY;
  int imageW;
  int imageH;
  int idCenterY;
  int nameX;
  int nameY;
  int nameMaxW;
  int nameLineH;
};

EvolutionCardLayout getEvolutionCardLayout(int index, int totalCount) {
  if (totalCount <= 3) {
    return {
        kEvolutionCardX[index],
        kEvolutionCardY,
        84,
        80,
        kEvolutionCardX[index] + kEvolutionImageOffsetX,
        kEvolutionCardY + kEvolutionImageOffsetY,
        kEvolutionImageW,
        kEvolutionImageH,
        kEvolutionCardY + 40,
        kEvolutionCardX[index] + 8,
        kEvolutionCardY + 56,
        68,
        14,
    };
  }

  constexpr int cols = 4;
  constexpr int cardW = 62;
  constexpr int cardH = 62;
  constexpr int gapX = 8;
  constexpr int gapY = 8;
  constexpr int startX = 24;
  constexpr int startY = 66;
  const int col = index % cols;
  const int row = index / cols;
  const int x = startX + (col * (cardW + gapX));
  const int y = startY + (row * (cardH + gapY));
  return {
      x,
      y,
      cardW,
      cardH,
      x + 6,
      y + 8,
      50,
      32,
      y + 28,
      x + 4,
      y + 36,
      54,
      11,
  };
}

String truncateUtf8Label(const String& text, int maxChars) {
  if (maxChars <= 0) return "";

  String out = "";
  int chars = 0;
  for (size_t i = 0; i < text.length() && chars < maxChars; ++chars) {
    const uint8_t b0 = static_cast<uint8_t>(text[i]);
    size_t len = 1;
    if ((b0 & 0xE0) == 0xC0 && i + 1 < text.length()) {
      len = 2;
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < text.length()) {
      len = 3;
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < text.length()) {
      len = 4;
    }
    for (size_t j = 0; j < len; ++j) {
      out += text[i + j];
    }
    i += len;
  }

  if (out.length() < text.length()) {
    if (maxChars <= 3) return String("...");
    String trimmed = "";
    int kept = 0;
    for (size_t i = 0; i < out.length() && kept < maxChars - 3; ++kept) {
      const uint8_t b0 = static_cast<uint8_t>(out[i]);
      size_t len = 1;
      if ((b0 & 0xE0) == 0xC0 && i + 1 < out.length()) {
        len = 2;
      } else if ((b0 & 0xF0) == 0xE0 && i + 2 < out.length()) {
        len = 3;
      } else if ((b0 & 0xF8) == 0xF0 && i + 3 < out.length()) {
        len = 4;
      }
      for (size_t j = 0; j < len; ++j) {
        trimmed += out[i + j];
      }
      i += len;
    }
    return trimmed + "...";
  }

  return out;
}
}

UIController::UIController()
    : sprite(nullptr),
      wakeSplashSprite(nullptr),
      currentTheme(UI_THEME_CLASSIC),
      currentLanguage(APP_LANGUAGE_JA) {}

UIController::~UIController() {
  if (sprite != nullptr) {
    sprite->deleteSprite();
    delete sprite;
    sprite = nullptr;
  }
  if (wakeSplashSprite != nullptr) {
    wakeSplashSprite->deleteSprite();
    delete wakeSplashSprite;
    wakeSplashSprite = nullptr;
  }
}

bool UIController::begin() {
  sprite = new LGFX_Sprite(&M5.Display);
  if (sprite == nullptr) {
    return false;
  }
  wakeSplashSprite = new LGFX_Sprite(&M5.Display);
  if (wakeSplashSprite == nullptr) {
    return false;
  }
  if (!imageLoader.begin()) {
    return false;
  }
  if (!sprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
    return false;
  }
  wakeSplashSprite->setColorDepth(16);
  wakeSplashSprite->setPsram(true);
  if (!wakeSplashSprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
    return false;
  }
  wakeSplashSprite->fillScreen(TFT_BLACK);
  imageLoader.loadAndDisplayPNGPath(*wakeSplashSprite, kWakeSplashPath, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, false);
  return true;
}

void UIController::setTheme(UIThemeStyle theme) {
  if (theme < UI_THEME_CLASSIC || theme >= UI_THEME_COUNT) {
    currentTheme = UI_THEME_CLASSIC;
    return;
  }
  currentTheme = theme;
}

void UIController::setLanguage(AppLanguage language) {
  if (language < APP_LANGUAGE_JA || language >= APP_LANGUAGE_COUNT) {
    currentLanguage = APP_LANGUAGE_JA;
    return;
  }
  currentLanguage = language;
}

UIThemeStyle UIController::getTheme() const {
  return currentTheme;
}

AppLanguage UIController::getLanguage() const {
  return currentLanguage;
}

uint16_t UIController::getBackgroundColor() const {
  return getThemePalette(currentTheme).bg;
}

void UIController::drawBase() {
  sprite->fillScreen(getThemePalette(currentTheme).bg);
}

const lgfx::IFont* UIController::getFallbackFont(const lgfx::IFont* primaryFont) const {
  if (primaryFont == &fonts::efontJA_16_b) return &fonts::efontCN_16_b;
  if (primaryFont == &fonts::efontJA_16) return &fonts::efontCN_16;
  if (primaryFont == &fonts::efontJA_12_b) return &fonts::efontCN_12_b;
  if (primaryFont == &fonts::efontJA_12) return &fonts::efontCN_12;
  if (primaryFont == &fonts::efontJA_10_b) return &fonts::efontCN_10_b;
  return &fonts::efontCN_10;
}

const char* UIController::tr(const char* ja, const char* en) const {
  return currentLanguage == APP_LANGUAGE_EN ? en : ja;
}

const lgfx::IFont* UIController::selectFontForCodepoint(uint16_t codepoint, const lgfx::IFont* primaryFont) const {
  if (codepoint == 0x20 || codepoint == 0x3000) {
    return primaryFont;
  }

  lgfx::FontMetrics metrics;
  if (primaryFont != nullptr && primaryFont->updateFontMetric(&metrics, codepoint)) {
    return primaryFont;
  }

  const lgfx::IFont* fallbackFont = getFallbackFont(primaryFont);
  if (fallbackFont != nullptr && fallbackFont->updateFontMetric(&metrics, codepoint)) {
    return fallbackFont;
  }

  return primaryFont;
}

size_t UIController::readUtf8Glyph(const String& text, size_t index, uint16_t& codepoint, String& glyph) const {
  glyph = "";
  if (index >= text.length()) {
    codepoint = 0;
    return 0;
  }

  const uint8_t b0 = static_cast<uint8_t>(text[index]);
  size_t length = 1;
  codepoint = b0;

  if ((b0 & 0xE0) == 0xC0 && index + 1 < text.length()) {
    const uint8_t b1 = static_cast<uint8_t>(text[index + 1]);
    codepoint = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
    length = 2;
  } else if ((b0 & 0xF0) == 0xE0 && index + 2 < text.length()) {
    const uint8_t b1 = static_cast<uint8_t>(text[index + 1]);
    const uint8_t b2 = static_cast<uint8_t>(text[index + 2]);
    codepoint = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    length = 3;
  }

  for (size_t i = 0; i < length; ++i) {
    glyph += text[index + i];
  }
  return length;
}

void UIController::drawPressedOverlay(int x, int y, int w, int h, int radius) {
  const auto& theme = getThemePalette(currentTheme);
  const uint16_t overlay = blend565(theme.pressedOverlay, theme.surface, 72);
  if (radius > 0) {
    sprite->fillRoundRect(x, y, w, h, radius, overlay);
  } else {
    sprite->fillRect(x, y, w, h, overlay);
  }
}

void UIController::drawActionButton(
    int x,
    int y,
    int w,
    int h,
    const char* label,
    uint16_t fillColor,
    uint16_t textColor,
    bool pressed,
    uint16_t pressedFillColor,
    uint16_t borderColor) {
  const auto& theme = getThemePalette(currentTheme);
  const uint16_t bg = pressed ? pressedFillColor : fillColor;
  sprite->fillRoundRect(x, y, w, h, theme.buttonRadius, bg);
  sprite->drawRoundRect(x, y, w, h, theme.buttonRadius, borderColor);
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(textColor);
  sprite->drawCenterString(label, x + (w / 2), y + ((h - 12) / 2));
}

void UIController::drawHeader(const PokemonDetail& pk, bool searchPressed) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius, theme.border);
  if (searchPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius);
  }

  char idStr[12];
  snprintf(idStr, sizeof(idStr), "No.%04d", pk.id);
  const int baselineY = 22;
  const int idX = 22;

  sprite->setTextColor(theme.sub);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(idStr, idX, baselineY);

  const int nameX = idX + sprite->textWidth(idStr) + 12;

  sprite->setTextColor(theme.text);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(pk.name, nameX, baselineY);
}

void UIController::drawAppearanceTab(const PokemonDetail& pk, bool drawImage) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRect(kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH, theme.bg);
  if (drawImage) {
    imageLoader.loadAndDisplayPNG(*sprite, pk.id, kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH);
  }

  int cardX = 165;
  sprite->fillRoundRect(cardX, 60, 145, 135, 8, theme.surface);
  sprite->drawRoundRect(cardX, 60, 145, 135, 8, theme.border);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.text);
  sprite->drawString(pk.category, cardX + 10, 75);

  int ty = 95;
  for (const auto& t : pk.types) {
    drawTypeBadge(t, cardX + 10, ty);
    ty += 22;
  }

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.text);
  sprite->drawString(tr("高さ", "Height"), cardX + 10, 145);
  sprite->setTextColor(theme.sub);
  sprite->drawString(pk.height, cardX + 58, 145);
  sprite->setTextColor(theme.text);
  sprite->drawString(tr("重さ", "Weight"), cardX + 10, 167);
  sprite->setTextColor(theme.sub);
  sprite->drawString(pk.weight, cardX + 58, 167);
}

void UIController::drawAppearancePreviewFeedback() {
  drawPressedOverlay(kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH, 0);
}

void UIController::drawDescriptionTab(const PokemonDetail& pk) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.surface);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.border);
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.text);
  drawWrappedText(pk.description, 20, 72, 280, 22, 5);
}

void UIController::drawBodyTab(const PokemonDetail& pk) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.surface);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.border);

  drawInfoRow(tr("分類", "Category"), pk.category, 74);
  drawInfoRow(tr("高さ", "Height"), pk.height, 102);
  drawInfoRow(tr("重さ", "Weight"), pk.weight, 130);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.text);
  sprite->drawString(tr("タイプ", "Type"), 24, 160);

  int typeX = 92;
  for (const auto& type : pk.types) {
    drawTypeBadge(type, typeX, 154);
    typeX += 68;
  }
}

void UIController::drawAbilityTab(const PokemonDetail& pk) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.surface);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.border);

  int y = 72;
  for (size_t i = 0; i < pk.abilities.size() && i < 2; ++i) {
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(theme.text);
    sprite->drawString(pk.abilities[i].name, 20, y);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(theme.sub);
    drawWrappedText(pk.abilities[i].description, 20, y + 24, 280, 20, 2);
    y += 62;
  }
}

void UIController::drawEvolutionTab(const PokemonDetail& pk, int pressedEvolutionIndex, bool drawImages) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.surface);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, theme.border);

  const int totalCount = std::min<int>(pk.evolutions.size(), 8);
  for (int i = 0; i < totalCount; ++i) {
    const auto layout = getEvolutionCardLayout(i, totalCount);
    sprite->fillRoundRect(layout.x, layout.y, layout.w, layout.h, 8, theme.bg);
    sprite->drawRoundRect(layout.x, layout.y, layout.w, layout.h, 8, theme.border);

    sprite->fillRect(layout.imageX, layout.imageY, layout.imageW, layout.imageH, theme.bg);
    if (drawImages) {
      imageLoader.loadAndDisplayPNG(
          *sprite,
          pk.evolutions[i].id,
          layout.imageX,
          layout.imageY,
          layout.imageW,
          layout.imageH);
    }

    char idStr[12];
    snprintf(idStr, sizeof(idStr), "No.%04d", pk.evolutions[i].id);
    sprite->setTextColor(theme.sub);
    if (totalCount <= 3) {
      sprite->setFont(&fonts::efontJA_12);
      sprite->drawCenterString(idStr, layout.x + (layout.w / 2), layout.idCenterY);
      sprite->setTextColor(theme.text);
      drawWrappedText(pk.evolutions[i].name, layout.nameX, layout.nameY, layout.nameMaxW, layout.nameLineH, 2);
    } else {
      sprite->setFont(&fonts::efontJA_10);
      sprite->drawCenterString(idStr, layout.x + (layout.w / 2), layout.y + layout.h - 10);
    }

    if (static_cast<int>(i) == pressedEvolutionIndex) {
      drawPressedOverlay(layout.x, layout.y, layout.w, layout.h, 8);
    }
  }
}

void UIController::drawDetailNavigation(bool prevPressed, bool nextPressed) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->setTextColor(theme.sub);
  sprite->setFont(&fonts::efontJA_12);
  sprite->drawCenterString("◀", 18, 50);
  sprite->drawCenterString("▶", SCREEN_WIDTH - 18, 50);

  if (prevPressed) {
    drawPressedOverlay(0, 0, 40, TAB_BAR_Y);
  }
  if (nextPressed) {
    drawPressedOverlay(SCREEN_WIDTH - 40, 0, 40, TAB_BAR_Y);
  }
}

void UIController::drawFullscreenPreview(bool drawImage, uint16_t pokemonId) {
  sprite->fillScreen(TFT_BLACK);

  if (drawImage) {
    imageLoader.loadAndDisplayPNG(*sprite, pokemonId, -18, 0, SCREEN_WIDTH + 36, SCREEN_HEIGHT);
  }
}

void UIController::drawPreviewPocScreen(uint16_t pokemonId, int shiftX, int shiftY) {
  sprite->fillScreen(TFT_WHITE);

  char silhouettePath[64];
  char iconPath[64];
  snprintf(silhouettePath, sizeof(silhouettePath), "/pokemon/silhouettes/%04d.png", pokemonId);
  snprintf(iconPath, sizeof(iconPath), "/pokemon/icons/%04d.png", pokemonId);

  const int baseX = 64;
  const int baseY = 24;
  const int size = 192;
  const int shadowX = baseX + 4 - (shiftX / 3);
  const int shadowY = baseY + 6 - (shiftY / 3);
  const int iconX = baseX + shiftX;
  const int iconY = baseY + shiftY;

  imageLoader.loadAndDisplayPNGPath(*sprite, silhouettePath, shadowX, shadowY, size, size, false);
  imageLoader.loadAndDisplayPNGPath(*sprite, iconPath, iconX, iconY, size, size, false);
}

void UIController::drawPreviewPocScreenCached(LGFX_Sprite& silhouetteSprite, LGFX_Sprite& iconSprite, int shiftX, int shiftY, uint16_t transparentColor) {
  sprite->fillScreen(TFT_WHITE);

  const int baseX = 64;
  const int baseY = 24;
  const int shadowX = baseX + 3 - (shiftX / 2);
  const int shadowY = baseY + 5 - (shiftY / 2);
  const int iconX = baseX + shiftX;
  const int iconY = baseY + shiftY;

  silhouetteSprite.pushSprite(sprite, shadowX, shadowY, transparentColor);
  iconSprite.pushSprite(sprite, iconX, iconY, transparentColor);
}

void UIController::drawPreviewPocScreenLayered(
    LGFX_Sprite* backgroundSprite,
    LGFX_Sprite& silhouetteSprite,
    LGFX_Sprite& iconSprite,
    int shiftX,
    int shiftY,
    uint16_t transparentColor) {
  sprite->fillScreen(TFT_WHITE);

  const int bgOffsetX = shiftX / 4;
  const int bgOffsetY = shiftY / 5;

  if (backgroundSprite != nullptr) {
    const int bgX = -((backgroundSprite->width() - SCREEN_WIDTH) / 2) + bgOffsetX;
    const int bgY = -((backgroundSprite->height() - SCREEN_HEIGHT) / 2) + bgOffsetY;
    backgroundSprite->pushSprite(sprite, bgX, bgY);
  }

  const int baseX = 64;
  const int baseY = 24;
  const int shadowX = baseX + 3 - (shiftX / 2);
  const int shadowY = baseY + 5 - (shiftY / 2);
  const int iconX = baseX + shiftX;
  const int iconY = baseY + shiftY;

  silhouetteSprite.pushSprite(sprite, shadowX, shadowY, transparentColor);
  iconSprite.pushSprite(sprite, iconX, iconY, transparentColor);
}

void UIController::drawPreviewCaption(uint16_t pokemonId, const String& pokemonName) {
  char label[32];
  snprintf(label, sizeof(label), "No.%04d", pokemonId);
  const String caption = String(label) + " " + pokemonName;

  sprite->setFont(&fonts::efontJA_12_b);
  sprite->setTextColor(TFT_WHITE);
  sprite->drawRightString(caption, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 24);
}

void UIController::redrawPreviewCaptionToDisplay(uint16_t pokemonId, const String& pokemonName) {
  char label[32];
  snprintf(label, sizeof(label), "No.%04d", pokemonId);
  const String caption = String(label) + " " + pokemonName;

  M5.Display.setFont(&fonts::efontJA_12_b);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.drawRightString(caption, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 24);
}

void UIController::applyBlackFade(uint8_t alpha) {
  if (alpha == 0 || sprite == nullptr || sprite->getBuffer() == nullptr) {
    return;
  }
  uint16_t* buffer = static_cast<uint16_t*>(sprite->getBuffer());
  const size_t pixelCount = static_cast<size_t>(SCREEN_WIDTH) * static_cast<size_t>(SCREEN_HEIGHT);
  for (size_t i = 0; i < pixelCount; ++i) {
    buffer[i] = blend565(0x0000, buffer[i], alpha);
  }
}

void UIController::drawWakeSplashScreen(uint8_t progressPercent, const char* statusText) {
  sprite->pushImage(
      0,
      0,
      SCREEN_WIDTH,
      SCREEN_HEIGHT,
      static_cast<const uint16_t*>(wakeSplashSprite->getBuffer()));

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(kWakeSplashAccentColor);
  sprite->drawString(statusText != nullptr ? statusText : "", 18, 18);

  const int barX = 22;
  const int barY = SCREEN_HEIGHT - 30;
  const int barW = SCREEN_WIDTH - 44;
  const int barH = 10;
  sprite->fillRoundRect(barX, barY, barW, barH, 5, blend565(TFT_BLACK, COLOR_PK_CARD, 120));

  const int fillW = ((barW - 4) * constrain(progressPercent, 0, 100)) / 100;
  if (fillW > 0) {
    sprite->fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 4, kWakeSplashAccentColor);
  }
}

void UIController::drawLockOnScreen(
    int phase,
    int rarity,
    const char* rarityLabel,
    int markerCenterX,
    int zoneCenterX,
    int zoneWidth,
    int jitterOffset,
    uint8_t progressPercent,
    uint16_t pokemonId,
    const String& pokemonName,
    bool actionPressed,
    bool showDetailHint) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);

  const uint16_t frameColor = blend565(theme.highlight, theme.surface, 180);
  for (int y = 0; y < SCREEN_HEIGHT; y += 16) {
    sprite->drawFastHLine(0, y, SCREEN_WIDTH, blend565(theme.border, theme.bg, 120));
  }
  for (int x = 0; x < SCREEN_WIDTH; x += 16) {
    sprite->drawFastVLine(x, 0, SCREEN_HEIGHT, blend565(theme.border, theme.bg, 80));
  }

  sprite->fillRoundRect(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 24, theme.buttonRadius, blend565(theme.surface, theme.bg, 180));
  sprite->drawRoundRect(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 24, theme.buttonRadius, frameColor);

  const int reticleCx = (SCREEN_WIDTH / 2) + jitterOffset;
  const int reticleCy = 86 - jitterOffset;
  sprite->drawCircle(reticleCx, reticleCy, 28, theme.highlight);
  sprite->drawCircle(reticleCx, reticleCy, 18, frameColor);
  sprite->drawFastHLine(reticleCx - 40, reticleCy, 80, theme.highlight);
  sprite->drawFastVLine(reticleCx, reticleCy - 40, 80, theme.highlight);

  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.text);
  if (phase == 1) {
    sprite->drawCenterString("LOCK ON READY", SCREEN_WIDTH / 2, 24);
  } else if (phase == 2) {
    sprite->drawCenterString("LOCK ON SEARCH", SCREEN_WIDTH / 2, 24);
  } else if (phase == 3) {
    sprite->drawCenterString("LOCKED!", SCREEN_WIDTH / 2, 24);
  } else if (phase == 4) {
    sprite->drawCenterString("GATCHA!", SCREEN_WIDTH / 2, 24);
  } else {
    sprite->drawCenterString("LOCK FAILED", SCREEN_WIDTH / 2, 24);
  }

  if (phase == 4) {
    drawQuizPokemonImage(pokemonId, 86, 44, 148, 148);
    if (rarityLabel != nullptr && rarityLabel[0] != '\0') {
      const bool isLegend = rarity >= 2;
      const bool isRare = rarity == 1;
      const int badgeW = isLegend ? 124 : (isRare ? 108 : 92);
      const int badgeH = isLegend ? 24 : (isRare ? 20 : 18);
      const int badgeX = (SCREEN_WIDTH - badgeW) / 2;
      const int badgeY = 152;
      const uint16_t rarityFill = isLegend
          ? lgfx::v1::color565(255, 170, 40)
          : (isRare ? lgfx::v1::color565(90, 230, 255) : blend565(theme.accent, theme.surface, 150));
      const uint16_t rarityBorder = isLegend
          ? lgfx::v1::color565(255, 235, 150)
          : (isRare ? lgfx::v1::color565(180, 250, 255) : blend565(theme.highlight, theme.border, 150));
      const uint16_t rarityText = isLegend
          ? lgfx::v1::color565(40, 16, 0)
          : (isRare ? lgfx::v1::color565(0, 46, 58) : theme.text);
      sprite->fillRoundRect(badgeX, badgeY, badgeW, badgeH, badgeH / 2, rarityFill);
      sprite->drawRoundRect(badgeX, badgeY, badgeW, badgeH, badgeH / 2, rarityBorder);
      sprite->setFont(isLegend ? &fonts::efontJA_16_b : (isRare ? &fonts::efontJA_12_b : &fonts::efontJA_10));
      sprite->setTextColor(rarityText);
      sprite->drawCenterString(rarityLabel, SCREEN_WIDTH / 2, badgeY + (isLegend ? 4 : 5));
    }
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(theme.sub);
    char idLabel[12];
    snprintf(idLabel, sizeof(idLabel), "No.%04d", pokemonId);
    sprite->drawCenterString(idLabel, SCREEN_WIDTH / 2, 176);
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(theme.text);
    sprite->drawCenterString(pokemonName, SCREEN_WIDTH / 2, 194);
    if (showDetailHint) {
      sprite->setFont(&fonts::efontJA_12);
      sprite->setTextColor(theme.sub);
      sprite->drawCenterString(tr("タップ / Port.B で ずかんをひらく", "Tap / Port.B: Pokedex"), SCREEN_WIDTH / 2, 208);
    }
  } else if (phase == 5) {
    const uint16_t failColor = lgfx::v1::color565(255, 90, 90);
    const uint16_t failSoft = blend565(failColor, theme.surface, 140);
    const int failCx = SCREEN_WIDTH / 2;
    const int failCy = 102;
    sprite->drawCircle(failCx, failCy, 34, failSoft);
    sprite->drawCircle(failCx, failCy, 35, failColor);
    sprite->drawLine(failCx - 16, failCy - 16, failCx + 16, failCy + 16, failColor);
    sprite->drawLine(failCx - 16, failCy + 16, failCx + 16, failCy - 16, failColor);
    sprite->drawLine(failCx - 16, failCy - 15, failCx + 16, failCy + 17, failColor);
    sprite->drawLine(failCx - 16, failCy + 15, failCx + 16, failCy - 17, failColor);
    sprite->drawFastHLine(failCx - 46, failCy, 18, failSoft);
    sprite->drawFastHLine(failCx + 28, failCy, 18, failSoft);
    sprite->drawFastVLine(failCx, failCy - 46, 18, failSoft);
    sprite->drawFastVLine(failCx, failCy + 28, 18, failSoft);

    sprite->fillRoundRect(60, 162, SCREEN_WIDTH - 120, 38, 10, blend565(theme.surface, theme.bg, 200));
    sprite->drawRoundRect(60, 162, SCREEN_WIDTH - 120, 38, 10, failSoft);
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(failColor);
    sprite->drawCenterString(tr("にげられた!", "Got Away!"), SCREEN_WIDTH / 2, 172);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(theme.sub);
    sprite->drawCenterString(tr("タップ / Port.B で もどる", "Tap / Port.B: Back"), SCREEN_WIDTH / 2, 210);
  } else {
    constexpr int barX = 36;
    constexpr int barY = 176;
    constexpr int barW = SCREEN_WIDTH - 72;
    constexpr int barH = 20;
    const int zoneX = zoneCenterX - (zoneWidth / 2);
    const bool isLegend = rarity >= 2;
    const bool isRare = rarity == 1;
    const uint16_t zoneFill = isLegend
        ? lgfx::v1::color565(255, 190, 70)
        : (isRare ? lgfx::v1::color565(80, 230, 255) : blend565(theme.highlight, theme.surface, 180));
    const uint16_t zoneBorder = isLegend
        ? lgfx::v1::color565(255, 235, 150)
        : (isRare ? lgfx::v1::color565(180, 250, 255) : theme.highlight);
    sprite->fillRoundRect(barX, barY, barW, barH, 8, theme.surface);
    sprite->drawRoundRect(barX, barY, barW, barH, 8, theme.border);
    sprite->fillRoundRect(zoneX, barY + 2, zoneWidth, barH - 4, 6, zoneFill);
    sprite->drawRoundRect(zoneX, barY + 2, zoneWidth, barH - 4, 6, zoneBorder);
    sprite->drawFastVLine(markerCenterX, barY - 8, barH + 16, theme.accent);
    sprite->fillCircle(markerCenterX, barY + (barH / 2), 4, theme.accent);

    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(theme.sub);
    if (phase == 1) {
      sprite->drawCenterString(tr("Port.B でもういちど おして ロックする", "Press Port.B again"), SCREEN_WIDTH / 2, 204);
    } else if (phase == 3) {
      sprite->drawCenterString(tr("そのばで かくてい!", "Locked in!"), SCREEN_WIDTH / 2, 204);
    } else {
      sprite->drawCenterString(tr("ハイライト ゾーンで おす", "Press in the zone"), SCREEN_WIDTH / 2, 204);
    }
    char progressLabel[16];
    snprintf(progressLabel, sizeof(progressLabel), "%d%%", progressPercent);
    sprite->drawRightString(progressLabel, SCREEN_WIDTH - 24, 154);
  }

  if (actionPressed) {
    drawPressedOverlay(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 24, theme.buttonRadius);
  }
}

void UIController::drawMenuScreen(
    bool pokedexPressed,
    bool quizPressed,
    bool slideshowPressed,
    bool guidePressed,
    bool settingsPressed,
    const char* volumeLabel,
    int batteryLevel,
    bool batteryCharging) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);

  constexpr int statusBarX = 0;
  constexpr int statusBarY = 0;
  constexpr int statusBarW = SCREEN_WIDTH;
  constexpr int statusBarH = 22;
  sprite->fillRect(statusBarX, statusBarY, statusBarW, statusBarH, theme.statusBarBg);
  sprite->drawFastHLine(statusBarX, statusBarH - 1, statusBarW, theme.statusBarDivider);

  int statusRightX = statusBarX + statusBarW - 8;
  if (batteryLevel >= 0) {
    char batteryLabel[8];
    snprintf(batteryLabel, sizeof(batteryLabel), "%d%%", constrain(batteryLevel, 0, 100));
    sprite->setFont(&fonts::efontJA_10);
    sprite->setTextColor(theme.text);
    const int batteryTextW = sprite->textWidth(batteryLabel);
    statusRightX -= batteryTextW;
    sprite->drawString(batteryLabel, statusRightX, statusBarY + 6);
    statusRightX -= 24;
    drawBatteryIcon(sprite, theme, statusRightX, statusBarY + 6, batteryLevel, batteryCharging);
    statusRightX -= 8;
  }

  if (volumeLabel != nullptr && volumeLabel[0] != '\0') {
    char volumeText[16];
    snprintf(volumeText, sizeof(volumeText), "VOL %s", volumeLabel);
    sprite->setFont(&fonts::efontJA_10);
    sprite->setTextColor(theme.text);
    const int volumeTextW = sprite->textWidth(volumeText);
    statusRightX -= volumeTextW;
    sprite->drawString(volumeText, statusRightX, statusBarY + 6);
  }

  constexpr int menuLeftX = 24;
  constexpr int menuRightX = 166;
  constexpr int menuButtonW = 130;
  constexpr int menuButtonH = 34;
  constexpr int menuRowY0 = 42;
  constexpr int menuRowGap = 42;

  drawActionButton(menuLeftX, menuRowY0, menuButtonW, menuButtonH, tr("ポケモンずかん", "Pokedex"), theme.accent, theme.invertedText, pokedexPressed, theme.buttonPressedFill, theme.accent);
  drawActionButton(menuLeftX, menuRowY0 + menuRowGap, menuButtonW, menuButtonH, tr("ポケモンクイズ", "Pokemon Quiz"), theme.surface, theme.text, quizPressed, theme.buttonPressedFill, theme.border);
  drawActionButton(menuLeftX, menuRowY0 + (menuRowGap * 2), menuButtonW, menuButtonH, tr("スライドショー", "Slide Show"), theme.surface, theme.text, slideshowPressed, theme.buttonPressedFill, theme.border);
  drawActionButton(menuRightX, menuRowY0, menuButtonW, menuButtonH, tr("こうりゃく", "Guide"), theme.surface, theme.text, guidePressed, theme.buttonPressedFill, theme.border);
  drawActionButton(menuRightX, menuRowY0 + menuRowGap, menuButtonW, menuButtonH, tr("せってい", "Settings"), theme.surface, theme.text, settingsPressed, theme.buttonPressedFill, theme.border);
}

void UIController::drawSettingsScreen(
    bool backPressed,
    int settingsTabIndex,
    int pressedSettingsTabIndex,
    bool preview3dEnabled,
    bool preview3dPressed,
    bool previewCaptionEnabled,
    bool previewCaptionPressed,
    int selectedThemeIndex,
    int pressedThemeIndex,
    int selectedLanguageIndex,
    int pressedLanguageIndex,
    int selectedVolumeIndex,
    int pressedVolumeIndex) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius, theme.border);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, theme.buttonRadius);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.text);
  sprite->drawCenterString(tr("せってい", "Settings"), SCREEN_WIDTH / 2, 22);

  sprite->setFont(&fonts::efontJA_12);
  if (settingsTabIndex == 0) {
    sprite->setTextColor(theme.sub);
    sprite->drawString(tr("プレビュー", "Preview"), 24, 72);
    drawActionButton(
        202,
        60,
        94,
        30,
        preview3dEnabled ? "3D ON" : "3D OFF",
        preview3dEnabled ? theme.accent : theme.surface,
        preview3dEnabled ? theme.invertedText : theme.text,
        preview3dPressed,
        theme.buttonPressedFill,
        preview3dEnabled ? theme.accent : theme.border);
    drawActionButton(
        202,
        98,
        94,
        30,
        previewCaptionEnabled ? tr("文字 ON", "Text ON") : tr("文字 OFF", "Text OFF"),
        previewCaptionEnabled ? theme.accent : theme.surface,
        previewCaptionEnabled ? theme.invertedText : theme.text,
        previewCaptionPressed,
        theme.buttonPressedFill,
        previewCaptionEnabled ? theme.accent : theme.border);

    sprite->setTextColor(theme.sub);
    sprite->drawString(tr("テーマ", "Theme"), 24, 140);
    const char* themeLabels[2] = {tr("クラシック", "Classic"), tr("サイバー", "Cyber")};
    for (int i = 0; i < 2; ++i) {
      const int x = 24 + (i * 140);
      const bool selected = selectedThemeIndex == i;
      const bool pressed = pressedThemeIndex == i;
      drawActionButton(
          x,
          152,
          128,
          30,
          themeLabels[i],
          selected ? theme.highlight : theme.surface,
          selected ? theme.invertedText : theme.text,
          pressed,
          theme.buttonPressedFill,
          selected ? theme.highlight : theme.border);
    }
  } else {
    sprite->setTextColor(theme.sub);
    sprite->drawString(tr("ことば", "Language"), 24, 72);
    const char* languageLabels[2] = {tr("日本語", "Japanese"), "English"};
    for (int i = 0; i < 2; ++i) {
      const int x = 24 + (i * 140);
      const bool selected = selectedLanguageIndex == i;
      const bool pressed = pressedLanguageIndex == i;
      drawActionButton(
          x,
          84,
          128,
          30,
          languageLabels[i],
          selected ? theme.highlight : theme.surface,
          selected ? theme.invertedText : theme.text,
          pressed,
          theme.buttonPressedFill,
          selected ? theme.highlight : theme.border);
    }

    sprite->setTextColor(theme.sub);
    sprite->drawString(tr("おんりょう", "Volume"), 24, 140);
    const char* volumeLabels[4] = {
        tr("大", "High"),
        tr("中", "Med"),
        tr("小", "Low"),
        tr("なし", "Mute"),
    };
    for (int i = 0; i < 4; ++i) {
      const int x = 24 + (i * 70);
      const bool selected = selectedVolumeIndex == i;
      const bool pressed = pressedVolumeIndex == i;
      const uint16_t fill = selected ? theme.highlight : theme.surface;
      const uint16_t text = selected ? theme.invertedText : theme.text;
      const uint16_t border = selected ? theme.highlight : theme.border;
      drawActionButton(
          x,
          152,
          58,
          28,
          volumeLabels[i],
          fill,
          text,
          pressed,
          theme.buttonPressedFill,
          border);
    }
  }

  const char* settingsTabLabels[2] = {
      tr("ひょうじ", "Display"),
      tr("ことば/おと", "Lang/Sound"),
  };
  const int tabW = SCREEN_WIDTH / 2;
  for (int i = 0; i < 2; ++i) {
    const int x = i * tabW;
    const bool isActive = settingsTabIndex == i;
    const bool isPressed = pressedSettingsTabIndex == i;
    uint16_t bg = isActive ? theme.accent : theme.surface;
    uint16_t fg = isActive ? theme.invertedText : theme.text;
    if (isPressed) {
      bg = theme.buttonPressedFill;
      fg = theme.invertedText;
    }
    sprite->fillRect(x, TAB_BAR_Y, tabW, TAB_BAR_H, bg);
    sprite->drawRect(x, TAB_BAR_Y, tabW, TAB_BAR_H, theme.border);
    sprite->setTextColor(fg);
    sprite->setFont(&fonts::efontJA_12);
    sprite->drawCenterString(settingsTabLabels[i], x + (tabW / 2), TAB_BAR_Y + 12);
  }
}

void UIController::drawGuideMenuScreen(bool pokemonPressed, bool locationPressed, bool backPressed) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.border);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.text);
  sprite->drawCenterString(tr("こうりゃく", "Guide"), SCREEN_WIDTH / 2, 22);

  drawActionButton(32, 72, SCREEN_WIDTH - 64, 42, tr("ポケモンからみる", "By Pokemon"), theme.accent, theme.invertedText, pokemonPressed, theme.text, theme.accent);
  drawActionButton(32, 126, SCREEN_WIDTH - 64, 42, tr("場所からみる", "By Area"), theme.surface, theme.text, locationPressed, theme.border, theme.border);
}

void UIController::drawGuidePokemonListScreen(
    const char* title,
    const std::vector<String>& labels,
    const std::vector<bool>& caughtFlags,
    bool backPressed,
    int pressedItemIndex,
    bool prevPressed,
    bool nextPressed) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.border);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.text);
  sprite->drawCenterString(title, SCREEN_WIDTH / 2, 22);
  drawDetailNavigation(prevPressed, nextPressed);

  sprite->setFont(&fonts::efontJA_12);
  constexpr int itemX[2] = {44, 162};
  constexpr int itemYStart = 48;
  constexpr int itemW = 114;
  constexpr int itemH = 24;
  constexpr int itemGapY = 6;
  for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
    const int col = i % 2;
    const int row = i / 2;
    const int x = itemX[col];
    const int y = itemYStart + (row * (itemH + itemGapY));
    drawActionButton(
        x,
        y,
        itemW,
        itemH,
        labels[i].c_str(),
        theme.bg,
        theme.text,
        pressedItemIndex == i,
        theme.accent,
        theme.border);
    if (i < static_cast<int>(caughtFlags.size()) && caughtFlags[i]) {
      const int cx = x + itemW - 8;
      const int cy = y + (itemH / 2);
      sprite->drawLine(cx - 4, cy, cx - 1, cy + 3, theme.accent);
      sprite->drawLine(cx - 1, cy + 3, cx + 6, cy - 4, theme.accent);
      sprite->drawLine(cx - 4, cy + 1, cx - 1, cy + 4, theme.accent);
      sprite->drawLine(cx - 1, cy + 4, cx + 6, cy - 3, theme.accent);
    }
  }

}

void UIController::drawGuideLocationListScreen(const std::vector<String>& labels, bool backPressed, int pressedItemIndex, bool prevPressed, bool nextPressed) {
  std::vector<bool> emptyFlags(labels.size(), false);
  std::vector<String> displayLabels;
  displayLabels.reserve(labels.size());
  for (const auto& label : labels) {
    displayLabels.push_back(truncateUtf8Label(label, 9));
  }
  drawGuidePokemonListScreen(tr("場所からみる", "By Area"), displayLabels, emptyFlags, backPressed, pressedItemIndex, prevPressed, nextPressed);
}

void UIController::drawGuidePokemonDetailScreen(
    uint16_t pokemonId,
    const String& headerLabel,
    const std::vector<String>& lines,
    int activeTab,
    int pageIndex,
    int pageCount,
    bool caughtEnabled,
    bool caughtPressed,
    bool backPressed,
    int pressedTab,
    bool pagePressed,
    bool prevPressed,
    bool nextPressed) {
  const auto& theme = getThemePalette(currentTheme);
  const char* kTabs[5] = {
      tr("しんか", "Evo"),
      tr("しゅつげん", "Area"),
      tr("わざ", "Moves"),
      tr("マシン", "TM"),
      tr("ひでん", "HM"),
  };

  sprite->fillScreen(theme.bg);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.border);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.sub);
  sprite->drawString("No.", 18, 16);
  char idText[8];
  snprintf(idText, sizeof(idText), "%04d", pokemonId);
  sprite->setTextColor(theme.accent);
  sprite->drawString(idText, 42, 16);
  sprite->setTextColor(theme.text);
  sprite->drawString(headerLabel, 92, 16);
  drawDetailNavigation(prevPressed, nextPressed);

  const bool showIcon = activeTab == 0;
  int textX = 20;
  int textY = 62;
  int textMaxLines = 9;
  if (showIcon) {
    sprite->fillRoundRect(12, 56, 92, 92, 10, theme.surface);
    sprite->drawRoundRect(12, 56, 92, 92, 10, theme.border);
    drawQuizPokemonImage(pokemonId, 16, 60, 84, 84);
    drawActionButton(
        16,
        152,
        84,
        22,
        tr("つかまえた", "Caught"),
        caughtEnabled ? theme.accent : theme.bg,
        caughtEnabled ? theme.invertedText : theme.text,
        caughtPressed,
        caughtEnabled ? theme.text : theme.border,
        caughtEnabled ? theme.accent : theme.border);
    sprite->fillRoundRect(112, 56, SCREEN_WIDTH - 124, 122, 10, theme.surface);
    sprite->drawRoundRect(112, 56, SCREEN_WIDTH - 124, 122, 10, theme.border);
    if (pagePressed && pageCount > 1) {
      drawPressedOverlay(112, 56, SCREEN_WIDTH - 124, 122, 10);
    }
    textX = 120;
    textY = 64;
    textMaxLines = 8;
  } else {
    sprite->fillRoundRect(12, 56, SCREEN_WIDTH - 24, 122, 10, theme.surface);
    sprite->drawRoundRect(12, 56, SCREEN_WIDTH - 24, 122, 10, theme.border);
    if (pagePressed && pageCount > 1) {
      drawPressedOverlay(12, 56, SCREEN_WIDTH - 24, 122, 10);
    }
    textX = 20;
    textY = 64;
    textMaxLines = 8;
  }

  sprite->setFont(&fonts::efontJA_12);
  if (lines.empty()) {
    sprite->setTextColor(theme.sub);
    sprite->drawCenterString(tr("データなし", "No Data"), SCREEN_WIDTH / 2, 108);
  } else {
    const int lineH = 14;
    const int startIndex = pageIndex * textMaxLines;
    for (int i = 0; i < textMaxLines; ++i) {
      const int lineIndex = startIndex + i;
      if (lineIndex >= static_cast<int>(lines.size())) break;
      sprite->setTextColor(theme.text);
      sprite->drawString(lines[lineIndex], textX, textY + (i * lineH));
    }
  }

  if (pageCount > 1) {
    char pageText[12];
    snprintf(pageText, sizeof(pageText), "%d/%d", pageIndex + 1, pageCount);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(theme.sub);
    if (showIcon) {
      sprite->drawRightString(pageText, SCREEN_WIDTH - 18, 158);
    } else {
      sprite->drawRightString(pageText, SCREEN_WIDTH - 18, 158);
    }
  }

  const int tabW = SCREEN_WIDTH / 5;
  const int tabH = TAB_BAR_H;
  const int startX = 0;
  const int y = TAB_BAR_Y;
  for (int i = 0; i < 5; ++i) {
    const int x = startX + (i * tabW);
    const bool isActive = activeTab == i;
    uint16_t bg = isActive ? theme.accent : theme.surface;
    uint16_t fg = isActive ? theme.invertedText : theme.text;
    if (pressedTab == i) {
      bg = theme.buttonPressedFill;
      fg = theme.invertedText;
    }
    sprite->fillRect(x, y, tabW, tabH, bg);
    sprite->drawRect(x, y, tabW, tabH, theme.border);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(fg);
    sprite->drawCenterString(kTabs[i], x + (tabW / 2), y + 11);
  }
}

void UIController::drawQuizScreen(bool answerSide, uint16_t pokemonId, const String& answerName) {
  sprite->fillScreen(TFT_BLACK);
  if (SD.exists(kQuizBackgroundPath)) {
    sprite->drawPngFile(SD, kQuizBackgroundPath, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  }

  if (answerSide) {
    drawQuizPokemonImage(pokemonId, 15, 15, 133, 128);
  } else {
    drawQuizSilhouetteImage(pokemonId, 15, 15, 133, 128);
  }

  sprite->setTextColor(COLOR_PK_BAR);
  sprite->setFont(&fonts::efontJA_16_b);
  const int textBoxX = 175;
  const int textBoxY = 30;
  const int textBoxW = 125;
  const String displayName = answerSide ? answerName : "？？？？";
  sprite->drawCenterString(displayName, textBoxX + (textBoxW / 2), textBoxY + 14);
}

void UIController::drawQuizSilhouetteImage(uint16_t pokemonId, int x, int y, int w, int h) {
  char path[64];
  snprintf(path, sizeof(path), "/pokemon/silhouettes/%04d.png", pokemonId);
  imageLoader.loadAndDisplayPNGPath(*sprite, path, x, y, w, h, false);
}

void UIController::drawQuizPokemonImage(uint16_t pokemonId, int x, int y, int w, int h) {
  imageLoader.loadAndDisplayPNG(*sprite, pokemonId, x, y, w, h, false);
}

void UIController::blitAppearanceImageToCanvas(LGFX_Sprite& imageSprite) {
  sprite->pushImage(
      kAppearanceImageX,
      kAppearanceImageY,
      kAppearanceImageW,
      kAppearanceImageH,
      static_cast<const uint16_t*>(imageSprite.getBuffer()));
}

void UIController::pushAppearanceImageToDisplay(LGFX_Sprite& imageSprite) {
  imageSprite.pushSprite(&M5.Display, kAppearanceImageX, kAppearanceImageY);
}

void UIController::redrawDetailNavigationToDisplay() {
  const auto& theme = getThemePalette(currentTheme);
  M5.Display.setTextColor(theme.sub);
  M5.Display.setFont(&fonts::efontJA_12);
  M5.Display.drawCenterString("◀", 18, 50);
  M5.Display.drawCenterString("▶", SCREEN_WIDTH - 18, 50);
}

void UIController::blitPreviewImageToCanvas(LGFX_Sprite& imageSprite) {
  sprite->pushImage(
      0,
      0,
      SCREEN_WIDTH,
      SCREEN_HEIGHT,
      static_cast<const uint16_t*>(imageSprite.getBuffer()));
}

void UIController::pushPreviewImageToDisplay(LGFX_Sprite& imageSprite) {
  imageSprite.pushSprite(&M5.Display, 0, 0);
}

void UIController::blitEvolutionImageToCanvas(LGFX_Sprite& imageSprite, int imageIndex, int totalCount) {
  if (imageIndex < 0 || imageIndex >= totalCount) {
    return;
  }
  const auto layout = getEvolutionCardLayout(imageIndex, totalCount);
  sprite->pushImage(
      layout.imageX,
      layout.imageY,
      layout.imageW,
      layout.imageH,
      static_cast<const uint16_t*>(imageSprite.getBuffer()));
}

void UIController::pushEvolutionImageToDisplay(LGFX_Sprite& imageSprite, int imageIndex, int totalCount) {
  if (imageIndex < 0 || imageIndex >= totalCount) {
    return;
  }
  const auto layout = getEvolutionCardLayout(imageIndex, totalCount);
  imageSprite.pushSprite(
      &M5.Display,
      layout.imageX,
      layout.imageY);
}

void UIController::drawSearchScreen(
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
    bool flashActive,
    bool cancelPressed,
    bool openPressed) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);
  sprite->fillRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, flashActive ? theme.statusBarBg : theme.surface);
  sprite->drawRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, theme.border);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.surface);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, theme.border);
  if (menuPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(theme.text);
  sprite->drawCenterString(nameMode ? tr("ポケモンずかん", "Pokedex") : tr("No.でえらぶ", "Pick No."), SCREEN_WIDTH / 2, 22);

  if (!nameMode) {
    const int digitX[4] = {60, 110, 160, 210};
    const int digitStep[4] = {1000, 100, 10, 1};

    char idText[12];
    snprintf(idText, sizeof(idText), "%04d", selectedId);
    const bool validId = selectedId >= MIN_POKEMON_ID && selectedName.length() > 0;

    for (int i = 0; i < 4; ++i) {
      const bool upPressed = pressedDigitDelta == digitStep[i];
      const bool downPressed = pressedDigitDelta == -digitStep[i];

      drawActionButton(digitX[i], 52, 40, 34, "", theme.bg, theme.text, upPressed, theme.accent, theme.border);
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(upPressed ? theme.invertedText : theme.text);
      sprite->drawCenterString("▲", digitX[i] + 20, 61);

      char digitText[2] = {idText[i], '\0'};
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(theme.accent);
      sprite->drawCenterString(digitText, digitX[i] + 20, 92);

      drawActionButton(digitX[i], 120, 40, 34, "", theme.bg, theme.text, downPressed, theme.accent, theme.border);
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(downPressed ? theme.invertedText : theme.text);
      sprite->drawCenterString("▼", digitX[i] + 20, 129);
    }

    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(validId ? theme.text : theme.sub);
    sprite->drawCenterString(validId ? selectedName : tr("データなし", "No Data"), 160, 156);

    drawActionButton(20, 178, 280, 40, tr("ひらく", "Open"), theme.accent, theme.invertedText, openPressed, theme.text, theme.accent);
    return;
  }

  const bool hasKeyword = nameQuery.length() > 0;
  const String keywordLabel = hasKeyword ? nameQuery : String(tr("キーワード", "Keyword"));
  const uint16_t keywordFill = hasKeyword ? theme.accent : theme.bg;
  const uint16_t keywordText = hasKeyword ? theme.invertedText : theme.text;
  drawActionButton(44, 52, 114, 24, keywordLabel.c_str(), keywordFill, keywordText, cancelPressed, theme.border, theme.border);
  drawActionButton(162, 52, 114, 24, tr("ばんごう", "Number"), theme.bg, theme.text, modePressed, theme.border, theme.border);

  drawDetailNavigation(pagePrevPressed, pageNextPressed);

  const int itemX[2] = {44, 162};
  const int itemW = 114;
  const int itemH = 26;
  const int startY = 82;
  for (int i = 0; i < 10; ++i) {
    const int col = i % 2;
    const int row = i / 2;
    const int x = itemX[col];
    const int y = startY + (row * 28);
    const bool pressed = pressedCandidateIndex == i;

    uint16_t fill = pressed ? theme.buttonPressedFill : theme.bg;
    uint16_t textColor = pressed ? theme.invertedText : theme.text;
    sprite->fillRoundRect(x, y, itemW, itemH, 6, fill);
    sprite->drawRoundRect(x, y, itemW, itemH, 6, theme.border);

    if (i < static_cast<int>(nameCandidateIds.size())) {
      const String candidateText = nameCandidateLabels[i];
      sprite->setFont(&fonts::efontJA_12);
      sprite->setTextColor(textColor);
      sprite->drawString(candidateText, x + 6, y + 6);
    }
  }
}

void UIController::drawSearchInputScreen(
    const String& nameQuery,
    bool vowelMode,
    const String& selectedRowLabel,
    bool backPressed,
    bool clearPressed,
    bool deletePressed,
    int pressedKeyIndex) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->fillScreen(theme.bg);
  sprite->fillRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, theme.surface);
  sprite->drawRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, theme.border);

  drawActionButton(12, 202, 88, 28, tr("もどる", "Back"), theme.bg, theme.text, backPressed, theme.border, theme.border);
  drawActionButton(116, 202, 88, 28, tr("けす", "Del"), theme.bg, theme.text, deletePressed, theme.border, theme.border);
  drawActionButton(220, 202, 88, 28, tr("クリア", "Clear"), theme.bg, theme.text, clearPressed, theme.border, theme.border);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.sub);
  sprite->drawString(tr("おとを えらぶ", "Name Input"), 12, 16);

  sprite->fillRoundRect(12, 32, 296, 24, 6, theme.bg);
  sprite->drawRoundRect(12, 32, 296, 24, 6, theme.border);
  sprite->setTextColor(nameQuery.length() > 0 ? theme.text : theme.sub);
  sprite->drawString(nameQuery.length() > 0 ? nameQuery : tr("ここに なまえが はいります", "Name appears here"), 20, 38);
  sprite->fillRoundRect(12, 56, 120, 16, 6, vowelMode ? theme.accent : theme.bg);
  sprite->drawRoundRect(12, 56, 120, 16, 6, theme.border);
  sprite->setTextColor(vowelMode ? theme.invertedText : theme.sub);
  sprite->drawCenterString(vowelMode ? tr("もじを えらぶ", "Pick Char") : tr("ぎょうを えらぶ", "Pick Set"), 72, 60);

  const bool englishMode = currentLanguage == APP_LANGUAGE_EN;

  if (!vowelMode) {
    if (englishMode) {
      static constexpr const char* englishRowLabels[12] = {"ABC", "DEF", "GHI", "JKL", "MNO", "PQR", "STU", "VWX", "YZ", "SYM", "", ""};
      const int keyX[3] = {30, 120, 210};
      const int keyY[4] = {76, 106, 136, 166};
      for (int i = 0; i < 12; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const bool disabled = englishRowLabels[i][0] == '\0';
        drawActionButton(
            keyX[col],
            keyY[row],
            80,
            24,
            englishRowLabels[i],
            theme.bg,
            disabled ? theme.sub : theme.text,
            pressedKeyIndex == i,
            theme.accent,
            theme.border);
      }
      return;
    }

    static constexpr const char* rowLabels[12] = {"ア", "カ", "サ", "タ", "ナ", "ハ", "マ", "ヤ", "ラ", "ワ", "゛゜", "小"};
    const int keyX[3] = {30, 120, 210};
    const int keyY[4] = {76, 106, 136, 166};
    for (int i = 0; i < 12; ++i) {
      const int col = i % 3;
      const int row = i / 3;
      drawActionButton(
          keyX[col],
          keyY[row],
          80,
          24,
          rowLabels[i],
          theme.bg,
          (i >= 10) ? theme.sub : theme.text,
          pressedKeyIndex == i,
          theme.accent,
          theme.border);
    }
    return;
  }

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.sub);
  sprite->drawCenterString(currentLanguage == APP_LANGUAGE_EN ? (selectedRowLabel + " set") : (selectedRowLabel + "ぎょう"), 160, 78);

  if (englishMode) {
    static constexpr const char* englishCharTable[10][12] = {
        {"A", "B", "C", "", "", "", "", "", "", "", "", ""},
        {"D", "E", "F", "", "", "", "", "", "", "", "", ""},
        {"G", "H", "I", "", "", "", "", "", "", "", "", ""},
        {"J", "K", "L", "", "", "", "", "", "", "", "", ""},
        {"M", "N", "O", "", "", "", "", "", "", "", "", ""},
        {"P", "Q", "R", "", "", "", "", "", "", "", "", ""},
        {"S", "T", "U", "", "", "", "", "", "", "", "", ""},
        {"V", "W", "X", "", "", "", "", "", "", "", "", ""},
        {"Y", "Z", "", "", "", "", "", "", "", "", "", ""},
        {"-", ".", "'", ":", "SP", "F", "M", "", "", "", "", ""},
    };
    int rowIndex = -1;
    if (selectedRowLabel == "ABC") rowIndex = 0;
    else if (selectedRowLabel == "DEF") rowIndex = 1;
    else if (selectedRowLabel == "GHI") rowIndex = 2;
    else if (selectedRowLabel == "JKL") rowIndex = 3;
    else if (selectedRowLabel == "MNO") rowIndex = 4;
    else if (selectedRowLabel == "PQR") rowIndex = 5;
    else if (selectedRowLabel == "STU") rowIndex = 6;
    else if (selectedRowLabel == "VWX") rowIndex = 7;
    else if (selectedRowLabel == "YZ") rowIndex = 8;
    else if (selectedRowLabel == "SYM") rowIndex = 9;

    const int keyX[3] = {30, 120, 210};
    const int keyY[4] = {76, 106, 136, 166};
    for (int i = 0; i < 12; ++i) {
      const int col = i % 3;
      const int row = i / 3;
      const char* label = "";
      bool disabled = true;
      if (rowIndex >= 0) {
        label = englishCharTable[rowIndex][i];
        disabled = label[0] == '\0';
      }
      drawActionButton(
          keyX[col],
          keyY[row],
          80,
          24,
          label,
          theme.bg,
          disabled ? theme.sub : theme.text,
          pressedKeyIndex == i,
          theme.accent,
          theme.border);
    }
    sprite->setFont(&fonts::efontJA_10);
    sprite->setTextColor(theme.sub);
    if (selectedRowLabel == "SYM") {
      sprite->drawString("SP=space  F=female  M=male", 48, 184);
    }
    return;
  }

  static constexpr const char* rowKanaTable[10][5] = {
      {"ア","イ","ウ","エ","オ"},
      {"カ","キ","ク","ケ","コ"},
      {"サ","シ","ス","セ","ソ"},
      {"タ","チ","ツ","テ","ト"},
      {"ナ","ニ","ヌ","ネ","ノ"},
      {"ハ","ヒ","フ","ヘ","ホ"},
      {"マ","ミ","ム","メ","モ"},
      {"ヤ","","ユ","","ヨ"},
      {"ラ","リ","ル","レ","ロ"},
      {"ワ","","ン","ー","ヲ"}
  };
  static constexpr const char* extraLabels[3] = {"ン", "ー", "ッ"};
  const int keyX[3] = {30, 120, 210};
  const int keyY[4] = {76, 106, 136, 166};
  for (int i = 0; i < 12; ++i) {
    const int col = i % 3;
    const int row = i / 3;
    const char* label = "";
    bool disabled = false;
    if (i <= 4) {
      if (selectedRowLabel == "ア") label = rowKanaTable[0][i];
      else if (selectedRowLabel == "カ") label = rowKanaTable[1][i];
      else if (selectedRowLabel == "サ") label = rowKanaTable[2][i];
      else if (selectedRowLabel == "タ") label = rowKanaTable[3][i];
      else if (selectedRowLabel == "ナ") label = rowKanaTable[4][i];
      else if (selectedRowLabel == "ハ") label = rowKanaTable[5][i];
      else if (selectedRowLabel == "マ") label = rowKanaTable[6][i];
      else if (selectedRowLabel == "ヤ") label = rowKanaTable[7][i];
      else if (selectedRowLabel == "ラ") label = rowKanaTable[8][i];
      else if (selectedRowLabel == "ワ") label = rowKanaTable[9][i];
      if (label[0] == '\0') disabled = true;
    } else if (i >= 5 && i <= 7) {
      label = extraLabels[i - 5];
    } else if (i == 8) {
      label = "";
      disabled = true;
    } else if (i == 9) {
      label = "";
      disabled = true;
    } else if (i == 10) {
      label = "";
      disabled = true;
    } else if (i == 11) {
      label = "";
      disabled = true;
    } else {
      label = "";
      disabled = true;
    }
    drawActionButton(
        keyX[col],
        keyY[row],
        80,
        24,
        label,
        theme.bg,
        disabled ? theme.sub : theme.text,
        pressedKeyIndex == i,
        theme.accent,
        theme.border);
  }
}

void UIController::drawInfoRow(const char* label, const String& value, int y) {
  const auto& theme = getThemePalette(currentTheme);
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(theme.text);
  sprite->drawString(label, 20, y);
  sprite->setTextColor(theme.sub);
  sprite->drawString(value, 92, y);
}

void UIController::drawTypeBadge(const String& type, int x, int y) {
  const uint16_t bg = typeBadgeColor(type);
  sprite->fillRoundRect(x, y, 68, 20, 5, bg);
  sprite->setTextColor(getThemePalette(currentTheme).invertedText);
  sprite->setFont(&fonts::efontJA_12);
  sprite->drawCenterString(type, x + 34, y + 3);
}

void UIController::drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines) {
  const lgfx::IFont* primaryFont = sprite->getFont();
  int cursorX = x;
  int cursorY = y;
  int lineCount = 0;

  for (size_t i = 0; i < text.length() && lineCount < maxLines; ) {
    uint16_t codepoint = 0;
    String glyph;
    const size_t glyphLength = readUtf8Glyph(text, i, codepoint, glyph);
    if (glyphLength == 0) break;
    i += glyphLength;

    if (glyph == "\n") {
      ++lineCount;
      cursorX = x;
      cursorY = y + (lineCount * lineHeight);
      continue;
    }

    const lgfx::IFont* font = selectFontForCodepoint(codepoint, primaryFont);
    sprite->setFont(font);

    int glyphWidth = 0;
    if (codepoint == 0x20) {
      glyphWidth = 6;
    } else if (codepoint == 0x3000) {
      glyphWidth = 12;
    } else {
      glyphWidth = sprite->textWidth(glyph);
    }

    if (cursorX > x && (cursorX + glyphWidth) > (x + maxWidth)) {
      ++lineCount;
      if (lineCount >= maxLines) break;
      cursorX = x;
      cursorY = y + (lineCount * lineHeight);
    }

    if (codepoint != 0x20 && codepoint != 0x3000) {
      sprite->drawString(glyph, cursorX, cursorY);
    }
    cursorX += glyphWidth;
  }

  sprite->setFont(primaryFont);
}

void UIController::drawTabBar(TabType activeTab, int pressedTab) {
  const auto& theme = getThemePalette(currentTheme);
  const char* labels[] = {
      tr("すがた", "Look"),
      tr("せつめい", "Info"),
      tr("からだ", "Body"),
      tr("とくせい", "Ability"),
      tr("しんか", "Evo"),
  };
  int tabW = SCREEN_WIDTH / 5;
  for (int i = 0; i < 5; i++) {
    const bool isActive = (i == (int)activeTab);
    const bool isPressed = (i == pressedTab);
    uint16_t bg = isPressed ? theme.buttonPressedFill : (isActive ? theme.accent : theme.surface);
    uint16_t tx = (isActive || isPressed) ? theme.invertedText : theme.text;
    sprite->fillRect(i * tabW, TAB_BAR_Y, tabW, TAB_BAR_H, bg);
    sprite->drawRect(i * tabW, TAB_BAR_Y, tabW, TAB_BAR_H, theme.border);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(tx);
    sprite->drawCenterString(labels[i], (i * tabW) + (tabW / 2), TAB_BAR_Y + 11);
  }
}

void UIController::pushToDisplay() { sprite->pushSprite(0, 0); }
