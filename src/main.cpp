#include <M5Unified.h>
#include <SD.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "Config.h"
#include "DataManager.h"
#include "UIController.h"

DataManager dataMgr;
UIController ui;
uint16_t currentId = 1;
uint16_t searchId = 1;
uint16_t returnId = 1;
TabType currentTab = TAB_APPEARANCE;

enum ScreenMode {
  SCREEN_MENU = 0,
  SCREEN_DETAIL,
  SCREEN_SEARCH,
  SCREEN_PREVIEW,
  SCREEN_QUIZ,
};

ScreenMode screenMode = SCREEN_MENU;

namespace {
constexpr int kAppearanceImageX = 6;
constexpr int kAppearanceImageY = 54;
constexpr int kAppearanceImageW = 140;
constexpr int kAppearanceImageH = 140;
constexpr int kEvolutionImageW = 50;
constexpr int kEvolutionImageH = 32;
constexpr uint32_t kQuizSideDurationMs = 5000;

enum QuizPhase {
  QUIZ_A_SIDE = 0,
  QUIZ_B_SIDE,
};

struct AppearanceImageRequest {
  uint16_t pokemonId;
  uint32_t generation;
};

struct AppearanceImageResult {
  uint16_t pokemonId;
  uint32_t generation;
  bool success;
};

struct PreviewImageRequest {
  uint16_t pokemonId;
  uint32_t generation;
};

struct PreviewImageResult {
  uint16_t pokemonId;
  uint32_t generation;
  bool success;
};

struct EvolutionImageRequest {
  uint16_t pokemonId;
  uint16_t sourcePokemonId;
  uint8_t imageIndex;
  uint32_t generation;
};

struct EvolutionImageResult {
  uint16_t pokemonId;
  uint16_t sourcePokemonId;
  uint8_t imageIndex;
  uint32_t generation;
  bool success;
};

QueueHandle_t appearanceRequestQueue = nullptr;
QueueHandle_t appearanceResultQueue = nullptr;
SemaphoreHandle_t appearanceSpriteMutex = nullptr;
TaskHandle_t appearanceWorkerTaskHandle = nullptr;
LGFX_Sprite* appearanceImageSprite = nullptr;
ImageLoader appearanceImageLoader;
uint16_t appearanceCachedId = 0;
uint32_t appearanceCachedGeneration = 0;
bool appearanceCacheReady = false;
uint16_t appearanceRequestedId = 0;
uint32_t appearanceRequestedGeneration = 0;

QueueHandle_t previewRequestQueue = nullptr;
QueueHandle_t previewResultQueue = nullptr;
SemaphoreHandle_t previewSpriteMutex = nullptr;
TaskHandle_t previewWorkerTaskHandle = nullptr;
LGFX_Sprite* previewImageSprite = nullptr;
ImageLoader previewImageLoader;
uint16_t previewCachedId = 0;
uint32_t previewCachedGeneration = 0;
bool previewCacheReady = false;
uint16_t previewRequestedId = 0;
uint32_t previewRequestedGeneration = 0;

QueueHandle_t evolutionRequestQueue = nullptr;
QueueHandle_t evolutionResultQueue = nullptr;
SemaphoreHandle_t evolutionSpriteMutex = nullptr;
TaskHandle_t evolutionWorkerTaskHandle = nullptr;
LGFX_Sprite* evolutionImageSprites[3] = {nullptr, nullptr, nullptr};
ImageLoader evolutionImageLoader;
uint16_t evolutionSourcePokemonId = 0;
uint32_t evolutionRequestedGeneration = 0;
uint16_t evolutionRequestedIds[3] = {0, 0, 0};
uint16_t evolutionRenderedIds[3] = {0, 0, 0};
uint32_t evolutionRenderedGeneration = 0;

bool hitTest(int tx, int ty, int x, int y, int w, int h, int pad = 0) {
  return tx >= (x - pad) && tx <= (x + w + pad)
      && ty >= (y - pad) && ty <= (y + h + pad);
}

enum PressedControl {
  PRESS_NONE = 0,
  PRESS_SEARCH_HEADER,
  PRESS_SEARCH_DIGIT_UP_0,
  PRESS_SEARCH_DIGIT_UP_1,
  PRESS_SEARCH_DIGIT_UP_2,
  PRESS_SEARCH_DIGIT_UP_3,
  PRESS_SEARCH_DIGIT_DOWN_0,
  PRESS_SEARCH_DIGIT_DOWN_1,
  PRESS_SEARCH_DIGIT_DOWN_2,
  PRESS_SEARCH_DIGIT_DOWN_3,
  PRESS_SEARCH_CANCEL,
  PRESS_SEARCH_OPEN,
  PRESS_APPEARANCE_PREVIEW,
  PRESS_MENU_POKEDEX,
  PRESS_MENU_QUIZ,
  PRESS_NAV_PREV,
  PRESS_NAV_NEXT,
  PRESS_EVOLUTION_0,
  PRESS_EVOLUTION_1,
  PRESS_EVOLUTION_2,
  PRESS_TAB_0 = 32,
};

PressedControl getPressedControl(int tx, int ty, ScreenMode mode) {
  if (mode == SCREEN_MENU) {
    if (hitTest(tx, ty, 44, 102, SCREEN_WIDTH - 88, 42, 8)) return PRESS_MENU_POKEDEX;
    if (hitTest(tx, ty, 44, 158, SCREEN_WIDTH - 88, 42, 8)) return PRESS_MENU_QUIZ;
    return PRESS_NONE;
  }

  if (mode == SCREEN_QUIZ) {
    return PRESS_MENU_QUIZ;
  }

  if (mode == SCREEN_SEARCH) {
    const int digitX[4] = {70, 115, 160, 205};
    for (int i = 0; i < 4; ++i) {
      if (hitTest(tx, ty, digitX[i], 66, 32, 28, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_UP_0 + i);
      if (hitTest(tx, ty, digitX[i], 148, 32, 28, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_DOWN_0 + i);
    }
    if (hitTest(tx, ty, 24, 196, 126, 34, 10)) return PRESS_SEARCH_CANCEL;
    if (hitTest(tx, ty, 170, 196, 126, 34, 10)) return PRESS_SEARCH_OPEN;
    return PRESS_NONE;
  }

  if (mode == SCREEN_PREVIEW) {
    return PRESS_APPEARANCE_PREVIEW;
  }

  if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12)) return PRESS_SEARCH_HEADER;
  if (hitTest(tx, ty, 0, 0, 40, 204)) return PRESS_NAV_PREV;
  if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, 204)) return PRESS_NAV_NEXT;
  if (currentTab == TAB_APPEARANCE && hitTest(tx, ty, 46, 54, 100, 140, 0)) return PRESS_APPEARANCE_PREVIEW;
  if (hitTest(tx, ty, 22, 80, 84, 80)) return PRESS_EVOLUTION_0;
  if (hitTest(tx, ty, 118, 80, 84, 80)) return static_cast<PressedControl>(PRESS_EVOLUTION_0 + 1);
  if (hitTest(tx, ty, 214, 80, 84, 80)) return static_cast<PressedControl>(PRESS_EVOLUTION_0 + 2);
  if (ty >= 194) return static_cast<PressedControl>(PRESS_TAB_0 + constrain(tx / (SCREEN_WIDTH / 5), 0, 4));
  return PRESS_NONE;
}

