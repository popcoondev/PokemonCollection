#include "UIController.h"

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

void UIController::drawHeader(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 10, COLOR_PK_BORDER);

  char idStr[12];
  snprintf(idStr, sizeof(idStr), "No.%04d", pk.id);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->setFont(&fonts::efontJA_10);
  sprite->drawString(idStr, 22, 14);

  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->setFont(&fonts::efontJA_16_b);
  sprite->drawString(pk.name, 22, 26);

  sprite->setFont(&fonts::efontJA_10);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString("SEARCH", SCREEN_WIDTH - 64, 20);
}

void UIController::drawAppearanceTab(const PokemonDetail& pk) {
  sprite->fillCircle(85, 125, 55, COLOR_PK_CARD);
  sprite->drawCircle(85, 125, 55, COLOR_PK_BORDER);

  const int imageX = 35;
  const int imageY = 75;
  const int imageW = 100;
  const int imageH = 100;

  imageLoader.loadAndDisplayPNG(*sprite, pk.id, imageX, imageY, imageW, imageH);
  
  int cardX = 165;
  sprite->fillRoundRect(cardX, 60, 145, 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(cardX, 60, 145, 135, 8, COLOR_PK_BORDER);

  sprite->setFont(&fonts::efontJA_10);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString(pk.category, cardX + 10, 75);

  int ty = 95;
  for (const auto& t : pk.types) {
    drawTypeBadge(t, cardX + 10, ty);
    ty += 22;
  }

  sprite->setFont(&fonts::efontJA_10);
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
  sprite->setFont(&fonts::efontJA_10);
  sprite->setTextColor(COLOR_PK_TEXT);
  drawWrappedText(pk.description, 20, 74, 280, 20, 6);
}

void UIController::drawBodyTab(const PokemonDetail& pk) {
  sprite->fillRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_CARD);
  sprite->drawRoundRect(MARGIN, 60, SCREEN_WIDTH - (MARGIN * 2), 135, 8, COLOR_PK_BORDER);

  drawInfoRow("分類", pk.category, 74);
  drawInfoRow("高さ", pk.height, 102);
  drawInfoRow("重さ", pk.weight, 130);

  sprite->setFont(&fonts::efontJA_10);
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
    sprite->setFont(&fonts::efontJA_10);
    sprite->setTextColor(COLOR_PK_SUB);
    drawWrappedText(pk.abilities[i].description, 20, y + 22, 280, 18, 3);
    y += 60;
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

    char idStr[12];
    snprintf(idStr, sizeof(idStr), "No.%04d", pk.evolutions[i].id);
    sprite->setFont(&fonts::efontJA_10);
    sprite->setTextColor(COLOR_PK_SUB);
    sprite->drawCenterString(idStr, x + 42, y + 12);
    sprite->setTextColor(COLOR_PK_TEXT);
    drawWrappedText(pk.evolutions[i].name, x + 8, y + 34, 68, 16, 2);

    x += 96;
  }
}

void UIController::drawInfoRow(const char* label, const String& value, int y) {
  sprite->setFont(&fonts::efontJA_10);
  sprite->setTextColor(COLOR_PK_TEXT);
  sprite->drawString(label, 20, y);
  sprite->setTextColor(COLOR_PK_SUB);
  sprite->drawString(value, 92, y);
}

void UIController::drawTypeBadge(const String& type, int x, int y) {
  sprite->fillRoundRect(x, y, 60, 18, 4, COLOR_PK_TEXT);
  sprite->setTextColor(COLOR_PK_CARD);
  sprite->setFont(&fonts::efontJA_10);
  sprite->drawCenterString(type, x + 30, y + 2);
}

void UIController::drawWrappedText(const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines) {
  String line;
  int lineCount = 0;

  for (size_t i = 0; i < text.length() && lineCount < maxLines; ++i) {
    String candidate = line + text[i];
    if (sprite->textWidth(candidate) > maxWidth && !line.isEmpty()) {
      sprite->drawString(line, x, y + (lineCount * lineHeight));
      line = String(text[i]);
      ++lineCount;
    } else {
      line = candidate;
    }
  }

  if (lineCount < maxLines && !line.isEmpty()) {
    sprite->drawString(line, x, y + (lineCount * lineHeight));
  }
}

void UIController::drawTabBar(TabType activeTab) {
  const char* labels[] = {"すがた", "せつめい", "からだ", "とくせい", "しんか"};
  int tabW = SCREEN_WIDTH / 5;
  for (int i = 0; i < 5; i++) {
    uint16_t bg = (i == (int)activeTab) ? COLOR_PK_RED : COLOR_PK_CARD;
    uint16_t tx = (i == (int)activeTab) ? COLOR_PK_CARD : COLOR_PK_TEXT;
    sprite->fillRect(i * tabW, 204, tabW, TAB_BAR_H, bg);
    sprite->drawRect(i * tabW, 204, tabW, TAB_BAR_H, COLOR_PK_BORDER);
    sprite->setTextColor(tx);
    sprite->drawCenterString(labels[i], (i * tabW) + (tabW / 2), 214);
  }
}

void UIController::pushToDisplay() { sprite->pushSprite(0, 0); }
