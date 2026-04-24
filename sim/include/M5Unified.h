#ifndef SIM_M5_UNIFIED_H
#define SIM_M5_UNIFIED_H

#include <Arduino.h>
#include <SD.h>
#include <SimPlatform.h>
#include <cstdint>

using gpio_num_t = int;
inline constexpr gpio_num_t GPIO_NUM_4 = 4;
inline constexpr gpio_num_t GPIO_NUM_8 = 8;
inline constexpr gpio_num_t GPIO_NUM_9 = 9;
inline constexpr uint16_t TFT_BLACK = 0x0000;
inline constexpr uint16_t TFT_WHITE = 0xFFFF;

namespace lgfx {

struct FontMetrics {
};

struct IFont {
  bool updateFontMetric(FontMetrics*, uint16_t) const { return true; }
};

class LGFXBase {
public:
  virtual ~LGFXBase() = default;

  virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { SimPlatform::fillRect(x, y, w, h, color); }
  virtual void fillScreen(uint16_t color) { SimPlatform::clear(color); }
  virtual void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color) { SimPlatform::fillRect(x, y, w, h, color); }
  virtual void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { SimPlatform::drawRect(x, y, w, h, color); }
  virtual void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color) { SimPlatform::drawRect(x, y, w, h, color); }
  virtual void setFont(const IFont* font) { font_ = font; }
  virtual const IFont* getFont() const { return font_; }
  virtual void setTextColor(uint16_t color) { textColor_ = color; }
  virtual void drawString(const char* text, int32_t x, int32_t y) { SimPlatform::drawText(text, x, y, textColor_, 0); }
  virtual void drawString(const String& text, int32_t x, int32_t y) { drawString(text.c_str(), x, y); }
  virtual void drawCenterString(const char* text, int32_t x, int32_t y) { SimPlatform::drawText(text, x, y, textColor_, 1); }
  virtual void drawCenterString(const String& text, int32_t x, int32_t y) { drawCenterString(text.c_str(), x, y); }
  virtual void drawRightString(const char* text, int32_t x, int32_t y) { SimPlatform::drawText(text, x, y, textColor_, 2); }
  virtual void drawRightString(const String& text, int32_t x, int32_t y) { drawRightString(text.c_str(), x, y); }
  virtual void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) { SimPlatform::drawLine(x, y, x + w - 1, y, color); }
  virtual void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) { SimPlatform::drawLine(x, y, x, y + h - 1, color); }
  virtual void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { SimPlatform::drawCircle(x, y, r, color); }
  virtual void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { SimPlatform::fillCircle(x, y, r, color); }
  virtual void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) { SimPlatform::drawLine(x1, y1, x2, y2, color); }
  virtual void pushImage(int32_t, int32_t, int32_t, int32_t, const uint16_t*) {}
  virtual int32_t textWidth(const char* text) { return SimPlatform::measureTextWidth(text); }
  virtual int32_t textWidth(const String& text) { return textWidth(text.c_str()); }
  virtual bool drawPngFile(const class SimSDClass& sd, const char* path, int32_t x, int32_t y, int32_t, int32_t, int32_t, int32_t, float scaleX, float scaleY) {
    const auto resolved = sd.resolvePath(path);
    return SimPlatform::drawPng(resolved.empty() ? nullptr : resolved.string().c_str(), x, y, scaleX, scaleY);
  }
  virtual bool drawPngFile(const class SimSDClass& sd, const char* path, int32_t x, int32_t y, int32_t maxWidth, int32_t maxHeight) {
    return drawPngFile(sd, path, x, y, maxWidth, maxHeight, 0, 0, 1.0f, 1.0f);
  }

protected:
  const IFont* font_ = nullptr;
  uint16_t textColor_ = TFT_WHITE;
};

class LGFX_Device : public LGFXBase {
};

