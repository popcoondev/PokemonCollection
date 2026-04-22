#ifndef PC_RENDERER_H
#define PC_RENDERER_H

#include "Renderer.h"

class PcRenderer : public Renderer, public lgfx::LGFXBase {
public:
  PcRenderer();
  ~PcRenderer() override;

  bool beginNative(void* nativeRenderer, const char* fontPath, int pointSize = 14);
  void endNative();

  lgfx::LGFX_Device& device() override;
  void fillScreen(uint16_t color) override;
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) override;
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color) override;
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) override;
  void setFont(const lgfx::IFont* font) override;
  void setTextColor(uint16_t color) override;
  void drawString(const String& text, int32_t x, int32_t y) override;
  void drawCenterString(const String& text, int32_t x, int32_t y) override;
  void drawRightString(const String& text, int32_t x, int32_t y) override;
  void setRotation(uint8_t rotation) override;
  void setTextFont(int font) override;
  void setBrightness(uint8_t brightness) override;
  void print(const char* text) override;
  void wakeup() override;
  void sleep() override;
#ifdef POKEMONCOLLECTION_SIM_NATIVE
  bool drawPngFile(const SDClass& fs,
                   const char* path,
                   int32_t x,
                   int32_t y,
                   int32_t sx,
                   int32_t sy,
                   int32_t sw,
                   int32_t sh,
                   float scaleX,
                   float scaleY) override;
#endif
#ifdef POKEMONCOLLECTION_SIM_NATIVE
  const lgfx::IFont* getFont() const override;
  size_t textWidth(const String& text) const override;
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override;
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override;
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) override;
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) override;
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) override;
#endif

private:
  struct Impl;
  Impl* impl;
};

#endif