enum PendingActionType {
  ACTION_NONE = 0,
  ACTION_OPEN_SEARCH,
  ACTION_OPEN_POKEDEX,
  ACTION_OPEN_QUIZ,
  ACTION_CLOSE_QUIZ,
  ACTION_SEARCH_ADJUST,
  ACTION_SEARCH_CANCEL,
  ACTION_SEARCH_OPEN,
  ACTION_PREVIEW_OPEN,
  ACTION_PREVIEW_CLOSE,
  ACTION_NAV_PREV,
  ACTION_NAV_NEXT,
  ACTION_OPEN_EVOLUTION,
  ACTION_SET_TAB,
};

struct PendingAction {
  PendingActionType type = ACTION_NONE;
  int value = 0;
};

PendingAction makePendingAction(PendingActionType type, int value = 0) {
  PendingAction action;
  action.type = type;
  action.value = value;
  return action;
}

uint16_t adjustSearchDigit(uint16_t currentValue, int placeValue, int direction) {
  const int digit = (currentValue / placeValue) % 10;
  const int nextDigit = (digit + direction + 10) % 10;
  return static_cast<uint16_t>(currentValue + ((nextDigit - digit) * placeValue));
}

uint16_t chooseNextQuizPokemonId(uint16_t lastId) {
  if (MAX_POKEMON_ID <= MIN_POKEMON_ID) {
    return MIN_POKEMON_ID;
  }

  uint16_t nextId = lastId;
  while (nextId == lastId) {
    nextId = static_cast<uint16_t>(random(MIN_POKEMON_ID, MAX_POKEMON_ID + 1));
  }
  return nextId;
}

