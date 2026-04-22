#ifndef SIM_M5UNIFIED_H
#define SIM_M5UNIFIED_H

#include "Arduino.h"

#include <cstdint>

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

class SDClass;

template <typename T, typename U, typename V>
constexpr auto constrain(T x, U a, V b) {
  return x < a ? a : (x > b ? b : x);
}

namespace lgfx {
struct FontMetrics {
  int x_advance = 8;
};

class IFont {
public:
  virtual ~IFont() = default;
  virtual bool updateFontMetric(FontMetrics* metrics, uint16_t codepoint) const {
    (void)codepoint;
    if (metrics != nullptr) {
      metrics->x_advance = 8;
    }
    return true;
  }
};

class LGFX_Device {
public:
  virtual ~LGFX_Device() = default;
};

class LGFXBase : public LGFX_Device {
public:
  virtual ~LGFXBase() = default;

  virtual void fillScreen(uint16_t color) = 0;
  virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) = 0;
  virtual void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) = 0;
  virtual void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) = 0;
  virtual void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) = 0;
  virtual void setFont(const lgfx::IFont* font) = 0;
  virtual const lgfx::IFont* getFont() const = 0;
  virtual void setTextColor(uint16_t color) = 0;
  virtual void drawString(const String& text, int32_t x, int32_t y) = 0;
  virtual void drawCenterString(const String& text, int32_t x, int32_t y) = 0;
  virtual void drawRightString(const String& text, int32_t x, int32_t y) = 0;
  virtual size_t textWidth(const String& text) const = 0;
  virtual bool drawPngFile(const ::SDClass& fs,
                           const char* path,
                           int32_t x,
                           int32_t y,
                           int32_t sx,
                           int32_t sy,
                           int32_t sw,
                           int32_t sh,
                           float scaleX,
                           float scaleY) = 0;
  virtual void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) = 0;
  virtual void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) = 0;
  virtual void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) = 0;
  virtual void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) = 0;
  virtual void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) = 0;
  virtual void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) = 0;
};

namespace v1 {
constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
}  // namespace v1
}  // namespace lgfx

class LGFX_Sprite : public lgfx::LGFXBase {
public:
  explicit LGFX_Sprite(lgfx::LGFX_Device* target = nullptr);
  ~LGFX_Sprite() override;

  bool createSprite(int32_t width, int32_t height);
  void deleteSprite();
  void setColorDepth(int depth);
  void setPsram(bool enabled);
  void* getBuffer() const;
  int32_t width() const;
  int32_t height() const;
  void pushSprite(int32_t x, int32_t y, uint16_t transparent = 0);
  void pushSprite(LGFX_Sprite* target, int32_t x, int32_t y, uint16_t transparent = 0);
  void pushSprite(lgfx::LGFX_Device* target, int32_t x, int32_t y, uint16_t transparent = 0);

  void fillScreen(uint16_t color) override;
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void setFont(const lgfx::IFont* font) override;
  const lgfx::IFont* getFont() const override;
  void setTextColor(uint16_t color) override;
  void drawString(const String& text, int32_t x, int32_t y) override;
  void drawCenterString(const String& text, int32_t x, int32_t y) override;
  void drawRightString(const String& text, int32_t x, int32_t y) override;
  size_t textWidth(const String& text) const override;
  bool drawPngFile(const ::SDClass& fs,
                   const char* path,
                   int32_t x,
                   int32_t y,
                   int32_t sx,
                   int32_t sy,
                   int32_t sw,
                   int32_t sh,
                   float scaleX,
                   float scaleY) override;
  bool drawPngFile(const ::SDClass& fs, const char* path, int32_t x, int32_t y, int32_t maxWidth, int32_t maxHeight);
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) override;
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override;
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) override;
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) override;
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) override;
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) override;

private:
  lgfx::LGFXBase* target;
  const lgfx::IFont* currentFont;
  bool created;
  mutable uint8_t bufferStub;
  int32_t spriteWidth;
  int32_t spriteHeight;
};

namespace fonts {
extern lgfx::IFont Font0;
extern lgfx::IFont efontJA_10;
extern lgfx::IFont efontJA_10_b;
extern lgfx::IFont efontJA_12;
extern lgfx::IFont efontJA_12_b;
extern lgfx::IFont efontJA_16;
extern lgfx::IFont efontJA_16_b;
extern lgfx::IFont efontCN_10;
extern lgfx::IFont efontCN_10_b;
extern lgfx::IFont efontCN_12;
extern lgfx::IFont efontCN_12_b;
extern lgfx::IFont efontCN_16;
extern lgfx::IFont efontCN_16_b;
}  // namespace fonts

#endif