class LGFX_Sprite : public LGFXBase {
public:
  explicit LGFX_Sprite(LGFX_Device* nextTarget = nullptr)
      : target_(dynamic_cast<lgfx::LGFXBase*>(nextTarget)) {}

  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override {
    if (target_ != nullptr) target_->fillRect(x, y, w, h, color);
    else LGFXBase::fillRect(x, y, w, h, color);
  }
  void fillScreen(uint16_t color) override {
    if (target_ != nullptr) target_->fillScreen(color);
    else LGFXBase::fillScreen(color);
  }
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override {
    if (target_ != nullptr) target_->fillRoundRect(x, y, w, h, r, color);
    else LGFXBase::fillRoundRect(x, y, w, h, r, color);
  }
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override {
    if (target_ != nullptr) target_->drawRect(x, y, w, h, color);
    else LGFXBase::drawRect(x, y, w, h, color);
  }
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override {
    if (target_ != nullptr) target_->drawRoundRect(x, y, w, h, r, color);
    else LGFXBase::drawRoundRect(x, y, w, h, r, color);
  }
  void setFont(const IFont* font) override {
    LGFXBase::setFont(font);
    if (target_ != nullptr) target_->setFont(font);
  }
  const IFont* getFont() const override { return LGFXBase::getFont(); }
  void setTextColor(uint16_t color) override {
    LGFXBase::setTextColor(color);
    if (target_ != nullptr) target_->setTextColor(color);
  }
  void drawString(const char* text, int32_t x, int32_t y) override {
    if (target_ != nullptr) target_->drawString(text, x, y);
    else LGFXBase::drawString(text, x, y);
  }
  void drawString(const String& text, int32_t x, int32_t y) override { drawString(text.c_str(), x, y); }
  void drawCenterString(const char* text, int32_t x, int32_t y) override {
    if (target_ != nullptr) target_->drawCenterString(text, x, y);
    else LGFXBase::drawCenterString(text, x, y);
  }
  void drawCenterString(const String& text, int32_t x, int32_t y) override { drawCenterString(text.c_str(), x, y); }
  void drawRightString(const char* text, int32_t x, int32_t y) override {
    if (target_ != nullptr) target_->drawRightString(text, x, y);
    else LGFXBase::drawRightString(text, x, y);
  }
  void drawRightString(const String& text, int32_t x, int32_t y) override { drawRightString(text.c_str(), x, y); }
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) override {
    if (target_ != nullptr) target_->drawFastHLine(x, y, w, color);
    else LGFXBase::drawFastHLine(x, y, w, color);
  }
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) override {
    if (target_ != nullptr) target_->drawFastVLine(x, y, h, color);
    else LGFXBase::drawFastVLine(x, y, h, color);
  }
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override {
    if (target_ != nullptr) target_->drawCircle(x, y, r, color);
    else LGFXBase::drawCircle(x, y, r, color);
  }
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override {
    if (target_ != nullptr) target_->fillCircle(x, y, r, color);
    else LGFXBase::fillCircle(x, y, r, color);
  }
  void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) override {
    if (target_ != nullptr) target_->drawLine(x1, y1, x2, y2, color);
    else LGFXBase::drawLine(x1, y1, x2, y2, color);
  }
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) override {
    if (target_ != nullptr) target_->pushImage(x, y, w, h, data);
    else LGFXBase::pushImage(x, y, w, h, data);
  }
  int32_t textWidth(const char* text) override {
    if (target_ != nullptr) return target_->textWidth(text);
    return LGFXBase::textWidth(text);
  }
  int32_t textWidth(const String& text) override { return textWidth(text.c_str()); }
  void setColorDepth(int) {}
  void setPsram(bool) {}
  bool createSprite(int32_t w, int32_t h) { width_ = w; height_ = h; return true; }
  void deleteSprite() { width_ = 0; height_ = 0; }
  int32_t width() const { return width_; }
  int32_t height() const { return height_; }
  void* getBuffer() { return nullptr; }
  void pushSprite(int32_t, int32_t) {}
  void pushSprite(LGFX_Device* nextTarget, int32_t, int32_t) {
    target_ = dynamic_cast<lgfx::LGFXBase*>(nextTarget);
  }
  void pushSprite(LGFXBase* nextTarget, int32_t, int32_t, uint16_t = 0) {
    target_ = nextTarget;
  }

  bool drawPngFile(const class SimSDClass& sd,
                   const char* path,
                   int32_t x,
                   int32_t y,
                   int32_t sx,
                   int32_t sy,
                   int32_t sw,
                   int32_t sh,
                   float scaleX,
                   float scaleY) override {
    if (target_ != nullptr) {
      return target_->drawPngFile(sd, path, x, y, sx, sy, sw, sh, scaleX, scaleY);
    }
    return LGFXBase::drawPngFile(sd, path, x, y, sx, sy, sw, sh, scaleX, scaleY);
  }
  bool drawPngFile(const class SimSDClass& sd,
                   const char* path,
                   int32_t x,
                   int32_t y,
                   int32_t maxWidth,
                   int32_t maxHeight) override {
    return drawPngFile(sd, path, x, y, 0, 0, maxWidth, maxHeight, 1.0f, 1.0f);
  }

private:
  lgfx::LGFXBase* target_ = nullptr;
  int32_t width_ = 0;
  int32_t height_ = 0;
};

namespace v1 {
inline uint16_t color565(uint8_t, uint8_t, uint8_t) { return 0; }
}

}  // namespace lgfx

using LGFX_Sprite = lgfx::LGFX_Sprite;

namespace fonts {
inline constexpr lgfx::IFont Font0{};
inline constexpr lgfx::IFont efontJA_10{};
inline constexpr lgfx::IFont efontJA_10_b{};
inline constexpr lgfx::IFont efontJA_12{};
inline constexpr lgfx::IFont efontJA_12_b{};
inline constexpr lgfx::IFont efontJA_16{};
inline constexpr lgfx::IFont efontJA_16_b{};
inline constexpr lgfx::IFont efontCN_10{};
inline constexpr lgfx::IFont efontCN_10_b{};
inline constexpr lgfx::IFont efontCN_12{};
inline constexpr lgfx::IFont efontCN_12_b{};
inline constexpr lgfx::IFont efontCN_16{};
inline constexpr lgfx::IFont efontCN_16_b{};
}

#endif
