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

UIController::UIController() : sprite(nullptr), wakeSplashSprite(nullptr) {}

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

void UIController::drawBase() {
  sprite->fillScreen(COLOR_PK_BG);
}

const lgfx::IFont* UIController::getFallbackFont(const lgfx::IFont* primaryFont) const {
  if (primaryFont == &fonts::efontJA_16_b) return &fonts::efontCN_16_b;
  if (primaryFont == &fonts::efontJA_16) return &fonts::efontCN_16;
  if (primaryFont == &fonts::efontJA_12_b) return &fonts::efontCN_12_b;
  if (primaryFont == &fonts::efontJA_12) return &fonts::efontCN_12;
  if (primaryFont == &fonts::efontJA_10_b) return &fonts::efontCN_10_b;
  return &fonts::efontCN_10;
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
  const uint16_t overlay = blend565(COLOR_PK_SUB, COLOR_PK_CARD, 48);
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
  const uint16_t bg = pressed ? pressedFillColor : fillColor;
  sprite->fillRoundRect(x, y, w, h, 10, bg);
  sprite->drawRoundRect(x, y, w, h, 10, borderColor);
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(textColor);
  sprite->drawCenterString(label, x + (w / 2), y + ((h - 12) / 2));
}

void UIController::drawHeader(const PokemonDetail& pk, bool searchPressed) {
  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (searchPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }

  char idStr[12];
  snprintf(idStr, sizeof(idStr), "No.%04d", pk.id);
  const int baselineY = 22;
  const int idX = 22;

  sprite->setTextColor(COLOR_PK_SUB);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(idStr, idX, baselineY);

  const int nameX = idX + sprite->textWidth(idStr) + 12;

  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(pk.name, nameX, baselineY);
}

void UIController::drawAppearanceTab(const PokemonDetail& pk, bool drawImage) {
  sprite->fillRect(kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH, COLOR_PK_BG);
  if (drawImage) {
    imageLoader.loadAndDisplayPNG(*sprite, pk.id, kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH);
  }

  int cardX = 165;
  sprite->fillRoundRect(cardX, 60, 145, 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(cardX, 60, 145, 135, 8, COLOR_PK_BORDER);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString(pk.category, cardX + 10, 75);

  int ty = 95;
  for (const auto& t : pk.types) {
    drawTypeBadge(t, cardX + 10, ty);
    ty += 22;
  }

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString("高さ", cardX + 10, 145);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString(pk.height, cardX + 58, 145);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString("重さ", cardX + 10, 167);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString(pk.weight, cardX + 58, 167);
}

void UIController::drawAppearancePreviewFeedback() {
  drawPressedOverlay(kAppearanceImageX, kAppearanceImageY, kAppearanceImageW, kAppearanceImageH, 0);
}

void UIController::drawDescriptionTab(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_TEXT);
  drawWrappedText(pk.description, 20, 72, 280, 22, 5);
}

void UIController::drawBodyTab(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);

  drawInfoRow("分類", pk.category, 74);
  drawInfoRow("高さ", pk.height, 102);
  drawInfoRow("重さ", pk.weight, 130);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString("タイプ", 24, 160);

  int typeX = 92;
  for (const auto& type : pk.types) {
    drawTypeBadge(type, typeX, 154);
    typeX += 68;
  }
}

void UIController::drawAbilityTab(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);

  int y = 72;
  for (size_t i = 0; i < pk.abilities.size() && i < 2; ++i) {
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(COLOR_PK_TEXT);
    sprite->drawString(pk.abilities[i].name, 20, y);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(COLOR_PK_SUB);
    drawWrappedText(pk.abilities[i].description, 20, y + 24, 280, 20, 2);
    y += 62;
  }
}

