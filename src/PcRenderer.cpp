#include "PcRenderer.h"

namespace {
class NullDisplayDevice : public lgfx::LGFX_Device {
};

NullDisplayDevice gNullDevice;
}

lgfx::LGFX_Device& PcRenderer::device() {
  return gNullDevice;
}

void PcRenderer::fillScreen(uint16_t color) {
  (void)color;
}

void PcRenderer::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
}

void PcRenderer::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
}

void PcRenderer::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)radius;
  (void)color;
}

void PcRenderer::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)radius;
  (void)color;
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
  (void)color;
}

void PcRenderer::drawString(const String& text, int32_t x, int32_t y) {
  (void)text;
  (void)x;
  (void)y;
}

void PcRenderer::drawCenterString(const String& text, int32_t x, int32_t y) {
  (void)text;
  (void)x;
  (void)y;
}

void PcRenderer::drawRightString(const String& text, int32_t x, int32_t y) {
  (void)text;
  (void)x;
  (void)y;
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
