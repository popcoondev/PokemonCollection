#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <Arduino.h>
#include <M5Unified.h>
#include <FS.h>
#include "Config.h"

// ========== ImageLoader Class ==========

class ImageLoader {
public:
  ImageLoader();
  ~ImageLoader();

  // Initialize image loader
  bool begin();

  // Load and display PNG with aspect ratio preservation
  // Returns true if successful, false if image not found or decode failed
  bool loadAndDisplayPNG(lgfx::LGFXBase& target, uint16_t id, int16_t x, int16_t y,
                         uint16_t maxW, uint16_t maxH);

  // Draw placeholder when image unavailable
  void drawPlaceholder(lgfx::LGFXBase& target, int16_t x, int16_t y, uint16_t w, uint16_t h);

private:
  // Helper functions
  bool openPNGFile(uint16_t id, char* filename, uint16_t filenameSize);
};

#endif // IMAGE_LOADER_H
