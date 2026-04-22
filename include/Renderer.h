#ifndef RENDERER_H
#define RENDERER_H

#include <M5Unified.h>

class Renderer {
public:
  virtual ~Renderer() = default;

  virtual lgfx::LGFX_Device& device() = 0;
  virtual void setFont(const lgfx::IFont* font) = 0;
  virtual void setTextColor(uint16_t color) = 0;
  virtual void drawString(const String& text, int32_t x, int32_t y) = 0;
  virtual void drawCenterString(const String& text, int32_t x, int32_t y) = 0;
  virtual void drawRightString(const String& text, int32_t x, int32_t y) = 0;
  virtual void setRotation(uint8_t rotation) = 0;
  virtual void setTextFont(int font) = 0;
  virtual void setBrightness(uint8_t brightness) = 0;
  virtual void print(const char* text) = 0;
  virtual void wakeup() = 0;
  virtual void sleep() = 0;
};

class M5Renderer : public Renderer {
public:
  static M5Renderer& instance();

  lgfx::LGFX_Device& device() override;
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
  M5Renderer() = default;
};

#endif
