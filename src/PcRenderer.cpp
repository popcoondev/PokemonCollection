#include "PcRenderer.h"
#ifdef POKEMONCOLLECTION_SIM
#include "SimPlatform.h"
#endif

#ifdef POKEMONCOLLECTION_SIM_NATIVE
#include <algorithm>
#include <cmath>
#endif

namespace {
class NullDisplayDevice : public lgfx::LGFX_Device {
};

NullDisplayDevice gNullDevice;
}  // namespace

#ifdef POKEMONCOLLECTION_SIM_NATIVE

#include <SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SD.h>

namespace {
lgfx::IFont gSimFont;
SDL_Color rgb565ToColor(uint16_t color) {
  const uint8_t r = static_cast<uint8_t>(((color >> 11) & 0x1F) * 255 / 31);
  const uint8_t g = static_cast<uint8_t>(((color >> 5) & 0x3F) * 255 / 63);
  const uint8_t b = static_cast<uint8_t>((color & 0x1F) * 255 / 31);
  return {r, g, b, 255};
}

void setDrawColor(SDL_Renderer* renderer, uint16_t color) {
  const SDL_Color c = rgb565ToColor(color);
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

void drawCircleOutline(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r) {
  int32_t x = r - 1;
  int32_t y = 0;
  int32_t dx = 1;
  int32_t dy = 1;
  int32_t err = dx - (r << 1);

  while (x >= y) {
    SDL_RenderDrawPoint(renderer, cx + x, cy + y);
    SDL_RenderDrawPoint(renderer, cx + y, cy + x);
    SDL_RenderDrawPoint(renderer, cx - y, cy + x);
    SDL_RenderDrawPoint(renderer, cx - x, cy + y);
    SDL_RenderDrawPoint(renderer, cx - x, cy - y);
    SDL_RenderDrawPoint(renderer, cx - y, cy - x);
    SDL_RenderDrawPoint(renderer, cx + y, cy - x);
    SDL_RenderDrawPoint(renderer, cx + x, cy - y);

    if (err <= 0) {
      ++y;
      err += dy;
      dy += 2;
    }
    if (err > 0) {
      --x;
      dx += 2;
      err += dx - (r << 1);
    }
  }
}

void fillCircleSolid(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r) {
  for (int32_t dy = -r; dy <= r; ++dy) {
    const int32_t dx = static_cast<int32_t>(std::sqrt((r * r) - (dy * dy)));
    SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

void drawQuarterCircleOutline(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r, int quadrant) {
  int32_t x = r - 1;
  int32_t y = 0;
  int32_t dx = 1;
  int32_t dy = 1;
  int32_t err = dx - (r << 1);

  auto drawPoint = [&](int32_t px, int32_t py) {
    SDL_RenderDrawPoint(renderer, px, py);
  };

  while (x >= y) {
    switch (quadrant) {
      case 0:  // top-left
        drawPoint(cx - x, cy - y);
        drawPoint(cx - y, cy - x);
        break;
      case 1:  // top-right
        drawPoint(cx + x, cy - y);
        drawPoint(cx + y, cy - x);
        break;
      case 2:  // bottom-left
        drawPoint(cx - x, cy + y);
        drawPoint(cx - y, cy + x);
        break;
      case 3:  // bottom-right
        drawPoint(cx + x, cy + y);
        drawPoint(cx + y, cy + x);
        break;
    }

    if (err <= 0) {
      ++y;
      err += dy;
      dy += 2;
    }
    if (err > 0) {
      --x;
      dx += 2;
      err += dx - (r << 1);
    }
  }
}

void fillQuarterCircle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r, int quadrant) {
  for (int32_t dy = 0; dy <= r; ++dy) {
    const int32_t dx = static_cast<int32_t>(std::sqrt((r * r) - (dy * dy)));
    switch (quadrant) {
      case 0:  // top-left
        SDL_RenderDrawLine(renderer, cx - dx, cy - dy, cx, cy - dy);
        break;
      case 1:  // top-right
        SDL_RenderDrawLine(renderer, cx, cy - dy, cx + dx, cy - dy);
        break;
      case 2:  // bottom-left
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx, cy + dy);
        break;
      case 3:  // bottom-right
        SDL_RenderDrawLine(renderer, cx, cy + dy, cx + dx, cy + dy);
        break;
    }
  }
}
}  // namespace

struct PcRenderer::Impl {
  SDL_Renderer* renderer = nullptr;
  TTF_Font* font = nullptr;
  const lgfx::IFont* logicalFont = &fonts::efontJA_12;
  SDL_Color textColor = {255, 255, 255, 255};
};

PcRenderer::PcRenderer() : impl(new Impl()) {
}

PcRenderer::~PcRenderer() {
  endNative();
  delete impl;
}

bool PcRenderer::beginNative(void* nativeRenderer, const char* fontPath, int pointSize) {
  endNative();
  impl->renderer = static_cast<SDL_Renderer*>(nativeRenderer);
  if (impl->renderer == nullptr || fontPath == nullptr) {
    return false;
  }
  impl->font = TTF_OpenFont(fontPath, pointSize);
  return impl->font != nullptr;
}

void PcRenderer::endNative() {
  if (impl != nullptr && impl->font != nullptr) {
    TTF_CloseFont(impl->font);
    impl->font = nullptr;
  }
  if (impl != nullptr) {
    impl->renderer = nullptr;
  }
}

lgfx::LGFX_Device& PcRenderer::device() {
  return *this;
}

void PcRenderer::fillScreen(uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  SDL_RenderClear(impl->renderer);
}

void PcRenderer::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  SDL_Rect rect = {x, y, w, h};
  SDL_RenderFillRect(impl->renderer, &rect);
}

