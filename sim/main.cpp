#include <SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Config.h"
#include "DataManager.h"
#include "ImageLoader.h"
#include "PcRenderer.h"
#include "SimSd.h"
#include "UIController.h"

namespace {
constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 2;
constexpr const char* kUiFontPath = "/System/Library/Fonts/AppleSDGothicNeo.ttc";
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
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    ui.drawBase();
    ui.drawHeader(dataMgr.getCurrentPokemon(), false);
    ui.drawTabBar(TAB_DESCRIPTION, -1);
    ui.drawDescriptionTab(dataMgr.getCurrentPokemon());
    ui.pushToDisplay();

    SDL_RenderPresent(renderer);
  }

  pcRenderer.endNative();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  return 0;
}