void UIController::drawEvolutionTab(const PokemonDetail& pk, int pressedEvolutionIndex, bool drawImages) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);

  const int totalCount = std::min<int>(pk.evolutions.size(), 8);
  for (int i = 0; i < totalCount; ++i) {
    const auto layout = getEvolutionCardLayout(i, totalCount);
    sprite->fillRoundRect(layout.x, layout.y, layout.w, layout.h, 8, COLOR_PK_BG);
    sprite->drawRoundRect(layout.x, layout.y, layout.w, layout.h, 8, COLOR_PK_BORDER);

    sprite->fillRect(layout.imageX, layout.imageY, layout.imageW, layout.imageH, COLOR_PK_BG);
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
    sprite->setTextColor(COLOR_PK_SUB);
    if (totalCount <= 3) {
      sprite->setFont(&fonts::efontJA_12);
      sprite->drawCenterString(idStr, layout.x + (layout.w / 2), layout.idCenterY);
      sprite->setTextColor(COLOR_PK_TEXT);
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
  sprite->setTextColor(COLOR_PK_SUB);
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

void UIController::drawMenuScreen(bool pokedexPressed, bool quizPressed, bool slideshowPressed, bool guidePressed, bool preview3dEnabled, bool preview3dPressed, int selectedVolumeIndex, int pressedVolumeIndex) {
  sprite->fillScreen(COLOR_PK_BG);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawCenterString("ポケモン図鑑", SCREEN_WIDTH / 2, 22);

  constexpr int menuLeftX = 24;
  constexpr int menuRightX = 166;
  constexpr int menuButtonW = 130;
  constexpr int menuButtonH = 34;
  constexpr int menuRowY0 = 64;
  constexpr int menuRowGap = 42;

  drawActionButton(menuLeftX, menuRowY0, menuButtonW, menuButtonH, "ポケモンずかん", COLOR_PK_RED, COLOR_PK_CARD, pokedexPressed, COLOR_PK_TEXT, COLOR_PK_RED);
  drawActionButton(menuLeftX, menuRowY0 + menuRowGap, menuButtonW, menuButtonH, "ポケモンクイズ", COLOR_PK_CARD, COLOR_PK_TEXT, quizPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(menuLeftX, menuRowY0 + (menuRowGap * 2), menuButtonW, menuButtonH, "スライドショー", COLOR_PK_CARD, COLOR_PK_TEXT, slideshowPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(menuRightX, menuRowY0, menuButtonW, menuButtonH, "こうりゃく", COLOR_PK_CARD, COLOR_PK_TEXT, guidePressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(menuRightX, menuRowY0 + menuRowGap, menuButtonW, menuButtonH, "せってい", COLOR_PK_CARD, COLOR_PK_TEXT, preview3dPressed || pressedVolumeIndex >= 0, COLOR_PK_BORDER, COLOR_PK_BORDER);
}

void UIController::drawSettingsScreen(
    bool backPressed,
    bool preview3dEnabled,
    bool preview3dPressed,
    bool previewCaptionEnabled,
    bool previewCaptionPressed,
    int selectedVolumeIndex,
    int pressedVolumeIndex) {
  sprite->fillScreen(COLOR_PK_BG);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawCenterString("せってい", SCREEN_WIDTH / 2, 22);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("プレビュー", 24, 72);
  drawActionButton(
      202,
      60,
      94,
      30,
      preview3dEnabled ? "3D ON" : "3D OFF",
      preview3dEnabled ? COLOR_PK_RED : COLOR_PK_CARD,
      preview3dEnabled ? COLOR_PK_CARD : COLOR_PK_TEXT,
      preview3dPressed,
      COLOR_PK_RED,
      preview3dEnabled ? COLOR_PK_RED : COLOR_PK_BORDER);
  drawActionButton(
      202,
      98,
      94,
      30,
      previewCaptionEnabled ? "文字 ON" : "文字 OFF",
      previewCaptionEnabled ? COLOR_PK_RED : COLOR_PK_CARD,
      previewCaptionEnabled ? COLOR_PK_CARD : COLOR_PK_TEXT,
      previewCaptionPressed,
      COLOR_PK_RED,
      previewCaptionEnabled ? COLOR_PK_RED : COLOR_PK_BORDER);

  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("おんりょう", 24, 150);
  static constexpr const char* volumeLabels[4] = {"大", "中", "小", "なし"};
  for (int i = 0; i < 4; ++i) {
    const int x = 24 + (i * 70);
    const bool selected = selectedVolumeIndex == i;
    const bool pressed = pressedVolumeIndex == i;
    const uint16_t fill = selected ? COLOR_PK_BAR : COLOR_PK_CARD;
    const uint16_t text = selected ? COLOR_PK_CARD : COLOR_PK_TEXT;
    const uint16_t border = selected ? COLOR_PK_BAR : COLOR_PK_BORDER;
    drawActionButton(
        x,
        162,
        58,
        32,
        volumeLabels[i],
        fill,
        text,
        pressed,
        selected ? COLOR_PK_SUB : COLOR_PK_BORDER,
        border);
  }

}

void UIController::drawGuideMenuScreen(bool pokemonPressed, bool locationPressed, bool backPressed) {
  sprite->fillScreen(COLOR_PK_BG);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawCenterString("こうりゃく", SCREEN_WIDTH / 2, 22);

  drawActionButton(32, 72, SCREEN_WIDTH - 64, 42, "ポケモンからみる", COLOR_PK_RED, COLOR_PK_CARD, pokemonPressed, COLOR_PK_TEXT, COLOR_PK_RED);
  drawActionButton(32, 126, SCREEN_WIDTH - 64, 42, "場所からみる", COLOR_PK_CARD, COLOR_PK_TEXT, locationPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
}

void UIController::drawGuidePokemonListScreen(
    const char* title,
    const std::vector<String>& labels,
    const std::vector<bool>& caughtFlags,
    bool backPressed,
    int pressedItemIndex,
    bool prevPressed,
    bool nextPressed) {
  sprite->fillScreen(COLOR_PK_BG);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
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
        COLOR_PK_BG,
        COLOR_PK_TEXT,
        pressedItemIndex == i,
        COLOR_PK_RED,
        COLOR_PK_BORDER);
    if (i < static_cast<int>(caughtFlags.size()) && caughtFlags[i]) {
      const int cx = x + itemW - 8;
      const int cy = y + (itemH / 2);
      sprite->drawLine(cx - 4, cy, cx - 1, cy + 3, COLOR_PK_RED);
      sprite->drawLine(cx - 1, cy + 3, cx + 6, cy - 4, COLOR_PK_RED);
      sprite->drawLine(cx - 4, cy + 1, cx - 1, cy + 4, COLOR_PK_RED);
      sprite->drawLine(cx - 1, cy + 4, cx + 6, cy - 3, COLOR_PK_RED);
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
  drawGuidePokemonListScreen("場所からみる", displayLabels, emptyFlags, backPressed, pressedItemIndex, prevPressed, nextPressed);
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
  static const char* kTabs[5] = {"しんか", "しゅつげん", "わざ", "マシン", "ひでん"};

  sprite->fillScreen(COLOR_PK_BG);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (backPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("No.", 18, 16);
  char idText[8];
  snprintf(idText, sizeof(idText), "%04d", pokemonId);
  sprite->setTextColor(COLOR_PK_RED);
  sprite->drawString(idText, 42, 16);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString(headerLabel, 92, 16);
  drawDetailNavigation(prevPressed, nextPressed);

  const bool showIcon = activeTab == 0;
  int textX = 20;
  int textY = 62;
  int textMaxLines = 9;
  if (showIcon) {
    sprite->fillRoundRect(12, 56, 92, 92, 10, COLOR_PK_CARD);
    sprite->drawRoundRect(12, 56, 92, 92, 10, COLOR_PK_BORDER);
    drawQuizPokemonImage(pokemonId, 16, 60, 84, 84);
    drawActionButton(
        16,
        152,
        84,
        22,
        "つかまえた",
        caughtEnabled ? COLOR_PK_RED : COLOR_PK_BG,
        caughtEnabled ? COLOR_PK_CARD : COLOR_PK_TEXT,
        caughtPressed,
        caughtEnabled ? COLOR_PK_TEXT : COLOR_PK_BORDER,
        caughtEnabled ? COLOR_PK_RED : COLOR_PK_BORDER);
    sprite->fillRoundRect(112, 56, SCREEN_WIDTH - 124, 122, 10, COLOR_PK_CARD);
    sprite->drawRoundRect(112, 56, SCREEN_WIDTH - 124, 122, 10, COLOR_PK_BORDER);
    if (pagePressed && pageCount > 1) {
      drawPressedOverlay(112, 56, SCREEN_WIDTH - 124, 122, 10);
    }
    textX = 120;
    textY = 64;
    textMaxLines = 8;
  } else {
    sprite->fillRoundRect(12, 56, SCREEN_WIDTH - 24, 122, 10, COLOR_PK_CARD);
    sprite->drawRoundRect(12, 56, SCREEN_WIDTH - 24, 122, 10, COLOR_PK_BORDER);
    if (pagePressed && pageCount > 1) {
      drawPressedOverlay(12, 56, SCREEN_WIDTH - 24, 122, 10);
    }
    textX = 20;
    textY = 64;
    textMaxLines = 8;
  }

  sprite->setFont(&fonts::efontJA_12);
  if (lines.empty()) {
    sprite->setTextColor(COLOR_PK_SUB);
    sprite->drawCenterString("データなし", SCREEN_WIDTH / 2, 108);
  } else {
    const int lineH = 14;
    const int startIndex = pageIndex * textMaxLines;
    for (int i = 0; i < textMaxLines; ++i) {
      const int lineIndex = startIndex + i;
      if (lineIndex >= static_cast<int>(lines.size())) break;
      sprite->setTextColor(COLOR_PK_TEXT);
      sprite->drawString(lines[lineIndex], textX, textY + (i * lineH));
    }
  }

  if (pageCount > 1) {
    char pageText[12];
    snprintf(pageText, sizeof(pageText), "%d/%d", pageIndex + 1, pageCount);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(COLOR_PK_SUB);
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
    uint16_t bg = isActive ? COLOR_PK_RED : COLOR_PK_CARD;
    uint16_t fg = isActive ? COLOR_PK_CARD : COLOR_PK_TEXT;
    if (pressedTab == i) {
      bg = COLOR_PK_RED;
      fg = COLOR_PK_CARD;
    }
    sprite->fillRect(x, y, tabW, tabH, bg);
    sprite->drawRect(x, y, tabW, tabH, COLOR_PK_BORDER);
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
  M5.Display.setTextColor(COLOR_PK_SUB);
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
  sprite->fillRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, flashActive ? COLOR_PK_BAR : COLOR_PK_CARD);
  sprite->drawRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, COLOR_PK_BORDER);

  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);
  if (menuPressed) {
    drawPressedOverlay(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10);
  }
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawCenterString(nameMode ? "ポケモンずかん" : "No.でえらぶ", SCREEN_WIDTH / 2, 22);

  if (!nameMode) {
    const int digitX[4] = {60, 110, 160, 210};
    const int digitStep[4] = {1000, 100, 10, 1};

    char idText[12];
    snprintf(idText, sizeof(idText), "%04d", selectedId);
    const bool validId = selectedId >= MIN_POKEMON_ID && selectedName.length() > 0;

    for (int i = 0; i < 4; ++i) {
      const bool upPressed = pressedDigitDelta == digitStep[i];
      const bool downPressed = pressedDigitDelta == -digitStep[i];

      drawActionButton(digitX[i], 52, 40, 34, "", COLOR_PK_BG, COLOR_PK_TEXT, upPressed, COLOR_PK_RED, COLOR_PK_BORDER);
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(upPressed ? COLOR_PK_CARD : COLOR_PK_TEXT);
      sprite->drawCenterString("▲", digitX[i] + 20, 61);

      char digitText[2] = {idText[i], '\0'};
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(COLOR_PK_RED);
      sprite->drawCenterString(digitText, digitX[i] + 20, 92);

      drawActionButton(digitX[i], 120, 40, 34, "", COLOR_PK_BG, COLOR_PK_TEXT, downPressed, COLOR_PK_RED, COLOR_PK_BORDER);
      sprite->setFont(&fonts::efontJA_16_b);
      sprite->setTextColor(downPressed ? COLOR_PK_CARD : COLOR_PK_TEXT);
      sprite->drawCenterString("▼", digitX[i] + 20, 129);
    }

    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(validId ? COLOR_PK_TEXT : COLOR_PK_SUB);
    sprite->drawCenterString(validId ? selectedName : "データなし", 160, 156);

    drawActionButton(20, 178, 280, 40, "ひらく", COLOR_PK_RED, COLOR_PK_CARD, openPressed, COLOR_PK_TEXT, COLOR_PK_RED);
    return;
  }

  const bool hasKeyword = nameQuery.length() > 0;
  const String keywordLabel = hasKeyword ? nameQuery : "キーワード";
  const uint16_t keywordFill = hasKeyword ? COLOR_PK_RED : COLOR_PK_BG;
  const uint16_t keywordText = hasKeyword ? COLOR_PK_CARD : COLOR_PK_TEXT;
  drawActionButton(44, 52, 114, 24, keywordLabel.c_str(), keywordFill, keywordText, cancelPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(162, 52, 114, 24, "ばんごう", COLOR_PK_BG, COLOR_PK_TEXT, modePressed, COLOR_PK_BORDER, COLOR_PK_BORDER);

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

    uint16_t fill = pressed ? COLOR_PK_TEXT : COLOR_PK_BG;
    uint16_t textColor = pressed ? COLOR_PK_CARD : COLOR_PK_TEXT;
    sprite->fillRoundRect(x, y, itemW, itemH, 6, fill);
    sprite->drawRoundRect(x, y, itemW, itemH, 6, COLOR_PK_BORDER);

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
  sprite->fillRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, COLOR_PK_CARD);
  sprite->drawRoundRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, 12, COLOR_PK_BORDER);

  drawActionButton(12, 202, 88, 28, "もどる", COLOR_PK_BG, COLOR_PK_TEXT, backPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(116, 202, 88, 28, "けす", COLOR_PK_BG, COLOR_PK_TEXT, deletePressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(220, 202, 88, 28, "クリア", COLOR_PK_BG, COLOR_PK_TEXT, clearPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("おとを えらぶ", 12, 16);

  sprite->fillRoundRect(12, 32, 296, 24, 6, COLOR_PK_BG);
  sprite->drawRoundRect(12, 32, 296, 24, 6, COLOR_PK_BORDER);
  sprite->setTextColor(nameQuery.length() > 0 ? COLOR_PK_TEXT : COLOR_PK_SUB);
  sprite->drawString(nameQuery.length() > 0 ? nameQuery : "ここに なまえが はいります", 20, 38);
  sprite->fillRoundRect(12, 56, 120, 16, 6, vowelMode ? COLOR_PK_RED : COLOR_PK_BG);
  sprite->drawRoundRect(12, 56, 120, 16, 6, COLOR_PK_BORDER);
  sprite->setTextColor(vowelMode ? COLOR_PK_CARD : COLOR_PK_SUB);
  sprite->drawCenterString(vowelMode ? "もじを えらぶ" : "ぎょうを えらぶ", 72, 60);

  if (!vowelMode) {
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
          COLOR_PK_BG,
          (i >= 10) ? COLOR_PK_SUB : COLOR_PK_TEXT,
          pressedKeyIndex == i,
          COLOR_PK_RED,
          COLOR_PK_BORDER);
    }
    return;
  }

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawCenterString(selectedRowLabel + "ぎょう", 160, 78);

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
        COLOR_PK_BG,
        disabled ? COLOR_PK_SUB : COLOR_PK_TEXT,
        pressedKeyIndex == i,
        COLOR_PK_RED,
        COLOR_PK_BORDER);
  }
}

void UIController::drawInfoRow(const char* label, const String& value, int y) {
  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString(label, 20, y);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString(value, 92, y);
}

void UIController::drawTypeBadge(const String& type, int x, int y) {
  const uint16_t bg = typeBadgeColor(type);
  sprite->fillRoundRect(x, y, 68, 20, 5, bg);
  sprite->setTextColor(COLOR_PK_CARD);
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
  const char* labels[] = {"すがた", "せつめい", "からだ", "とくせい", "しんか"};
  int tabW = SCREEN_WIDTH / 5;
  for (int i = 0; i < 5; i++) {
    const bool isActive = (i == (int)activeTab);
    const bool isPressed = (i == pressedTab);
    uint16_t bg = isPressed ? COLOR_PK_TEXT : (isActive ? COLOR_PK_RED : COLOR_PK_CARD);
    uint16_t tx = (isActive || isPressed) ? COLOR_PK_CARD : COLOR_PK_TEXT;
    sprite->fillRect(i * tabW, TAB_BAR_Y, tabW, TAB_BAR_H, bg);
    sprite->drawRect(i * tabW, TAB_BAR_Y, tabW, TAB_BAR_H, COLOR_PK_BORDER);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(tx);
    sprite->drawCenterString(labels[i], (i * tabW) + (tabW / 2), TAB_BAR_Y + 11);
  }
}

void UIController::pushToDisplay() { sprite->pushSprite(0, 0); }
