#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdlib>
#include <cstdio>

#include "AppRuntime.h"
#include "SimPlatform.h"

namespace {
constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 2;
constexpr int kDebugWindowWidth = 320;
constexpr int kDebugWindowHeight = 140;

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

const char* pickDebugFontPath() {
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
}

int main(int argc, char** argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    return 1;
  }
  if (TTF_Init() != 0) {
    SDL_Quit();
    return 1;
  }
  if (!SimSd::begin(argc > 0 ? argv[0] : nullptr)) {
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  DataManager dataMgr;
  if (!dataMgr.begin() || !dataMgr.loadPokemonDetail(1)) {
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
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_RenderSetLogicalSize(renderer, kLogicalWidth, kLogicalHeight);
  SimPlatform::setRenderer(renderer);

  SDL_Window* debugWindow = nullptr;
  SDL_Renderer* debugRenderer = nullptr;
  TTF_Font* debugFont = nullptr;
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
      if (const char* fontPath = pickDebugFontPath()) {
        debugFont = TTF_OpenFont(fontPath, 14);
      }
    }
  }

  appBoot();

  PcRenderer pcRenderer;
  if (!pcRenderer.beginNative(renderer, kUiFontPath, 14)) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  UIController ui;
  ui.setRenderer(&pcRenderer);
  if (!ui.begin()) {
    pcRenderer.endNative();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  bool running = true;
  bool mouseDown = false;
  bool mouseClicked = false;
  int16_t mouseX = 0;
  int16_t mouseY = 0;
  int windowMouseX = 0;
  int windowMouseY = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        mouseDown = true;
      } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        mouseDown = false;
        mouseClicked = true;
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
    SimPlatform::setDigitalButtonPressed(false);
    if (SimPlatform::beginRenderFrame()) {
      appTick();
      SimPlatform::endRenderFrame();
    }

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
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  if (TTF_WasInit()) {
    TTF_Quit();
  }
  SDL_Quit();
  return 0;
}
