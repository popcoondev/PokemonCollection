#include "SimPlatform.h"

#include <cmath>
#include <cstdio>
#include <SDL_ttf.h>

namespace {
SimTouchState gTouchState;
SimControlState gControlState;
bool gDigitalButtonPressed = false;
SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;
bool gFrameDirty = false;
bool gRedrawRequested = false;
bool gDisplaySleeping = false;
bool gDebugHitboxEnabled = false;
uint32_t gDigitalButtonClickUntilMs = 0;

SDL_Color colorFrom565(uint16_t color) {
  SDL_Color out;
  out.r = static_cast<uint8_t>(((color >> 11) & 0x1F) * 255 / 31);
  out.g = static_cast<uint8_t>(((color >> 5) & 0x3F) * 255 / 63);
  out.b = static_cast<uint8_t>((color & 0x1F) * 255 / 31);
  out.a = 255;
  return out;
}

const char* pickFontPath() {
  static const char* kCandidates[] = {
      "/System/Library/Fonts/AppleSDGothicNeo.ttc",
      "/System/Library/Fonts/Helvetica.ttc",
  };
  for (const char* path : kCandidates) {
    if (path != nullptr) {
      if (FILE* fp = std::fopen(path, "rb")) {
        std::fclose(fp);
        return path;
      }
    }
  }
  return nullptr;
}

TTF_Font* ensureFont() {
  if (gFont != nullptr) {
    return gFont;
  }
  if (TTF_WasInit() == 0 && TTF_Init() != 0) {
    return nullptr;
  }
  const char* path = pickFontPath();
  if (path == nullptr) {
    return nullptr;
  }
  gFont = TTF_OpenFont(path, 14);
  return gFont;
}

void setRenderColor(uint16_t color) {
  if (gRenderer == nullptr) {
    return;
  }
  const SDL_Color c = colorFrom565(color);
  SDL_SetRenderDrawColor(gRenderer, c.r, c.g, c.b, c.a);
}
}

namespace SimPlatform {

void setRenderer(SDL_Renderer* renderer) {
  gRenderer = renderer;
}

SDL_Renderer* getRenderer() {
  return gRenderer;
}

bool beginRenderFrame() {
  if (gRenderer == nullptr) {
    return false;
  }
  SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
  gFrameDirty = false;
  if (gDisplaySleeping) {
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);
    gFrameDirty = true;
  }
  return true;
}

void endRenderFrame() {
  if (gRenderer != nullptr && gFrameDirty) {
    SDL_RenderPresent(gRenderer);
  }
}

void clear(uint16_t color) {
  if (gRenderer == nullptr) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  SDL_RenderClear(gRenderer);
}

void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (gRenderer == nullptr) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  const SDL_Rect rect = {x, y, w, h};
  SDL_RenderFillRect(gRenderer, &rect);
}

void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  if (gRenderer == nullptr) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  const SDL_Rect rect = {x, y, w, h};
  SDL_RenderDrawRect(gRenderer, &rect);
}

void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
  if (gRenderer == nullptr) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  SDL_RenderDrawLine(gRenderer, x1, y1, x2, y2);
}

void drawCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color) {
  if (gRenderer == nullptr || r <= 0) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  int32_t x = r - 1;
  int32_t y = 0;
  int32_t dx = 1;
  int32_t dy = 1;
  int32_t err = dx - (r << 1);
  while (x >= y) {
    SDL_RenderDrawPoint(gRenderer, cx + x, cy + y);
    SDL_RenderDrawPoint(gRenderer, cx + y, cy + x);
    SDL_RenderDrawPoint(gRenderer, cx - y, cy + x);
    SDL_RenderDrawPoint(gRenderer, cx - x, cy + y);
    SDL_RenderDrawPoint(gRenderer, cx - x, cy - y);
    SDL_RenderDrawPoint(gRenderer, cx - y, cy - x);
    SDL_RenderDrawPoint(gRenderer, cx + y, cy - x);
    SDL_RenderDrawPoint(gRenderer, cx + x, cy - y);
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

void fillCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color) {
  if (gRenderer == nullptr || r <= 0) {
    return;
  }
  gFrameDirty = true;
  setRenderColor(color);
  for (int32_t dy = -r; dy <= r; ++dy) {
    const int32_t dx = static_cast<int32_t>(std::sqrt((r * r) - (dy * dy)));
    SDL_RenderDrawLine(gRenderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

void drawText(const char* text, int32_t x, int32_t y, uint16_t color, int align) {
  if (gRenderer == nullptr || text == nullptr || *text == '\0') {
    return;
  }
  TTF_Font* font = ensureFont();
  if (font == nullptr) {
    return;
  }
  const SDL_Color c = colorFrom565(color);
  SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, c);
  if (surface == nullptr) {
    return;
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, surface);
  if (texture == nullptr) {
    SDL_FreeSurface(surface);
    return;
  }
  SDL_Rect dst = {x, y, surface->w, surface->h};
  if (align == 1) {
    dst.x -= surface->w / 2;
  } else if (align == 2) {
    dst.x -= surface->w;
  }
  SDL_RenderCopy(gRenderer, texture, nullptr, &dst);
  gFrameDirty = true;
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void setTouchState(const SimTouchState& state) {
  gTouchState = state;
}

SimTouchState getTouchState() {
  return gTouchState;
}

void setDigitalButtonPressed(bool pressed) {
  gControlState.portBHold = pressed;
}

bool isDigitalButtonPressed() {
  return gDigitalButtonPressed;
}

void triggerDigitalButtonClick() {
  gDigitalButtonClickUntilMs = SDL_GetTicks() + 150;
  gRedrawRequested = true;
}

void beginInputFrame() {
  gDigitalButtonPressed = gControlState.portBHold || (SDL_GetTicks() < gDigitalButtonClickUntilMs);
}

void endInputFrame() {
}

void setCharging(bool charging) {
  gControlState.charging = charging;
  gRedrawRequested = true;
}

bool isCharging() {
  return gControlState.charging;
}

void setBatteryLevel(int level) {
  if (level < 0) level = 0;
  if (level > 100) level = 100;
  gControlState.batteryLevel = level;
  gRedrawRequested = true;
}

int getBatteryLevel() {
  return gControlState.batteryLevel;
}

void setProximityValue(uint16_t value) {
  gControlState.proximityValue = value;
  gRedrawRequested = true;
}

uint16_t getProximityValue() {
  return gControlState.proximityValue;
}

void setControlState(const SimControlState& state) {
  gControlState = state;
  gRedrawRequested = true;
}

SimControlState getControlState() {
  return gControlState;
}

void requestRedraw() {
  gRedrawRequested = true;
}

bool consumeRedrawRequest() {
  const bool requested = gRedrawRequested;
  gRedrawRequested = false;
  return requested;
}

void setDisplaySleeping(bool sleeping) {
  gDisplaySleeping = sleeping;
  gRedrawRequested = true;
}

bool isDisplaySleeping() {
  return gDisplaySleeping;
}

void setDebugHitboxEnabled(bool enabled) {
  gDebugHitboxEnabled = enabled;
  gRedrawRequested = true;
}

bool isDebugHitboxEnabled() {
  return gDebugHitboxEnabled;
}

}
