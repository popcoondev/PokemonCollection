#include "ImageLoader.h"
#include <FS.h>
#include <SD.h>
#ifndef POKEMONCOLLECTION_SIM_NATIVE
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

// ========== Constructor & Destructor ==========

ImageLoader::ImageLoader() {
}

ImageLoader::~ImageLoader() {
}

namespace {
#ifdef POKEMONCOLLECTION_SIM_NATIVE
struct SimMutex {
};
using SemaphoreHandle_t = SimMutex*;
constexpr int pdTRUE = 1;
constexpr int portMAX_DELAY = 0;
SemaphoreHandle_t xSemaphoreCreateMutex() {
  static SimMutex mutex;
  return &mutex;
}
int xSemaphoreTake(SemaphoreHandle_t mutex, int ticks) {
  (void)mutex;
  (void)ticks;
  return pdTRUE;
}
void xSemaphoreGive(SemaphoreHandle_t mutex) {
  (void)mutex;
}
#endif

SemaphoreHandle_t pngLoadMutex = nullptr;

uint32_t readBigEndian32(const uint8_t* bytes) {
  return (static_cast<uint32_t>(bytes[0]) << 24)
       | (static_cast<uint32_t>(bytes[1]) << 16)
       | (static_cast<uint32_t>(bytes[2]) << 8)
       | static_cast<uint32_t>(bytes[3]);
}
}

// ========== Public Methods ==========

bool ImageLoader::begin() {
  if (pngLoadMutex == nullptr) {
    pngLoadMutex = xSemaphoreCreateMutex();
    if (pngLoadMutex == nullptr) {
      return false;
    }
  }
  return true;
}

bool ImageLoader::loadAndDisplayPNG(lgfx::LGFXBase& target, uint16_t id, int16_t x, int16_t y,
                                    uint16_t maxW, uint16_t maxH, bool drawErrorOnFailure) {
  char filename[64];
  if (!resolveImagePath(id, filename, sizeof(filename))) {
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }

  return loadAndDisplayPNGInternal(target, filename, x, y, maxW, maxH, drawErrorOnFailure);
}

bool ImageLoader::loadAndDisplayPNGPath(lgfx::LGFXBase& target, const char* path, int16_t x, int16_t y,
                                        uint16_t maxW, uint16_t maxH, bool drawErrorOnFailure) {
  return loadAndDisplayPNGInternal(target, path, x, y, maxW, maxH, drawErrorOnFailure);
}

bool ImageLoader::loadAndDisplayPNGInternal(lgfx::LGFXBase& target, const char* path, int16_t x, int16_t y,
                                            uint16_t maxW, uint16_t maxH, bool drawErrorOnFailure) {
  if (pngLoadMutex == nullptr || xSemaphoreTake(pngLoadMutex, portMAX_DELAY) != pdTRUE) {
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }

  if (path == nullptr || !SD.exists(path)) {
    xSemaphoreGive(pngLoadMutex);
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    xSemaphoreGive(pngLoadMutex);
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }
  file.close();

  uint32_t srcW = 0;
  uint32_t srcH = 0;
  if (!readPngSize(path, srcW, srcH) || srcW == 0 || srcH == 0) {
    xSemaphoreGive(pngLoadMutex);
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }

  const float scale = std::min(static_cast<float>(maxW) / static_cast<float>(srcW),
                               static_cast<float>(maxH) / static_cast<float>(srcH));
  const int drawW = static_cast<int>(srcW * scale);
  const int drawH = static_cast<int>(srcH * scale);
  const int drawX = x + ((static_cast<int>(maxW) - drawW) / 2);
  const int drawY = y + ((static_cast<int>(maxH) - drawH) / 2);

  bool success = target.drawPngFile(SD, path, drawX, drawY, 0, 0, 0, 0, scale, scale);
  xSemaphoreGive(pngLoadMutex);
  if (!success) {
    if (drawErrorOnFailure) {
      drawPlaceholder(target, x, y, maxW, maxH);
    }
    return false;
  }

  return true;
}

void ImageLoader::drawPlaceholder(lgfx::LGFXBase& target, int16_t x, int16_t y, uint16_t w, uint16_t h) {
  target.fillRoundRect(x, y, w, h, 8, COLOR_PK_CARD);
  target.drawRoundRect(x, y, w, h, 8, COLOR_PK_RED);
  target.setFont(&fonts::efontJA_10);
  target.setTextColor(COLOR_PK_RED);
  target.drawCenterString("IMAGE", x + (w / 2), y + (h / 2) - 14);
  target.drawCenterString("ERROR", x + (w / 2), y + (h / 2) + 2);
}

// ========== Private Methods ==========

bool ImageLoader::resolveImagePath(uint16_t id, char* filename, uint16_t filenameSize) {
  snprintf(filename, filenameSize, "/pokemon/imgs/%04d.png", id);
  if (SD.exists(filename)) {
    return true;
  }

  snprintf(filename, filenameSize, "/pokemon/icons/%04d.png", id);
  return SD.exists(filename);
}

bool ImageLoader::readPngSize(const char* path, uint32_t& width, uint32_t& height) {
  width = 0;
  height = 0;

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  uint8_t header[24];
  const size_t readSize = file.read(header, sizeof(header));
  file.close();
  if (readSize != sizeof(header)) {
    return false;
  }

  static const uint8_t pngSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  if (memcmp(header, pngSignature, sizeof(pngSignature)) != 0) {
    return false;
  }

  width = readBigEndian32(&header[16]);
  height = readBigEndian32(&header[20]);
  return true;
}