void PcRenderer::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  SDL_Rect rect = {x, y, w, h};
  SDL_RenderDrawRect(impl->renderer, &rect);
}

void PcRenderer::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  if (radius <= 0) {
    fillRect(x, y, w, h, color);
    return;
  }
  const int32_t clamped = std::min(radius, std::min(w / 2, h / 2));
  fillRect(x + clamped, y, w - (clamped * 2), h, color);
  fillRect(x, y + clamped, clamped, h - (clamped * 2), color);
  fillRect(x + w - clamped, y + clamped, clamped, h - (clamped * 2), color);
  setDrawColor(impl->renderer, color);
  fillQuarterCircle(impl->renderer, x + clamped, y + clamped, clamped, 0);
  fillQuarterCircle(impl->renderer, x + w - clamped - 1, y + clamped, clamped, 1);
  fillQuarterCircle(impl->renderer, x + clamped, y + h - clamped - 1, clamped, 2);
  fillQuarterCircle(impl->renderer, x + w - clamped - 1, y + h - clamped - 1, clamped, 3);
}

void PcRenderer::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  if (radius <= 0) {
    drawRect(x, y, w, h, color);
    return;
  }
  const int32_t clamped = std::min(radius, std::min(w / 2, h / 2));
  setDrawColor(impl->renderer, color);
  SDL_RenderDrawLine(impl->renderer, x + clamped, y, x + w - clamped - 1, y);
  SDL_RenderDrawLine(impl->renderer, x + clamped, y + h - 1, x + w - clamped - 1, y + h - 1);
  SDL_RenderDrawLine(impl->renderer, x, y + clamped, x, y + h - clamped - 1);
  SDL_RenderDrawLine(impl->renderer, x + w - 1, y + clamped, x + w - 1, y + h - clamped - 1);
  drawQuarterCircleOutline(impl->renderer, x + clamped, y + clamped, clamped, 0);
  drawQuarterCircleOutline(impl->renderer, x + w - clamped - 1, y + clamped, clamped, 1);
  drawQuarterCircleOutline(impl->renderer, x + clamped, y + h - clamped - 1, clamped, 2);
  drawQuarterCircleOutline(impl->renderer, x + w - clamped - 1, y + h - clamped - 1, clamped, 3);
}

void PcRenderer::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)data;
}

void PcRenderer::setFont(const lgfx::IFont* font) {
  if (impl == nullptr) return;
  impl->logicalFont = font;
}

void PcRenderer::setTextColor(uint16_t color) {
  if (impl == nullptr) return;
  impl->textColor = rgb565ToColor(color);
}

const lgfx::IFont* PcRenderer::getFont() const {
  return impl != nullptr ? impl->logicalFont : &fonts::efontJA_12;
}

void PcRenderer::drawString(const String& text, int32_t x, int32_t y) {
  if (impl == nullptr || impl->renderer == nullptr || impl->font == nullptr || text.empty()) return;
  SDL_Surface* surface = TTF_RenderUTF8_Blended(impl->font, text.c_str(), impl->textColor);
  if (surface == nullptr) return;
  SDL_Texture* texture = SDL_CreateTextureFromSurface(impl->renderer, surface);
  SDL_Rect dst = {x, y, surface->w, surface->h};
  SDL_FreeSurface(surface);
  if (texture == nullptr) return;
  SDL_RenderCopy(impl->renderer, texture, nullptr, &dst);
  SDL_DestroyTexture(texture);
}

void PcRenderer::drawCenterString(const String& text, int32_t x, int32_t y) {
  if (impl == nullptr || impl->font == nullptr) return;
  int width = 0;
  int height = 0;
  if (TTF_SizeUTF8(impl->font, text.c_str(), &width, &height) != 0) return;
  drawString(text, x - (width / 2), y);
}

void PcRenderer::drawRightString(const String& text, int32_t x, int32_t y) {
  if (impl == nullptr || impl->font == nullptr) return;
  int width = 0;
  int height = 0;
  if (TTF_SizeUTF8(impl->font, text.c_str(), &width, &height) != 0) return;
  drawString(text, x - width, y);
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
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::setDisplaySleeping(false);
#endif
  if (impl == nullptr || impl->renderer == nullptr) return;
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);
  SDL_RenderPresent(impl->renderer);
}

void PcRenderer::sleep() {
#ifdef POKEMONCOLLECTION_SIM
  SimPlatform::setDisplaySleeping(true);
#endif
  if (impl == nullptr || impl->renderer == nullptr) return;
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);
  SDL_RenderPresent(impl->renderer);
}

void PcRenderer::drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  drawCircleOutline(impl->renderer, x, y, r);
}

void PcRenderer::fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  fillCircleSolid(impl->renderer, x, y, r);
}

void PcRenderer::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  if (impl == nullptr || impl->renderer == nullptr) return;
  setDrawColor(impl->renderer, color);
  SDL_RenderDrawLine(impl->renderer, x0, y0, x1, y1);
}

void PcRenderer::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
  drawLine(x, y, x + w, y, color);
}

void PcRenderer::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
  drawLine(x, y, x, y + h, color);
}

#else

struct PcRenderer::Impl {
};

PcRenderer::PcRenderer() : impl(nullptr) {
}

PcRenderer::~PcRenderer() = default;

bool PcRenderer::beginNative(void* nativeRenderer, const char* fontPath, int pointSize) {
  (void)nativeRenderer;
  (void)fontPath;
  (void)pointSize;
  return false;
}

void PcRenderer::endNative() {
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

#endif