void renderAppearanceImage(LGFX_Sprite& target, uint16_t pokemonId) {
  target.fillRect(0, 0, kAppearanceImageW, kAppearanceImageH, COLOR_PK_BG);
  appearanceImageLoader.loadAndDisplayPNG(target, pokemonId, 0, 0, kAppearanceImageW, kAppearanceImageH, false);
}

void appearanceImageWorker(void*) {
  AppearanceImageRequest request;

  for (;;) {
    if (xQueueReceive(appearanceRequestQueue, &request, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    bool success = false;
    if (appearanceImageSprite != nullptr
        && xSemaphoreTake(appearanceSpriteMutex, portMAX_DELAY) == pdTRUE) {
      renderAppearanceImage(*appearanceImageSprite, request.pokemonId);
      xSemaphoreGive(appearanceSpriteMutex);
      success = true;
    }

    const AppearanceImageResult result = {
      request.pokemonId,
      request.generation,
      success,
    };
    xQueueSend(appearanceResultQueue, &result, 0);
  }
}

void queueAppearanceImageRequest(uint16_t pokemonId) {
  if (appearanceRequestQueue == nullptr) {
    return;
  }
  if (appearanceRequestedId == pokemonId && appearanceRequestedGeneration != 0) {
    return;
  }

  appearanceRequestedId = pokemonId;
  appearanceRequestedGeneration += 1;
  const AppearanceImageRequest request = {
    pokemonId,
    appearanceRequestedGeneration,
  };
  xQueueOverwrite(appearanceRequestQueue, &request);
}

void renderPreviewImage(LGFX_Sprite& target, uint16_t pokemonId) {
  target.fillScreen(TFT_BLACK);
  previewImageLoader.loadAndDisplayPNG(target, pokemonId, -18, 0, SCREEN_WIDTH + 36, SCREEN_HEIGHT, false);
}

bool ensurePreviewSpriteReady() {
  if (previewImageSprite != nullptr) {
    return true;
  }

  previewImageSprite = new LGFX_Sprite(&M5.Display);
  if (previewImageSprite == nullptr) {
    return false;
  }

  previewImageSprite->setColorDepth(16);
  previewImageSprite->setPsram(true);
  if (!previewImageSprite->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
    delete previewImageSprite;
    previewImageSprite = nullptr;
    return false;
  }

  return true;
}

void previewImageWorker(void*) {
  PreviewImageRequest request;

  for (;;) {
    if (xQueueReceive(previewRequestQueue, &request, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    bool success = false;
    if (ensurePreviewSpriteReady()
        && xSemaphoreTake(previewSpriteMutex, portMAX_DELAY) == pdTRUE) {
      renderPreviewImage(*previewImageSprite, request.pokemonId);
      xSemaphoreGive(previewSpriteMutex);
      success = true;
    }

    const PreviewImageResult result = {
      request.pokemonId,
      request.generation,
      success,
    };
    xQueueSend(previewResultQueue, &result, 0);
  }
}

void queuePreviewImageRequest(uint16_t pokemonId) {
  if (previewRequestQueue == nullptr) {
    return;
  }
  if (previewRequestedId == pokemonId && previewRequestedGeneration != 0) {
    return;
  }

  previewRequestedId = pokemonId;
  previewRequestedGeneration += 1;
  const PreviewImageRequest request = {
    pokemonId,
    previewRequestedGeneration,
  };
  xQueueOverwrite(previewRequestQueue, &request);
}

void renderEvolutionImage(LGFX_Sprite& target, uint16_t pokemonId) {
  target.fillRect(0, 0, kEvolutionImageW, kEvolutionImageH, COLOR_PK_BG);
  evolutionImageLoader.loadAndDisplayPNG(target, pokemonId, 0, 0, kEvolutionImageW, kEvolutionImageH, false);
}

void evolutionImageWorker(void*) {
  EvolutionImageRequest request;

  for (;;) {
    if (xQueueReceive(evolutionRequestQueue, &request, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    bool success = false;
    if (request.imageIndex < 3
        && evolutionImageSprites[request.imageIndex] != nullptr
        && xSemaphoreTake(evolutionSpriteMutex, portMAX_DELAY) == pdTRUE) {
      renderEvolutionImage(*evolutionImageSprites[request.imageIndex], request.pokemonId);
      xSemaphoreGive(evolutionSpriteMutex);
      success = true;
    }

    const EvolutionImageResult result = {
      request.pokemonId,
      request.sourcePokemonId,
      request.imageIndex,
      request.generation,
      success,
    };
    xQueueSend(evolutionResultQueue, &result, 0);
  }
}

void queueEvolutionImageRequests(const PokemonDetail& pk) {
  if (evolutionRequestQueue == nullptr) {
    return;
  }
  evolutionSourcePokemonId = pk.id;
  evolutionRequestedGeneration += 1;
  evolutionRenderedGeneration = 0;
  for (int i = 0; i < 3; ++i) {
    evolutionRequestedIds[i] = 0;
    evolutionRenderedIds[i] = 0;
  }

  for (size_t i = 0; i < pk.evolutions.size() && i < 3; ++i) {
    evolutionRequestedIds[i] = pk.evolutions[i].id;
    const EvolutionImageRequest request = {
      pk.evolutions[i].id,
      pk.id,
      static_cast<uint8_t>(i),
      evolutionRequestedGeneration,
    };
    xQueueSend(evolutionRequestQueue, &request, 0);
  }
}
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  randomSeed(static_cast<uint32_t>(esp_random()));
  M5.Display.setRotation(1);
  M5.Display.setTextFont(2);

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    M5.Display.print("SD Init Error");
    while(1) delay(100);
  }

  if (!dataMgr.begin()) {
    M5.Display.print("SD Error");
    while(1) delay(100);
  }

  if (!ui.begin()) {
    M5.Display.print("UI Error");
    while(1) delay(100);
  }

  appearanceRequestQueue = xQueueCreate(1, sizeof(AppearanceImageRequest));
  appearanceResultQueue = xQueueCreate(4, sizeof(AppearanceImageResult));
  appearanceSpriteMutex = xSemaphoreCreateMutex();
  appearanceImageSprite = new LGFX_Sprite(&M5.Display);
  previewRequestQueue = xQueueCreate(1, sizeof(PreviewImageRequest));
  previewResultQueue = xQueueCreate(4, sizeof(PreviewImageResult));
  previewSpriteMutex = xSemaphoreCreateMutex();
  evolutionRequestQueue = xQueueCreate(6, sizeof(EvolutionImageRequest));
  evolutionResultQueue = xQueueCreate(6, sizeof(EvolutionImageResult));
  evolutionSpriteMutex = xSemaphoreCreateMutex();
  for (auto& sprite : evolutionImageSprites) {
    sprite = new LGFX_Sprite(&M5.Display);
  }

  if (appearanceRequestQueue == nullptr
      || appearanceResultQueue == nullptr
      || appearanceSpriteMutex == nullptr
      || appearanceImageSprite == nullptr
      || previewRequestQueue == nullptr
      || previewResultQueue == nullptr
      || previewSpriteMutex == nullptr
      || evolutionRequestQueue == nullptr
      || evolutionResultQueue == nullptr
      || evolutionSpriteMutex == nullptr
      || evolutionImageSprites[0] == nullptr
      || evolutionImageSprites[1] == nullptr
      || evolutionImageSprites[2] == nullptr
      || !appearanceImageLoader.begin()
      || !previewImageLoader.begin()
      || !evolutionImageLoader.begin()) {
    M5.Display.print("Image Queue Error");
    while(1) delay(100);
  }

  appearanceImageSprite->setColorDepth(16);
  if (!appearanceImageSprite->createSprite(kAppearanceImageW, kAppearanceImageH)) {
    M5.Display.print("Image Sprite Error");
    while(1) delay(100);
  }
  for (auto& sprite : evolutionImageSprites) {
    sprite->setColorDepth(16);
    if (!sprite->createSprite(kEvolutionImageW, kEvolutionImageH)) {
      M5.Display.print("Evolution Sprite Error");
      while(1) delay(100);
    }
  }

  xTaskCreatePinnedToCore(
      appearanceImageWorker,
      "appearanceImage",
      6144,
      nullptr,
      1,
      &appearanceWorkerTaskHandle,
      0);
  xTaskCreatePinnedToCore(
      previewImageWorker,
      "previewImage",
      8192,
      nullptr,
      1,
      &previewWorkerTaskHandle,
      0);
  xTaskCreatePinnedToCore(
      evolutionImageWorker,
      "evolutionImage",
      6144,
      nullptr,
      1,
      &evolutionWorkerTaskHandle,
      0);

  dataMgr.loadPokemonDetail(currentId);
}

void loop() {
  M5.update();
  static PressedControl heldControl = PRESS_NONE;
  static PressedControl latchedControl = PRESS_NONE;
  static PendingAction pendingAction;
  static bool needsRedraw = true;
  static TabType lastRenderedTab = TAB_APPEARANCE;
  static QuizPhase quizPhase = QUIZ_A_SIDE;
  static uint32_t quizPhaseStartedAt = 0;
  static uint16_t quizPokemonId = MIN_POKEMON_ID;

  PressedControl pressedControl = PRESS_NONE;

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    pressedControl = getPressedControl(t.x, t.y, screenMode);
    if (pressedControl != heldControl) {
      heldControl = pressedControl;
      if (latchedControl == PRESS_NONE) {
        needsRedraw = true;
      }
    }
    if (t.wasClicked()) {
      latchedControl = pressedControl;
      needsRedraw = true;

      if (screenMode == SCREEN_SEARCH) {
        if (pressedControl >= PRESS_SEARCH_DIGIT_UP_0 && pressedControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, digitStep[pressedControl - PRESS_SEARCH_DIGIT_UP_0]);
        } else if (pressedControl >= PRESS_SEARCH_DIGIT_DOWN_0 && pressedControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, -digitStep[pressedControl - PRESS_SEARCH_DIGIT_DOWN_0]);
        } else if (pressedControl == PRESS_SEARCH_CANCEL) {
          pendingAction = makePendingAction(ACTION_SEARCH_CANCEL);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN);
        }
      } else if (screenMode == SCREEN_PREVIEW) {
        pendingAction = makePendingAction(ACTION_PREVIEW_CLOSE);
      } else if (screenMode == SCREEN_QUIZ) {
        pendingAction = makePendingAction(ACTION_CLOSE_QUIZ);
      } else if (screenMode == SCREEN_MENU) {
        if (pressedControl == PRESS_MENU_POKEDEX) {
          pendingAction = makePendingAction(ACTION_OPEN_POKEDEX);
        } else if (pressedControl == PRESS_MENU_QUIZ) {
          pendingAction = makePendingAction(ACTION_OPEN_QUIZ);
        }
      } else {
        if (pressedControl == PRESS_SEARCH_HEADER) {
          pendingAction = makePendingAction(ACTION_OPEN_SEARCH);
        } else if (pressedControl == PRESS_APPEARANCE_PREVIEW) {
          pendingAction = makePendingAction(ACTION_PREVIEW_OPEN);
        } else if (pressedControl == PRESS_NAV_PREV) {
          pendingAction = makePendingAction(ACTION_NAV_PREV);
        } else if (pressedControl == PRESS_NAV_NEXT) {
          pendingAction = makePendingAction(ACTION_NAV_NEXT);
        } else if (currentTab == TAB_EVOLUTION
            && pressedControl >= PRESS_EVOLUTION_0
            && pressedControl <= (PRESS_EVOLUTION_0 + 2)) {
          pendingAction = makePendingAction(ACTION_OPEN_EVOLUTION, pressedControl - PRESS_EVOLUTION_0);
        } else if (pressedControl >= PRESS_TAB_0 && pressedControl <= (PRESS_TAB_0 + 4)) {
          pendingAction = makePendingAction(ACTION_SET_TAB, pressedControl - PRESS_TAB_0);
        }
      }
    }
  } else if (heldControl != PRESS_NONE) {
    heldControl = PRESS_NONE;
    if (latchedControl == PRESS_NONE) {
      needsRedraw = true;
    }
  }

  if (pendingAction.type != ACTION_NONE && latchedControl != PRESS_NONE) {
    switch (pendingAction.type) {
      case ACTION_OPEN_POKEDEX:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_OPEN_QUIZ:
        screenMode = SCREEN_QUIZ;
        quizPhase = QUIZ_A_SIDE;
        quizPhaseStartedAt = millis();
        quizPokemonId = chooseNextQuizPokemonId(0);
        break;
      case ACTION_CLOSE_QUIZ:
        screenMode = SCREEN_MENU;
        break;
      case ACTION_OPEN_SEARCH:
        screenMode = SCREEN_SEARCH;
        returnId = currentId;
        searchId = currentId;
        break;
      case ACTION_SEARCH_ADJUST:
        searchId = adjustSearchDigit(
            searchId,
            pendingAction.value > 0 ? pendingAction.value : -pendingAction.value,
            pendingAction.value > 0 ? 1 : -1);
        break;
      case ACTION_SEARCH_CANCEL:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_SEARCH_OPEN:
        if (searchId >= MIN_POKEMON_ID
            && searchId <= MAX_POKEMON_ID
            && dataMgr.getPokemonName(searchId).length() > 0) {
          currentId = searchId;
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      case ACTION_PREVIEW_OPEN:
        screenMode = SCREEN_PREVIEW;
        queuePreviewImageRequest(currentId);
        break;
      case ACTION_PREVIEW_CLOSE:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_NAV_PREV:
        currentId = (currentId <= MIN_POKEMON_ID) ? MAX_POKEMON_ID : (currentId - 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_NAV_NEXT:
        currentId = (currentId >= MAX_POKEMON_ID) ? MIN_POKEMON_ID : (currentId + 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_OPEN_EVOLUTION: {
        const auto& pk = dataMgr.getCurrentPokemon();
        const int index = pendingAction.value;
        if (index >= 0 && index < static_cast<int>(pk.evolutions.size())) {
          currentId = pk.evolutions[index].id;
          currentTab = TAB_APPEARANCE;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      }
      case ACTION_SET_TAB:
        currentTab = static_cast<TabType>(pendingAction.value);
        break;
      case ACTION_NONE:
        break;
    }

    pendingAction = {};
    latchedControl = PRESS_NONE;
    needsRedraw = true;
  }

  if (screenMode == SCREEN_QUIZ && pendingAction.type == ACTION_NONE) {
    const uint32_t now = millis();
    if ((now - quizPhaseStartedAt) >= kQuizSideDurationMs) {
      if (quizPhase == QUIZ_A_SIDE) {
        quizPhase = QUIZ_B_SIDE;
      } else {
        quizPhase = QUIZ_A_SIDE;
        quizPokemonId = chooseNextQuizPokemonId(quizPokemonId);
      }
      quizPhaseStartedAt = now;
      needsRedraw = true;
    }
  }

  const PressedControl visualControl = (latchedControl != PRESS_NONE) ? latchedControl : heldControl;
  const bool tabChanged = lastRenderedTab != currentTab;
  if (tabChanged) {
    needsRedraw = true;
    lastRenderedTab = currentTab;
  }

  AppearanceImageResult imageResult;
  while (appearanceResultQueue != nullptr && xQueueReceive(appearanceResultQueue, &imageResult, 0) == pdTRUE) {
    if (!imageResult.success) {
      if (imageResult.generation == appearanceRequestedGeneration) {
        appearanceRequestedId = 0;
      }
      continue;
    }
    if (imageResult.generation != appearanceRequestedGeneration) {
      continue;
    }

    appearanceCacheReady = true;
    appearanceCachedId = imageResult.pokemonId;
    appearanceCachedGeneration = imageResult.generation;

    if (screenMode == SCREEN_DETAIL
        && currentTab == TAB_APPEARANCE
        && currentId == imageResult.pokemonId
        && appearanceImageSprite != nullptr
        && xSemaphoreTake(appearanceSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      ui.blitAppearanceImageToCanvas(*appearanceImageSprite);
      ui.pushAppearanceImageToDisplay(*appearanceImageSprite);
      ui.redrawDetailNavigationToDisplay();
      xSemaphoreGive(appearanceSpriteMutex);
    }
  }

  PreviewImageResult previewResult;
  while (previewResultQueue != nullptr && xQueueReceive(previewResultQueue, &previewResult, 0) == pdTRUE) {
    if (!previewResult.success) {
      if (previewResult.generation == previewRequestedGeneration) {
        previewRequestedId = 0;
      }
      continue;
    }
    if (previewResult.generation != previewRequestedGeneration) {
      continue;
    }

    previewCacheReady = true;
    previewCachedId = previewResult.pokemonId;
    previewCachedGeneration = previewResult.generation;

    if (screenMode == SCREEN_PREVIEW
        && currentId == previewResult.pokemonId
        && previewImageSprite != nullptr
        && xSemaphoreTake(previewSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      ui.blitPreviewImageToCanvas(*previewImageSprite);
      ui.pushPreviewImageToDisplay(*previewImageSprite);
      xSemaphoreGive(previewSpriteMutex);
    }
  }

  EvolutionImageResult evolutionResult;
  while (evolutionResultQueue != nullptr && xQueueReceive(evolutionResultQueue, &evolutionResult, 0) == pdTRUE) {
    if (!evolutionResult.success) {
      continue;
    }
    if (evolutionResult.generation != evolutionRequestedGeneration
        || evolutionResult.sourcePokemonId != evolutionSourcePokemonId
        || evolutionResult.imageIndex > 2) {
      continue;
    }

    evolutionRenderedIds[evolutionResult.imageIndex] = evolutionResult.pokemonId;
    evolutionRenderedGeneration = evolutionResult.generation;
    if (screenMode == SCREEN_DETAIL
        && currentTab == TAB_EVOLUTION
        && currentId == evolutionResult.sourcePokemonId) {
      needsRedraw = true;
    }
  }

  if (needsRedraw) {
    ui.drawBase();
    const auto& pk = dataMgr.getCurrentPokemon();

    if (screenMode == SCREEN_MENU) {
      ui.drawMenuScreen(
          visualControl == PRESS_MENU_POKEDEX,
          visualControl == PRESS_MENU_QUIZ);
    } else if (screenMode == SCREEN_QUIZ) {
      ui.drawQuizScreen(quizPhase == QUIZ_B_SIDE, dataMgr.getPokemonName(quizPokemonId));
    } else if (screenMode == SCREEN_SEARCH) {
      int pressedDigitDelta = 0;
      if (visualControl >= PRESS_SEARCH_DIGIT_UP_0 && visualControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = digitStep[visualControl - PRESS_SEARCH_DIGIT_UP_0];
      } else if (visualControl >= PRESS_SEARCH_DIGIT_DOWN_0 && visualControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = -digitStep[visualControl - PRESS_SEARCH_DIGIT_DOWN_0];
      }
      ui.drawSearchScreen(
          searchId,
          dataMgr.getPokemonName(searchId),
          pressedDigitDelta,
          visualControl == PRESS_SEARCH_CANCEL,
          visualControl == PRESS_SEARCH_OPEN);
    } else if (screenMode == SCREEN_PREVIEW) {
      ui.drawFullscreenPreview(false, currentId);
      if (previewCacheReady
          && previewCachedId == currentId
          && previewCachedGeneration == previewRequestedGeneration
          && previewImageSprite != nullptr
          && xSemaphoreTake(previewSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        ui.blitPreviewImageToCanvas(*previewImageSprite);
        xSemaphoreGive(previewSpriteMutex);
      } else {
        queuePreviewImageRequest(currentId);
      }
    } else {
      ui.drawHeader(pk, visualControl == PRESS_SEARCH_HEADER);

      if (currentTab == TAB_APPEARANCE) {
        ui.drawAppearanceTab(pk, false);
        if (appearanceCacheReady
            && appearanceCachedId == currentId
            && appearanceCachedGeneration == appearanceRequestedGeneration
            && appearanceImageSprite != nullptr
            && xSemaphoreTake(appearanceSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
          ui.blitAppearanceImageToCanvas(*appearanceImageSprite);
          xSemaphoreGive(appearanceSpriteMutex);
        } else {
          queueAppearanceImageRequest(currentId);
        }
      } else if (currentTab == TAB_DESCRIPTION) {
        ui.drawDescriptionTab(pk);
      } else if (currentTab == TAB_BODY) {
        ui.drawBodyTab(pk);
      } else if (currentTab == TAB_ABILITY) {
        ui.drawAbilityTab(pk);
      } else {
        ui.drawEvolutionTab(
            pk,
            (visualControl >= PRESS_EVOLUTION_0 && visualControl <= (PRESS_EVOLUTION_0 + 2))
                ? (visualControl - PRESS_EVOLUTION_0)
                : -1,
            false);
        if (evolutionSourcePokemonId != pk.id) {
          queueEvolutionImageRequests(pk);
        }
        for (int i = 0; i < 3; ++i) {
          if (evolutionRequestedIds[i] != 0
              && evolutionRenderedGeneration == evolutionRequestedGeneration
              && evolutionRenderedIds[i] == evolutionRequestedIds[i]
              && evolutionImageSprites[i] != nullptr
              && xSemaphoreTake(evolutionSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            ui.blitEvolutionImageToCanvas(*evolutionImageSprites[i], i);
            xSemaphoreGive(evolutionSpriteMutex);
          }
        }
      }

      ui.drawDetailNavigation(visualControl == PRESS_NAV_PREV, visualControl == PRESS_NAV_NEXT);

      ui.drawTabBar(
          currentTab,
          (visualControl >= PRESS_TAB_0 && visualControl <= (PRESS_TAB_0 + 4))
              ? (visualControl - PRESS_TAB_0)
              : -1);
    }

    ui.pushToDisplay();
    needsRedraw = false;
  }

  delay(10);
}
