#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cstdlib>
#include <cstdio>

#include "AppRuntime.h"
#include "PcRenderer.h"
#include "Renderer.h"
#include "SimPlatform.h"
#include "SimSd.h"

namespace {
constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 2;
constexpr int kDebugWindowWidth = 320;
constexpr int kDebugWindowHeight = 140;
constexpr int kControlWindowWidth = 360;
constexpr int kControlWindowHeight = 260;

bool simDebugHitboxEnabled() {
  const char* value = std::getenv("SIM_DEBUG_HITBOX");
  return value != nullptr && value[0] != '\0' && value[0] != '0';
}

void mapWindowToLogical(SDL_Window* window, int windowX, int windowY, int16_t& logicalX, int16_t& logicalY) {
  int windowW = 0;
  int windowH = 0;
  SDL_GetWindowSize(window, &windowW, &windowH);
  if (windowW <= 0 || windowH <= 0) {
    logicalX = 0;
    logicalY = 0;
    return;
  }
  logicalX = static_cast<int16_t>((windowX * kLogicalWidth) / windowW);
  logicalY = static_cast<int16_t>((windowY * kLogicalHeight) / windowH);
}

void drawDebugText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
  if (renderer == nullptr || font == nullptr || text == nullptr || text[0] == '\0') {
    return;
  }
  SDL_Color color = {240, 244, 248, 255};
  SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
  if (surface == nullptr) {
    return;
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (texture == nullptr) {
    SDL_FreeSurface(surface);
    return;
  }
  SDL_Rect dst = {x, y, surface->w, surface->h};
  SDL_RenderCopy(renderer, texture, nullptr, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

struct ControlButton {
  SDL_Rect rect;
  const char* label;
};

bool hitRect(const SDL_Rect& rect, int x, int y) {
  return x >= rect.x && x < (rect.x + rect.w) && y >= rect.y && y < (rect.y + rect.h);
}

void drawControlButton(SDL_Renderer* renderer, TTF_Font* font, const ControlButton& button, bool active) {
  SDL_SetRenderDrawColor(renderer, active ? 48 : 28, active ? 120 : 58, active ? 84 : 62, 255);
  SDL_RenderFillRect(renderer, &button.rect);
  SDL_SetRenderDrawColor(renderer, 210, 220, 228, 255);
  SDL_RenderDrawRect(renderer, &button.rect);
  drawDebugText(renderer, font, button.label, button.rect.x + 12, button.rect.y + 10);
}
}

int main(int argc, char** argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    return 1;
  }
  if (TTF_Init() != 0) {
    SDL_Quit();
    return 1;
  }
  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
    TTF_Quit();
    SDL_Quit();
    return 1;
  }
  if (!SimSd::begin((argc > 0) ? argv[0] : nullptr)) {
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
      "PokemonCollection Simulator",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      kLogicalWidth * kWindowScale,
      kLogicalHeight * kWindowScale,
      SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_RenderSetLogicalSize(renderer, kLogicalWidth, kLogicalHeight);
  SimPlatform::setRenderer(renderer);
  if (const char* fontPath = SimPlatform::pickUIFontPath()) {
    reinterpret_cast<PcRenderer*>(&M5Renderer::instance())->beginNative(renderer, fontPath, 14);
  }

  SDL_Window* debugWindow = nullptr;
  SDL_Renderer* debugRenderer = nullptr;
  TTF_Font* debugFont = nullptr;
  SDL_Window* controlWindow = nullptr;
  SDL_Renderer* controlRenderer = nullptr;
  TTF_Font* controlFont = nullptr;
  if (simDebugHitboxEnabled()) {
    debugWindow = SDL_CreateWindow(
        "PokemonCollection Sim Debug",
        SDL_WINDOWPOS_CENTERED + 40,
        SDL_WINDOWPOS_CENTERED + 40,
        kDebugWindowWidth,
        kDebugWindowHeight,
        SDL_WINDOW_SHOWN);
    if (debugWindow != nullptr) {
      debugRenderer = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }
    if (debugRenderer != nullptr) {
      if (TTF_WasInit() == 0) {
        TTF_Init();
      }
      if (const char* fontPath = SimPlatform::pickUIFontPath()) {
        debugFont = TTF_OpenFont(fontPath, 14);
      }
    }
  }

  controlWindow = SDL_CreateWindow(
      "PokemonCollection Sim Controls",
      SDL_WINDOWPOS_CENTERED + 80,
      SDL_WINDOWPOS_CENTERED + 80,
      kControlWindowWidth,
      kControlWindowHeight,
      SDL_WINDOW_SHOWN);
  if (controlWindow != nullptr) {
    controlRenderer = SDL_CreateRenderer(controlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  }
  if (controlRenderer != nullptr) {
    if (TTF_WasInit() == 0) {
      TTF_Init();
    }
    if (const char* fontPath = SimPlatform::pickUIFontPath()) {
      controlFont = TTF_OpenFont(fontPath, 14);
    }
  }

  SimControlState controlState;
  controlState.proximityValue = 0;
  controlState.batteryLevel = 100;
  SimPlatform::setControlState(controlState);
  SimPlatform::setDebugHitboxEnabled(simDebugHitboxEnabled());

  appBoot();

  bool running = true;
  bool mouseDown = false;
  bool mouseClicked = false;
  int16_t mouseX = 0;
  int16_t mouseY = 0;
  int windowMouseX = 0;
  int windowMouseY = 0;
  const uint32_t mainWindowId = SDL_GetWindowID(window);
  const uint32_t debugWindowId = debugWindow != nullptr ? SDL_GetWindowID(debugWindow) : 0;
  const uint32_t controlWindowId = controlWindow != nullptr ? SDL_GetWindowID(controlWindow) : 0;
  const ControlButton nearButton{{16, 34, 76, 32}, "Near"};
  const ControlButton farButton{{100, 34, 76, 32}, "Far"};
  const ControlButton clickButton{{16, 88, 98, 32}, "Port.B Click"};
  const ControlButton holdButton{{124, 88, 98, 32}, "Port.B Hold"};
  const ControlButton chargeButton{{16, 142, 116, 32}, "Charging"};
  const ControlButton batteryDownButton{{156, 142, 56, 32}, "Batt-"};
  const ControlButton batteryUpButton{{220, 142, 56, 32}, "Batt+"};
  const ControlButton hitboxButton{{16, 196, 150, 32}, "Debug Hitbox"};
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        running = false;
      } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (event.button.windowID == mainWindowId) {
          mouseDown = true;
        }
      } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (event.button.windowID == mainWindowId) {
          mouseDown = false;
          mouseClicked = true;
        } else if (event.button.windowID == controlWindowId) {
          const int x = event.button.x;
          const int y = event.button.y;
          controlState = SimPlatform::getControlState();
          if (hitRect(nearButton.rect, x, y)) {
            controlState.proximityValue = 1000;
          } else if (hitRect(farButton.rect, x, y)) {
            controlState.proximityValue = 0;
          } else if (hitRect(clickButton.rect, x, y)) {
            SimPlatform::triggerDigitalButtonClick();
          } else if (hitRect(holdButton.rect, x, y)) {
            controlState.portBHold = !controlState.portBHold;
          } else if (hitRect(chargeButton.rect, x, y)) {
            controlState.charging = !controlState.charging;
          } else if (hitRect(batteryDownButton.rect, x, y)) {
            controlState.batteryLevel = (controlState.batteryLevel >= 10) ? (controlState.batteryLevel - 10) : 0;
          } else if (hitRect(batteryUpButton.rect, x, y)) {
            controlState.batteryLevel = (controlState.batteryLevel <= 90) ? (controlState.batteryLevel + 10) : 100;
          } else if (hitRect(hitboxButton.rect, x, y)) {
            SimPlatform::setDebugHitboxEnabled(!SimPlatform::isDebugHitboxEnabled());
          }
          SimPlatform::setControlState(controlState);
        }
      }
    }
    SDL_GetMouseState(&windowMouseX, &windowMouseY);
    mapWindowToLogical(window, windowMouseX, windowMouseY, mouseX, mouseY);
    SimTouchState touchState;
    touchState.active = mouseDown || mouseClicked;
    touchState.clicked = mouseClicked;
    touchState.x = mouseX;
    touchState.y = mouseY;
    SimPlatform::setTouchState(touchState);
    mouseClicked = false;
    SimPlatform::beginInputFrame();
    if (SimPlatform::beginRenderFrame()) {
      appTick();
      SimPlatform::endRenderFrame();
    }
    SimPlatform::endInputFrame();

    if (debugRenderer != nullptr) {
      SDL_SetRenderDrawColor(debugRenderer, 18, 24, 32, 255);
      SDL_RenderClear(debugRenderer);
      SDL_SetRenderDrawColor(debugRenderer, 82, 92, 108, 255);
      SDL_Rect border = {8, 8, kDebugWindowWidth - 16, kDebugWindowHeight - 16};
      SDL_RenderDrawRect(debugRenderer, &border);

      char line0[96];
      char line1[96];
      char line2[96];
      std::snprintf(line0, sizeof(line0), "Mouse window : x=%d y=%d", windowMouseX, windowMouseY);
      std::snprintf(line1, sizeof(line1), "Mouse logical: x=%d y=%d", static_cast<int>(mouseX), static_cast<int>(mouseY));
      std::snprintf(line2, sizeof(line2), "Touch active=%d clicked=%d down=%d", touchState.active ? 1 : 0, touchState.clicked ? 1 : 0, mouseDown ? 1 : 0);
      drawDebugText(debugRenderer, debugFont, "SIM DEBUG HITBOX", 16, 16);
      drawDebugText(debugRenderer, debugFont, line0, 16, 46);
      drawDebugText(debugRenderer, debugFont, line1, 16, 70);
      drawDebugText(debugRenderer, debugFont, line2, 16, 94);
      SDL_RenderPresent(debugRenderer);
    }

    if (controlRenderer != nullptr) {
      controlState = SimPlatform::getControlState();
      SDL_SetRenderDrawColor(controlRenderer, 18, 24, 32, 255);
      SDL_RenderClear(controlRenderer);
      SDL_SetRenderDrawColor(controlRenderer, 82, 92, 108, 255);
      SDL_Rect border = {8, 8, kControlWindowWidth - 16, kControlWindowHeight - 16};
      SDL_RenderDrawRect(controlRenderer, &border);
      drawDebugText(controlRenderer, controlFont, "SIM CONTROL WINDOW", 16, 12);
      drawDebugText(controlRenderer, controlFont, "Proximity", 200, 18);
      drawControlButton(controlRenderer, controlFont, nearButton, controlState.proximityValue >= 500);
      drawControlButton(controlRenderer, controlFont, farButton, controlState.proximityValue < 500);
      drawDebugText(controlRenderer, controlFont, "Port.B", 16, 72);
      drawControlButton(controlRenderer, controlFont, clickButton, false);
      drawControlButton(controlRenderer, controlFont, holdButton, controlState.portBHold);
      drawDebugText(controlRenderer, controlFont, "Power", 16, 126);
      drawControlButton(controlRenderer, controlFont, chargeButton, controlState.charging);
      drawControlButton(controlRenderer, controlFont, batteryDownButton, false);
      drawControlButton(controlRenderer, controlFont, batteryUpButton, false);
      drawControlButton(controlRenderer, controlFont, hitboxButton, SimPlatform::isDebugHitboxEnabled());
      char line[96];
      std::snprintf(line, sizeof(line), "Battery: %d%%", controlState.batteryLevel);
      drawDebugText(controlRenderer, controlFont, line, 286, 150);
      std::snprintf(line, sizeof(line), "Port.B active: %d", SimPlatform::isDigitalButtonPressed() ? 1 : 0);
      drawDebugText(controlRenderer, controlFont, line, 16, 186);
      std::snprintf(line, sizeof(line), "Proximity: %u", static_cast<unsigned>(controlState.proximityValue));
      drawDebugText(controlRenderer, controlFont, line, 176, 202);
      std::snprintf(line, sizeof(line), "Display sleep: %d", SimPlatform::isDisplaySleeping() ? 1 : 0);
      drawDebugText(controlRenderer, controlFont, line, 176, 226);
      SDL_RenderPresent(controlRenderer);
    }
  }

  if (debugFont != nullptr) {
    TTF_CloseFont(debugFont);
  }
  if (debugRenderer != nullptr) {
    SDL_DestroyRenderer(debugRenderer);
  }
  if (debugWindow != nullptr) {
    SDL_DestroyWindow(debugWindow);
  }
  if (controlFont != nullptr) {
    TTF_CloseFont(controlFont);
  }
  if (controlRenderer != nullptr) {
    SDL_DestroyRenderer(controlRenderer);
  }
  if (controlWindow != nullptr) {
    SDL_DestroyWindow(controlWindow);
  }
  reinterpret_cast<PcRenderer*>(&M5Renderer::instance())->endNative();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  if (TTF_WasInit()) {
    TTF_Quit();
  }
  IMG_Quit();
  SDL_Quit();
  return 0;
}
