#include "ImageLoader.h"
#include <FS.h>
#include <SD.h>

// ========== Constructor & Destructor ==========

ImageLoader::ImageLoader() {
}

ImageLoader::~ImageLoader() {
}

// ========== Public Methods ==========

bool ImageLoader::begin() {
  return true;
}

bool ImageLoader::loadAndDisplayPNG(lgfx::LGFXBase& target, uint16_t id, int16_t x, int16_t y,
                                    uint16_t maxW, uint16_t maxH) {
  char filename[64];
  if (!openPNGFile(id, filename, sizeof(filename))) {
    drawPlaceholder(target, x, y, maxW, maxH);
    return false;
  }

  bool success = target.drawPngFile(filename, x, y, maxW, maxH, 0, 0, 1.0f, 0.0f);
  if (!success) {
    drawPlaceholder(target, x, y, maxW, maxH);
    return false;
  }

  return true;
}

void ImageLoader::drawPlaceholder(lgfx::LGFXBase& target, int16_t x, int16_t y, uint16_t w, uint16_t h) {
  target.fillRoundRect(x, y, w, h, 8, COLOR_PK_BG);
  target.drawRoundRect(x, y, w, h, 8, COLOR_PK_BORDER);
  target.setFont(&fonts::efontJA_10);
  target.setTextColor(COLOR_PK_SUB);
  target.drawCenterString("NO IMAGE", x + (w / 2), y + (h / 2) - 6);
}

// ========== Private Methods ==========

bool ImageLoader::openPNGFile(uint16_t id, char* filename, uint16_t filenameSize) {
  snprintf(filename, filenameSize, "/pokemon/imgs/%04d.png", id);
  return SD.exists(filename);
}
