#include <SDL.h>

namespace {
constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 2;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_RenderSetLogicalSize(renderer, kLogicalWidth, kLogicalHeight);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 18, 24, 32, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 235, 239, 244, 255);
    SDL_Rect frame = {12, 12, kLogicalWidth - 24, kLogicalHeight - 24};
    SDL_RenderDrawRect(renderer, &frame);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
