#ifndef SIM_PLATFORM_H
#define SIM_PLATFORM_H

#include <cstdint>
#include <SDL.h>

struct SimTouchState {
  bool active = false;
  bool clicked = false;
  int16_t x = 0;
  int16_t y = 0;
};

namespace SimPlatform {

void setRenderer(SDL_Renderer* renderer);
SDL_Renderer* getRenderer();
bool beginRenderFrame();
void endRenderFrame();
void clear(uint16_t color);
void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);
void drawCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color);
void fillCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color);
void drawText(const char* text, int32_t x, int32_t y, uint16_t color, int align);

void setTouchState(const SimTouchState& state);
SimTouchState getTouchState();

void setDigitalButtonPressed(bool pressed);
bool isDigitalButtonPressed();

}

#endif
