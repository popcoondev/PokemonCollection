#include "PcRenderer.h"
#include "Renderer.h"

M5Renderer& M5Renderer::instance() {
  static PcRenderer renderer;
  return reinterpret_cast<M5Renderer&>(renderer);
}
