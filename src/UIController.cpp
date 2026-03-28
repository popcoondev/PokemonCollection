#include "UIController.h"

namespace {
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
}

UIController::UIController() : sprite(nullptr) {}

UIController::~UIController() {
  if (sprite != nullptr) {
    sprite->deleteSprite();
    delete sprite;
    sprite = nullptr;
  }
}

bool UIController::begin() {
  sprite = new LGFX_Sprite(&M5.Display);
  if (sprite == nullptr) {
    return false;
  }
  if (!imageLoader.begin()) {
    return false;
  }
  return sprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
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
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->setFont(&fonts::efontJA_12);
  sprite->drawString(idStr, 22, 14);

  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(pk.name, 22, 26);
}

void UIController::drawAppearanceTab(const PokemonDetail& pk) {
  const int imageX = 6;
  const int imageY = 54;
  const int imageW = 140;
  const int imageH = 140;

  imageLoader.loadAndDisplayPNG(*sprite, pk.id, imageX, imageY, imageW, imageH);
  
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

void UIController::drawEvolutionTab(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);

  int x = 22;
  int y = 80;
  for (size_t i = 0; i < pk.evolutions.size() && i < 3; ++i) {
    sprite->fillRoundRect(x, y, 84, 80, 8, COLOR_PK_BG);
    sprite->drawRoundRect(x, y, 84, 80, 8, COLOR_PK_BORDER);

    imageLoader.loadAndDisplayPNG(*sprite, pk.evolutions[i].id, x + 17, y + 6, 50, 32);

    char idStr[12];
    snprintf(idStr, sizeof(idStr), "No.%04d", pk.evolutions[i].id);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(COLOR_PK_SUB);
    sprite->drawCenterString(idStr, x + 42, y + 40);
    sprite->setTextColor(COLOR_PK_TEXT);
    drawWrappedText(pk.evolutions[i].name, x + 8, y + 56, 68, 14, 2);

    x += 96;
  }
}

void UIController::drawDetailNavigation(bool prevPressed, bool nextPressed) {
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->setFont(&fonts::efontJA_12);
  sprite->drawCenterString("◀", 18, 50);
  sprite->drawCenterString("▶", SCREEN_WIDTH - 18, 50);

  if (prevPressed) {
    drawPressedOverlay(0, 0, 40, 204);
  }
  if (nextPressed) {
    drawPressedOverlay(SCREEN_WIDTH - 40, 0, 40, 204);
  }
}

void UIController::drawSearchScreen(
    uint16_t selectedId,
    const String& selectedName,
    int pressedDigitDelta,
    bool cancelPressed,
    bool openPressed) {
  const int digitX[4] = {70, 115, 160, 205};
  const int digitStep[4] = {1000, 100, 10, 1};

  sprite->fillRoundRect(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 24, 14, COLOR_PK_CARD);
  sprite->drawRoundRect(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 24, 14, COLOR_PK_BORDER);

  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString("SEARCH", 26, 24);

  sprite->setFont(&fonts::efontJA_12);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("えらびたい No. を へんこう", 26, 48);

  char idText[12];
  snprintf(idText, sizeof(idText), "%04d", selectedId);
  const bool validId = selectedId >= MIN_POKEMON_ID && selectedId <= MAX_POKEMON_ID && selectedName.length() > 0;

  for (int i = 0; i < 4; ++i) {
    const bool upPressed = pressedDigitDelta == digitStep[i];
    const bool downPressed = pressedDigitDelta == -digitStep[i];

    drawActionButton(digitX[i], 66, 32, 28, "", COLOR_PK_BG, COLOR_PK_TEXT, upPressed, COLOR_PK_RED, COLOR_PK_BORDER);
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(upPressed ? COLOR_PK_CARD : COLOR_PK_TEXT);
    sprite->drawCenterString("▲", digitX[i] + 16, 72);

    sprite->fillRoundRect(digitX[i], 100, 32, 42, 8, COLOR_PK_BG);
    sprite->drawRoundRect(digitX[i], 100, 32, 42, 8, COLOR_PK_BORDER);
    char digitText[2] = {idText[i], '\0'};
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(COLOR_PK_RED);
    sprite->drawCenterString(digitText, digitX[i] + 16, 112);

    drawActionButton(digitX[i], 148, 32, 28, "", COLOR_PK_BG, COLOR_PK_TEXT, downPressed, COLOR_PK_RED, COLOR_PK_BORDER);
    sprite->setFont(&fonts::efontJA_16_b);
    sprite->setTextColor(downPressed ? COLOR_PK_CARD : COLOR_PK_TEXT);
    sprite->drawCenterString("▼", digitX[i] + 16, 154);
  }

  sprite->setFont(&fonts::efontJA_16_b);
  sprite->setTextColor(validId ? COLOR_PK_TEXT : COLOR_PK_SUB);
  sprite->drawCenterString(validId ? selectedName : "データなし", 160, 178);

  drawActionButton(24, 196, 126, 34, "もどる", COLOR_PK_BG, COLOR_PK_TEXT, cancelPressed, COLOR_PK_BORDER, COLOR_PK_BORDER);
  drawActionButton(170, 196, 126, 34, "ひらく", COLOR_PK_RED, COLOR_PK_CARD, openPressed, COLOR_PK_TEXT, COLOR_PK_RED);
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
    sprite->fillRect(i * tabW, 204, tabW, TAB_BAR_H, bg);
    sprite->drawRect(i * tabW, 204, tabW, TAB_BAR_H, COLOR_PK_BORDER);
    sprite->setFont(&fonts::efontJA_12);
    sprite->setTextColor(tx);
    sprite->drawCenterString(labels[i], (i * tabW) + (tabW / 2), 214);
  }
}

void UIController::pushToDisplay() { sprite->pushSprite(0, 0); }
