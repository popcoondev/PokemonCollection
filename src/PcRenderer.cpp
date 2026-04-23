#include "PcRenderer.h"
#ifdef POKEMONCOLLECTION_SIM
#include "SimPlatform.h"
#endif

namespace {
class NullDisplayDevice : public lgfx::LGFX_Device {
};

NullDisplayDevice gNullDevice;
}

lgfx::LGFX_Device& PcRenderer::device() {
  return gNullDevice;
}

void PcRenderer::fillScreen(uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::clear(color);
#else
  (void)color;
#endif
}

void PcRenderer::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::fillRect(x, y, w, h, color);
#else
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
#endif
}

void PcRenderer::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::drawRect(x, y, w, h, color);
#else
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
#endif
}

void PcRenderer::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  (void)radius;
  SimPlatform::fillRect(x, y, w, h, color);
#else
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)radius;
  (void)color;
#endif
}

void PcRenderer::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  (void)radius;
  SimPlatform::drawRect(x, y, w, h, color);
#else
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)radius;
  (void)color;
#endif
}

void PcRenderer::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)data;
}

void PcRenderer::setFont(const lgfx::IFont* font) {
  (void)font;
}

void PcRenderer::setTextColor(uint16_t color) {
  currentTextColor_ = color;
}

void PcRenderer::drawString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::drawText(text.c_str(), x, y, currentTextColor_, 0);
#else
  (void)text;
  (void)x;
  (void)y;
#endif
}

void PcRenderer::drawCenterString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::drawText(text.c_str(), x, y, currentTextColor_, 1);
#else
  (void)text;
  (void)x;
  (void)y;
#endif
}

void PcRenderer::drawRightString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::drawText(text.c_str(), x, y, currentTextColor_, 2);
#else
  (void)text;
  (void)x;
  (void)y;
#endif
}

void PcRenderer::setRotation(uint8_t rotation) {
  (void)rotation;
}

void PcRenderer::setTextFont(int font) {
  (void)font;
}

void PcRenderer::setBrightness(uint8_t brightness) {
  (void)brightness;
}

void PcRenderer::print(const char* text) {
  (void)text;
}

void PcRenderer::wakeup() {
}

void PcRenderer::sleep() {
}
