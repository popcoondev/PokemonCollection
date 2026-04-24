#include "Renderer.h"
#ifdef POKEMONCOLLECTION_SIM
#include "PcRenderer.h"
#endif

M5Renderer& M5Renderer::instance() {
#ifdef POKEMONCOLLECTION_SIM
  static PcRenderer renderer;
  return reinterpret_cast<M5Renderer&>(renderer);
#else
  static M5Renderer renderer;
  return renderer;
#endif
}

lgfx::LGFX_Device& M5Renderer::device() {
#ifdef POKEMONCOLLECTION_SIM
  return reinterpret_cast<PcRenderer*>(this)->device();
#else
  return M5.Display;
#endif
}

void M5Renderer::fillScreen(uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->fillScreen(color);
#else
  M5.Display.fillScreen(color);
#endif
}

void M5Renderer::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->fillRect(x, y, w, h, color);
#else
  M5.Display.fillRect(x, y, w, h, color);
#endif
}

void M5Renderer::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->drawRect(x, y, w, h, color);
#else
  M5.Display.drawRect(x, y, w, h, color);
#endif
}

void M5Renderer::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->fillRoundRect(x, y, w, h, radius, color);
#else
  M5.Display.fillRoundRect(x, y, w, h, radius, color);
#endif
}

void M5Renderer::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->drawRoundRect(x, y, w, h, radius, color);
#else
  M5.Display.drawRoundRect(x, y, w, h, radius, color);
#endif
}

void M5Renderer::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->pushImage(x, y, w, h, data);
#else
  M5.Display.pushImage(x, y, w, h, data);
#endif
}

void M5Renderer::setFont(const lgfx::IFont* font) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->setFont(font);
#else
  M5.Display.setFont(font);
#endif
}

void M5Renderer::setTextColor(uint16_t color) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->setTextColor(color);
#else
  M5.Display.setTextColor(color);
#endif
}

void M5Renderer::drawString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->drawString(text, x, y);
#else
  M5.Display.drawString(text, x, y);
#endif
}

void M5Renderer::drawCenterString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->drawCenterString(text, x, y);
#else
  M5.Display.drawCenterString(text, x, y);
#endif
}

void M5Renderer::drawRightString(const String& text, int32_t x, int32_t y) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->drawRightString(text, x, y);
#else
  M5.Display.drawRightString(text, x, y);
#endif
}

void M5Renderer::setRotation(uint8_t rotation) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->setRotation(rotation);
#else
  M5.Display.setRotation(rotation);
#endif
}

void M5Renderer::setTextFont(int font) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->setTextFont(font);
#else
  M5.Display.setTextFont(font);
#endif
}

void M5Renderer::setBrightness(uint8_t brightness) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->setBrightness(brightness);
#else
  M5.Display.setBrightness(brightness);
#endif
}

void M5Renderer::print(const char* text) {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->print(text);
#else
  M5.Display.print(text);
#endif
}

void M5Renderer::wakeup() {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->wakeup();
#else
  M5.Display.wakeup();
#endif
}

void M5Renderer::sleep() {
#ifdef POKEMONCOLLECTION_SIM
  reinterpret_cast<PcRenderer*>(this)->sleep();
#else
  M5.Display.sleep();
#endif
}
