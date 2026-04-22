#include "Renderer.h"

M5Renderer& M5Renderer::instance() {
  static M5Renderer renderer;
  return renderer;
}

lgfx::LGFX_Device& M5Renderer::device() {
  return M5.Display;
}

void M5Renderer::fillScreen(uint16_t color) {
  M5.Display.fillScreen(color);
}

void M5Renderer::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  M5.Display.fillRect(x, y, w, h, color);
}

void M5Renderer::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  M5.Display.drawRect(x, y, w, h, color);
}

void M5Renderer::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  M5.Display.fillRoundRect(x, y, w, h, radius, color);
}

void M5Renderer::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  M5.Display.drawRoundRect(x, y, w, h, radius, color);
}

void M5Renderer::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
  M5.Display.pushImage(x, y, w, h, data);
}

void M5Renderer::setFont(const lgfx::IFont* font) {
  M5.Display.setFont(font);
}

void M5Renderer::setTextColor(uint16_t color) {
  M5.Display.setTextColor(color);
}

void M5Renderer::drawString(const String& text, int32_t x, int32_t y) {
  M5.Display.drawString(text, x, y);
}

void M5Renderer::drawCenterString(const String& text, int32_t x, int32_t y) {
  M5.Display.drawCenterString(text, x, y);
}

void M5Renderer::drawRightString(const String& text, int32_t x, int32_t y) {
  M5.Display.drawRightString(text, x, y);
}

void M5Renderer::setRotation(uint8_t rotation) {
  M5.Display.setRotation(rotation);
}

void M5Renderer::setTextFont(int font) {
  M5.Display.setTextFont(font);
}

void M5Renderer::setBrightness(uint8_t brightness) {
  M5.Display.setBrightness(brightness);
}

void M5Renderer::print(const char* text) {
  M5.Display.print(text);
}

void M5Renderer::wakeup() {
  M5.Display.wakeup();
}

void M5Renderer::sleep() {
  M5.Display.sleep();
}
