#include <M5Unified.h>

#include <SD.h>

namespace fonts {
lgfx::IFont Font0;
lgfx::IFont efontJA_10;
lgfx::IFont efontJA_10_b;
lgfx::IFont efontJA_12;
lgfx::IFont efontJA_12_b;
lgfx::IFont efontJA_16;
lgfx::IFont efontJA_16_b;
lgfx::IFont efontCN_10;
lgfx::IFont efontCN_10_b;
lgfx::IFont efontCN_12;
lgfx::IFont efontCN_12_b;
lgfx::IFont efontCN_16;
lgfx::IFont efontCN_16_b;
}  // namespace fonts

LGFX_Sprite::LGFX_Sprite(lgfx::LGFX_Device* nextTarget)
    : target(dynamic_cast<lgfx::LGFXBase*>(nextTarget)),
      currentFont(&fonts::efontJA_12),
      created(false),
      bufferStub(1),
      spriteWidth(0),
      spriteHeight(0) {
}

LGFX_Sprite::~LGFX_Sprite() = default;

bool LGFX_Sprite::createSprite(int32_t width, int32_t height) {
  spriteWidth = width;
  spriteHeight = height;
  created = true;
  return true;
}

void LGFX_Sprite::deleteSprite() {
  created = false;
}

void LGFX_Sprite::setColorDepth(int depth) {
  (void)depth;
}

void LGFX_Sprite::setPsram(bool enabled) {
  (void)enabled;
}

void* LGFX_Sprite::getBuffer() const {
  return created ? const_cast<uint8_t*>(&bufferStub) : nullptr;
}

int32_t LGFX_Sprite::width() const {
  return spriteWidth;
}

int32_t LGFX_Sprite::height() const {
  return spriteHeight;
}

void LGFX_Sprite::pushSprite(int32_t x, int32_t y, uint16_t transparent) {
  (void)x;
  (void)y;
  (void)transparent;
}

void LGFX_Sprite::pushSprite(LGFX_Sprite* nextTarget, int32_t x, int32_t y, uint16_t transparent) {
  (void)x;
  (void)y;
  (void)transparent;
  if (nextTarget != nullptr) {
    target = nextTarget;
  }
}

void LGFX_Sprite::pushSprite(lgfx::LGFX_Device* nextTarget, int32_t x, int32_t y, uint16_t transparent) {
  (void)x;
  (void)y;
  (void)transparent;
  target = dynamic_cast<lgfx::LGFXBase*>(nextTarget);
}

void LGFX_Sprite::fillScreen(uint16_t color) {
  if (target != nullptr) target->fillScreen(color);
}

void LGFX_Sprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (target != nullptr) target->fillRect(x, y, w, h, color);
}

void LGFX_Sprite::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (target != nullptr) target->drawRect(x, y, w, h, color);
}

void LGFX_Sprite::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
  if (target != nullptr) target->fillRoundRect(x, y, w, h, r, color);
}

void LGFX_Sprite::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
  if (target != nullptr) target->drawRoundRect(x, y, w, h, r, color);
}

void LGFX_Sprite::setFont(const lgfx::IFont* font) {
  currentFont = font;
  if (target != nullptr) target->setFont(font);
}

const lgfx::IFont* LGFX_Sprite::getFont() const {
  return currentFont;
}

void LGFX_Sprite::setTextColor(uint16_t color) {
  if (target != nullptr) target->setTextColor(color);
}

void LGFX_Sprite::drawString(const String& text, int32_t x, int32_t y) {
  if (target != nullptr) target->drawString(text, x, y);
}

void LGFX_Sprite::drawCenterString(const String& text, int32_t x, int32_t y) {
  if (target != nullptr) target->drawCenterString(text, x, y);
}

void LGFX_Sprite::drawRightString(const String& text, int32_t x, int32_t y) {
  if (target != nullptr) target->drawRightString(text, x, y);
}

size_t LGFX_Sprite::textWidth(const String& text) const {
  if (target != nullptr) return target->textWidth(text);
  return text.length() * 8;
}

bool LGFX_Sprite::drawPngFile(const SDClass& fs,
                              const char* path,
                              int32_t x,
                              int32_t y,
                              int32_t sx,
                              int32_t sy,
                              int32_t sw,
                              int32_t sh,
                              float scaleX,
                              float scaleY) {
  if (target == nullptr) return false;
  return target->drawPngFile(fs, path, x, y, sx, sy, sw, sh, scaleX, scaleY);
}

bool LGFX_Sprite::drawPngFile(const SDClass& fs,
                              const char* path,
                              int32_t x,
                              int32_t y,
                              int32_t maxWidth,
                              int32_t maxHeight) {
  return drawPngFile(fs, path, x, y, 0, 0, maxWidth, maxHeight, 1.0f, 1.0f);
}

void LGFX_Sprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
  if (target != nullptr) target->pushImage(x, y, w, h, data);
}

void LGFX_Sprite::drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
  if (target != nullptr) target->drawCircle(x, y, r, color);
}

void LGFX_Sprite::fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
  if (target != nullptr) target->fillCircle(x, y, r, color);
}

void LGFX_Sprite::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  if (target != nullptr) target->drawLine(x0, y0, x1, y1, color);
}

void LGFX_Sprite::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
  if (target != nullptr) target->drawFastHLine(x, y, w, color);
}

void LGFX_Sprite::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
  if (target != nullptr) target->drawFastVLine(x, y, h, color);
}
