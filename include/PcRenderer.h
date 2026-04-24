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

private:
  struct Impl;
  Impl* impl = nullptr;
  uint16_t currentTextColor_ = 0xFFFF;
};

#endif
