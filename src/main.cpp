#include <M5Unified.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <math.h>
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
  SCREEN_GUIDE_MENU,
  SCREEN_GUIDE_POKEMON_LIST,
  SCREEN_DETAIL,
  SCREEN_SEARCH,
  SCREEN_SEARCH_INPUT,
  SCREEN_PREVIEW,
  SCREEN_PREVIEW_POC,
  SCREEN_SLIDESHOW,
  SCREEN_QUIZ,
};

ScreenMode screenMode = SCREEN_MENU;

namespace {
constexpr int kAppearanceImageX = 6;
constexpr int kAppearanceImageY = 54;
constexpr int kAppearanceImageW = 140;
constexpr int kAppearanceImageH = 140;
constexpr int kMaxEvolutionCards = 8;
constexpr int kPreviewPocImageSize = 192;
constexpr int kPreviewPocBackgroundMargin = 16;
constexpr uint16_t kPreviewPocTransparentColor = 0x0001;
constexpr uint32_t kSlideshowSlideDurationMs = 5000;
constexpr int kEvolutionImageW = 50;
constexpr int kEvolutionImageH = 32;
constexpr uint32_t kQuizSideDurationMs = 7000;
constexpr const char* kQuizAsideSoundPath = "/pokemon/quiz/sounds/Eyecatch_Aside.wav";
constexpr const char* kQuizBsideSoundPath = "/pokemon/quiz/sounds/Eyecatch_Bside.wav";
constexpr const char* kSettingsPath = "/pokemon/settings.json";

enum QuizPhase {
  QUIZ_A_SIDE = 0,
  QUIZ_B_SIDE,
};

enum SearchMode {
  SEARCH_MODE_NUMBER = 0,
  SEARCH_MODE_NAME,
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
  uint8_t renderW;
  uint8_t renderH;
  uint32_t generation;
};

struct EvolutionImageResult {
  uint16_t pokemonId;
  uint16_t sourcePokemonId;
  uint8_t imageIndex;
  uint32_t generation;
  bool success;
};

struct QuizSoundCache {
  uint8_t* data = nullptr;
  size_t size = 0;
  const int16_t* pcm16 = nullptr;
  size_t sampleCount = 0;
  uint32_t sampleRate = 44100;
  bool stereo = false;
  bool ready = false;
};

enum QuizVolumeSetting {
  QUIZ_VOLUME_LARGE = 0,
  QUIZ_VOLUME_MEDIUM,
  QUIZ_VOLUME_SMALL,
  QUIZ_VOLUME_MUTE,
};

const char* getParallaxBackgroundPathForType(const String& type) {
  if (type == "ノーマル") return "/pokemon/parallax/bg_normal.png";
  if (type == "ほのお") return "/pokemon/parallax/bg_fire.png";
  if (type == "みず") return "/pokemon/parallax/bg_water.png";
  if (type == "でんき") return "/pokemon/parallax/bg_electric.png";
  if (type == "くさ") return "/pokemon/parallax/bg_grass.png";
  if (type == "こおり") return "/pokemon/parallax/bg_ice.png";
  if (type == "かくとう") return "/pokemon/parallax/bg_fighting.png";
  if (type == "どく") return "/pokemon/parallax/bg_poison.png";
  if (type == "じめん") return "/pokemon/parallax/bg_ground.png";
  if (type == "ひこう") return "/pokemon/parallax/bg_flying.png";
  if (type == "エスパー") return "/pokemon/parallax/bg_psychic.png";
  if (type == "むし") return "/pokemon/parallax/bg_bug.png";
  if (type == "いわ") return "/pokemon/parallax/bg_rock.png";
  if (type == "ゴースト") return "/pokemon/parallax/bg_ghost.png";
  if (type == "ドラゴン") return "/pokemon/parallax/bg_dragon.png";
  if (type == "あく") return "/pokemon/parallax/bg_dark.png";
  if (type == "はがね") return "/pokemon/parallax/bg_steel.png";
  if (type == "フェアリー") return "/pokemon/parallax/bg_fairy.png";
  return "/pokemon/parallax/bg_normal.png";
}

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
LGFX_Sprite* previewPocShadowSprite = nullptr;
LGFX_Sprite* previewPocIconSprite = nullptr;
LGFX_Sprite* previewPocBackgroundSprite = nullptr;
ImageLoader previewPocImageLoader;
bool previewPocCacheReady = false;
uint16_t previewPocCachedId = 0;
String previewPocCachedType = "";

QueueHandle_t evolutionRequestQueue = nullptr;
QueueHandle_t evolutionResultQueue = nullptr;
SemaphoreHandle_t evolutionSpriteMutex = nullptr;
TaskHandle_t evolutionWorkerTaskHandle = nullptr;
LGFX_Sprite* evolutionImageSprites[kMaxEvolutionCards] = {};
ImageLoader evolutionImageLoader;
uint16_t evolutionSourcePokemonId = 0;
uint32_t evolutionRequestedGeneration = 0;
uint16_t evolutionRequestedIds[kMaxEvolutionCards] = {};
uint16_t evolutionRenderedIds[kMaxEvolutionCards] = {};
uint32_t evolutionRenderedGeneration = 0;
QuizSoundCache quizAsideSound;
QuizSoundCache quizBsideSound;
QuizVolumeSetting quizVolumeSetting = QUIZ_VOLUME_MEDIUM;
bool preview3dEnabled = false;
SearchMode searchMode = SEARCH_MODE_NUMBER;
size_t searchNameOffset = 0;
String searchNameQuery = "";
bool searchInputVowelMode = false;
int searchInputRowIndex = -1;
uint16_t slideshowPokemonId = MIN_POKEMON_ID;
uint32_t slideshowPhaseStartedAt = 0;

bool hitTest(int tx, int ty, int x, int y, int w, int h, int pad = 0) {
  return tx >= (x - pad) && tx <= (x + w + pad)
      && ty >= (y - pad) && ty <= (y + h + pad);
}

String getPrimaryTypeOrNormal(const PokemonDetail& pk) {
  return pk.types.empty() ? String("ノーマル") : pk.types[0];
}

int getEvolutionCardIndexAt(int tx, int ty, int totalCount) {
  if (totalCount <= 0) return -1;
  if (totalCount <= 3) {
    const int cardX[3] = {22, 118, 214};
    for (int i = 0; i < totalCount; ++i) {
      if (hitTest(tx, ty, cardX[i], 80, 84, 80)) {
        return i;
      }
    }
    return -1;
  }

  constexpr int cols = 4;
  constexpr int cardW = 62;
  constexpr int cardH = 62;
  constexpr int gapX = 8;
  constexpr int gapY = 8;
  constexpr int startX = 24;
  constexpr int startY = 66;
  for (int i = 0; i < totalCount; ++i) {
    const int col = i % cols;
    const int row = i / cols;
    const int x = startX + (col * (cardW + gapX));
    const int y = startY + (row * (cardH + gapY));
    if (hitTest(tx, ty, x, y, cardW, cardH)) {
      return i;
    }
  }
  return -1;
}

void removeLastUtf8Glyph(String& text) {
  int newLength = text.length();
  if (newLength <= 0) return;
  --newLength;
  while (newLength > 0 && (static_cast<uint8_t>(text[newLength]) & 0xC0) == 0x80) {
    --newLength;
  }
  text.remove(newLength);
}

int getSearchInputKeyIndexAt(int tx, int ty) {
  const int keyX[3] = {30, 120, 210};
  const int keyY[4] = {76, 106, 136, 166};
  for (int i = 0; i < 12; ++i) {
    const int col = i % 3;
    const int row = i / 3;
    if (hitTest(tx, ty, keyX[col], keyY[row], 80, 24, 8)) {
      return i;
    }
  }
  return -1;
}

String getSearchInputGlyph(int keyIndex) {
  static constexpr const char* kRowKanaTable[10][5] = {
      {"ア","イ","ウ","エ","オ"},
      {"カ","キ","ク","ケ","コ"},
      {"サ","シ","ス","セ","ソ"},
      {"タ","チ","ツ","テ","ト"},
      {"ナ","ニ","ヌ","ネ","ノ"},
      {"ハ","ヒ","フ","ヘ","ホ"},
      {"マ","ミ","ム","メ","モ"},
      {"ヤ","","ユ","","ヨ"},
      {"ラ","リ","ル","レ","ロ"},
      {"ワ","","ン","ー","ヲ"}
  };
  if (searchInputVowelMode) {
    if (keyIndex >= 0 && keyIndex <= 4 && searchInputRowIndex >= 0 && searchInputRowIndex < 10) {
      return kRowKanaTable[searchInputRowIndex][keyIndex];
    }
    static constexpr const char* kExtraLabels[3] = {"ン", "ー", "ッ"};
    if (keyIndex >= 5 && keyIndex <= 7) {
      return kExtraLabels[keyIndex - 5];
    }
  }
  return "";
}

String getSearchInputRowLabel(int rowIndex) {
  static constexpr const char* kRowLabels[10] = {"ア", "カ", "サ", "タ", "ナ", "ハ", "マ", "ヤ", "ラ", "ワ"};
  if (rowIndex < 0 || rowIndex >= 10) {
    return "";
  }
  return kRowLabels[rowIndex];
}

bool replaceSearchQuerySuffix(const char* from, const char* to) {
  if (!searchNameQuery.endsWith(from)) {
    return false;
  }
  searchNameQuery.remove(searchNameQuery.length() - String(from).length());
  searchNameQuery += to;
  return true;
}

void applySearchDakutenToggle() {
  static const struct { const char* base; const char* daku; const char* handaku; } table[] = {
      {"ウ", "ヴ", nullptr},
      {"カ", "ガ", nullptr}, {"キ", "ギ", nullptr}, {"ク", "グ", nullptr}, {"ケ", "ゲ", nullptr}, {"コ", "ゴ", nullptr},
      {"サ", "ザ", nullptr}, {"シ", "ジ", nullptr}, {"ス", "ズ", nullptr}, {"セ", "ゼ", nullptr}, {"ソ", "ゾ", nullptr},
      {"タ", "ダ", nullptr}, {"チ", "ヂ", nullptr}, {"ツ", "ヅ", nullptr}, {"テ", "デ", nullptr}, {"ト", "ド", nullptr},
      {"ハ", "バ", "パ"}, {"ヒ", "ビ", "ピ"}, {"フ", "ブ", "プ"}, {"ヘ", "ベ", "ペ"}, {"ホ", "ボ", "ポ"},
  };
  for (const auto& entry : table) {
    if (entry.handaku != nullptr && replaceSearchQuerySuffix(entry.handaku, entry.base)) return;
    if (replaceSearchQuerySuffix(entry.daku, entry.handaku != nullptr ? entry.handaku : entry.base)) return;
    if (replaceSearchQuerySuffix(entry.base, entry.daku)) return;
  }
}

void applySearchSmallToggle() {
  static const struct { const char* normal; const char* small; } table[] = {
      {"ア", "ァ"}, {"イ", "ィ"}, {"ウ", "ゥ"}, {"エ", "ェ"}, {"オ", "ォ"},
      {"ヤ", "ャ"}, {"ユ", "ュ"}, {"ヨ", "ョ"}, {"ツ", "ッ"}, {"ワ", "ヮ"},
      {"ァ", "ア"}, {"ィ", "イ"}, {"ゥ", "ウ"}, {"ェ", "エ"}, {"ォ", "オ"},
      {"ャ", "ヤ"}, {"ュ", "ユ"}, {"ョ", "ヨ"}, {"ッ", "ツ"}, {"ヮ", "ワ"},
  };
  for (const auto& entry : table) {
    if (replaceSearchQuerySuffix(entry.normal, entry.small)) return;
  }
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
  PRESS_SEARCH_MENU,
  PRESS_SEARCH_MODE,
  PRESS_SEARCH_NAME_QUERY,
  PRESS_SEARCH_NAME_PAGE_PREV,
  PRESS_SEARCH_NAME_PAGE_NEXT,
  PRESS_SEARCH_NAME_RESULT_0,
  PRESS_SEARCH_NAME_RESULT_1,
  PRESS_SEARCH_NAME_RESULT_2,
  PRESS_SEARCH_NAME_RESULT_3,
  PRESS_SEARCH_NAME_RESULT_4,
  PRESS_SEARCH_NAME_RESULT_5,
  PRESS_SEARCH_NAME_RESULT_6,
  PRESS_SEARCH_NAME_RESULT_7,
  PRESS_SEARCH_NAME_RESULT_8,
  PRESS_SEARCH_NAME_RESULT_9,
  PRESS_APPEARANCE_PREVIEW,
  PRESS_MENU_POKEDEX,
  PRESS_MENU_QUIZ,
  PRESS_MENU_SLIDESHOW,
  PRESS_MENU_GUIDE,
  PRESS_MENU_3D,
  PRESS_MENU_VOL_LARGE,
  PRESS_MENU_VOL_MEDIUM,
  PRESS_MENU_VOL_SMALL,
  PRESS_MENU_VOL_MUTE,
  PRESS_SEARCH_INPUT_BACK,
  PRESS_SEARCH_INPUT_DELETE,
  PRESS_SEARCH_INPUT_CLEAR,
  PRESS_SEARCH_INPUT_KEY_0,
  PRESS_GUIDE_BACK,
  PRESS_GUIDE_POKEMON,
  PRESS_GUIDE_LOCATION,
  PRESS_GUIDE_LIST_BACK,
  PRESS_GUIDE_LIST_ITEM_0,
  PRESS_GUIDE_LIST_ITEM_1,
  PRESS_GUIDE_LIST_ITEM_2,
  PRESS_GUIDE_LIST_ITEM_3,
  PRESS_GUIDE_LIST_ITEM_4,
  PRESS_GUIDE_LIST_ITEM_5,
  PRESS_GUIDE_LIST_ITEM_6,
  PRESS_GUIDE_LIST_ITEM_7,
  PRESS_GUIDE_LIST_ITEM_8,
  PRESS_GUIDE_LIST_ITEM_9,
  PRESS_GUIDE_LIST_PREV,
  PRESS_GUIDE_LIST_NEXT,
  PRESS_NAV_PREV,
  PRESS_NAV_NEXT,
  PRESS_EVOLUTION_0,
  PRESS_EVOLUTION_1,
  PRESS_EVOLUTION_2,
  PRESS_TAB_0 = 128,
};

PressedControl getPressedControl(int tx, int ty, ScreenMode mode) {
  if (mode == SCREEN_MENU) {
    if (hitTest(tx, ty, 24, 64, 130, 34, 8)) return PRESS_MENU_POKEDEX;
    if (hitTest(tx, ty, 24, 106, 130, 34, 8)) return PRESS_MENU_QUIZ;
    if (hitTest(tx, ty, 24, 148, 130, 34, 8)) return PRESS_MENU_SLIDESHOW;
    if (hitTest(tx, ty, 166, 64, 130, 34, 8)) return PRESS_MENU_GUIDE;
    if (hitTest(tx, ty, 246, 14, 50, 18, 6)) return PRESS_MENU_3D;
    if (hitTest(tx, ty, 92, 204, 42, 24, 6)) return PRESS_MENU_VOL_LARGE;
    if (hitTest(tx, ty, 142, 204, 42, 24, 6)) return PRESS_MENU_VOL_MEDIUM;
    if (hitTest(tx, ty, 192, 204, 42, 24, 6)) return PRESS_MENU_VOL_SMALL;
    if (hitTest(tx, ty, 242, 204, 42, 24, 6)) return PRESS_MENU_VOL_MUTE;
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_MENU) {
    if (hitTest(tx, ty, 236, 14, 58, 18, 6)) return PRESS_GUIDE_BACK;
    if (hitTest(tx, ty, 32, 72, SCREEN_WIDTH - 64, 42, 8)) return PRESS_GUIDE_POKEMON;
    if (hitTest(tx, ty, 32, 126, SCREEN_WIDTH - 64, 42, 8)) return PRESS_GUIDE_LOCATION;
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_POKEMON_LIST) {
    if (hitTest(tx, ty, 236, 14, 58, 18, 6)) return PRESS_GUIDE_LIST_BACK;
    constexpr int listX = 12;
    constexpr int listY = 48;
    constexpr int listW = SCREEN_WIDTH - 24;
    constexpr int rowH = 14;
    constexpr int rowGap = 1;
    for (int i = 0; i < 10; ++i) {
      const int y = listY + (i * (rowH + rowGap));
      if (hitTest(tx, ty, listX, y, listW, rowH + 2, 6)) {
        return static_cast<PressedControl>(PRESS_GUIDE_LIST_ITEM_0 + i);
      }
    }
    if (hitTest(tx, ty, 16, 198, 72, 30, 6)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, 232, 198, 72, 30, 6)) return PRESS_GUIDE_LIST_NEXT;
    return PRESS_NONE;
  }

  if (mode == SCREEN_QUIZ) {
    return PRESS_MENU_QUIZ;
  }

  if (mode == SCREEN_SLIDESHOW) {
    return PRESS_MENU_SLIDESHOW;
  }

  if (mode == SCREEN_SEARCH) {
    if (hitTest(tx, ty, 236, 18, 58, 26, 8)) return PRESS_SEARCH_MENU;
    if (hitTest(tx, ty, 168, 18, 58, 26, 8)) return PRESS_SEARCH_MODE;

    if (searchMode == SEARCH_MODE_NUMBER) {
      const int digitX[4] = {60, 110, 160, 210};
      for (int i = 0; i < 4; ++i) {
        if (hitTest(tx, ty, digitX[i], 52, 40, 34, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_UP_0 + i);
        if (hitTest(tx, ty, digitX[i], 120, 40, 34, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_DOWN_0 + i);
      }
      if (hitTest(tx, ty, 20, 178, 132, 40, 10)) return PRESS_SEARCH_CANCEL;
      if (hitTest(tx, ty, 168, 178, 132, 40, 10)) return PRESS_SEARCH_OPEN;
      return PRESS_NONE;
    }

    if (hitTest(tx, ty, 20, 52, 280, 24, 6)) return PRESS_SEARCH_NAME_QUERY;
    const int resultX[2] = {20, 166};
    const int resultY = 84;
    const int resultW = 134;
    const int resultH = 22;
    for (int i = 0; i < 10; ++i) {
      const int col = i % 2;
      const int row = i / 2;
      if (hitTest(tx, ty, resultX[col], resultY + (row * 24), resultW, resultH, 6)) {
        return static_cast<PressedControl>(PRESS_SEARCH_NAME_RESULT_0 + i);
      }
    }
    if (hitTest(tx, ty, 20, 206, 64, 22, 6)) return PRESS_SEARCH_NAME_PAGE_PREV;
    if (hitTest(tx, ty, 236, 206, 64, 22, 6)) return PRESS_SEARCH_NAME_PAGE_NEXT;
    return PRESS_NONE;
  }

  if (mode == SCREEN_SEARCH_INPUT) {
    if (hitTest(tx, ty, 12, 202, 88, 28, 8)) return PRESS_SEARCH_INPUT_BACK;
    if (hitTest(tx, ty, 116, 202, 88, 28, 8)) return PRESS_SEARCH_INPUT_DELETE;
    if (hitTest(tx, ty, 220, 202, 88, 28, 8)) return PRESS_SEARCH_INPUT_CLEAR;
    const int keyIndex = getSearchInputKeyIndexAt(tx, ty);
    if (keyIndex >= 0) {
      return static_cast<PressedControl>(PRESS_SEARCH_INPUT_KEY_0 + keyIndex);
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_PREVIEW) {
    return PRESS_APPEARANCE_PREVIEW;
  }

  if (mode == SCREEN_PREVIEW_POC) {
    return PRESS_MENU_3D;
  }

  const int evolutionIndex = (currentTab == TAB_EVOLUTION)
      ? getEvolutionCardIndexAt(tx, ty, std::min<int>(dataMgr.getCurrentPokemon().evolutions.size(), kMaxEvolutionCards))
      : -1;
  if (evolutionIndex >= 0) {
    return static_cast<PressedControl>(PRESS_EVOLUTION_0 + evolutionIndex);
  }

  if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12)) return PRESS_SEARCH_HEADER;
  if (hitTest(tx, ty, 0, 0, 40, TAB_BAR_Y)) return PRESS_NAV_PREV;
  if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, TAB_BAR_Y)) return PRESS_NAV_NEXT;
  if (currentTab == TAB_APPEARANCE && hitTest(tx, ty, 46, 54, 100, 140, 0)) return PRESS_APPEARANCE_PREVIEW;
  if (ty >= TAB_BAR_Y) return static_cast<PressedControl>(PRESS_TAB_0 + constrain(tx / (SCREEN_WIDTH / 5), 0, 4));
  return PRESS_NONE;
}

enum PendingActionType {
  ACTION_NONE = 0,
  ACTION_OPEN_GUIDE_MENU,
  ACTION_CLOSE_GUIDE_MENU,
  ACTION_OPEN_GUIDE_POKEMON_LIST,
  ACTION_CLOSE_GUIDE_POKEMON_LIST,
  ACTION_GUIDE_POKEMON_LIST_PAGE,
  ACTION_OPEN_SEARCH,
  ACTION_OPEN_POKEDEX,
  ACTION_OPEN_QUIZ,
  ACTION_OPEN_SLIDESHOW,
  ACTION_CLOSE_SLIDESHOW,
  ACTION_CLOSE_PREVIEW_POC,
  ACTION_CLOSE_QUIZ,
  ACTION_TOGGLE_PREVIEW_3D,
  ACTION_SEARCH_TO_MENU,
  ACTION_SEARCH_TOGGLE_MODE,
  ACTION_SEARCH_OPEN_INPUT,
  ACTION_SEARCH_CLOSE_INPUT,
  ACTION_SEARCH_INPUT_ROW,
  ACTION_SEARCH_INPUT_APPEND,
  ACTION_SEARCH_INPUT_DELETE,
  ACTION_SEARCH_INPUT_CLEAR,
  ACTION_SEARCH_NAME_PAGE,
  ACTION_SEARCH_NAME_OPEN,
  ACTION_SET_QUIZ_VOLUME,
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

uint16_t getAvailableMaxPokemonId() {
  const uint16_t maxId = dataMgr.getMaxPokemonId();
  return (maxId >= MIN_POKEMON_ID) ? maxId : MIN_POKEMON_ID;
}

std::vector<uint16_t> getGuidePokemonPageIds(size_t offset, size_t limit) {
  std::vector<uint16_t> ids;
  const uint16_t maxId = getAvailableMaxPokemonId();
  for (uint16_t id = MIN_POKEMON_ID; id <= maxId; ++id) {
    if (dataMgr.getPokemonName(id).length() == 0) {
      continue;
    }
    if (offset > 0) {
      --offset;
      continue;
    }
    ids.push_back(id);
    if (ids.size() >= limit) {
      break;
    }
  }
  return ids;
}

uint16_t adjustSearchDigit(uint16_t currentValue, int placeValue, int direction) {
  const int digit = (currentValue / placeValue) % 10;
  const int nextDigit = (digit + direction + 10) % 10;
  return static_cast<uint16_t>(currentValue + ((nextDigit - digit) * placeValue));
}

uint8_t getQuizVolumeValue(QuizVolumeSetting setting) {
  switch (setting) {
    case QUIZ_VOLUME_LARGE: return 200;
    case QUIZ_VOLUME_MEDIUM: return 128;
    case QUIZ_VOLUME_SMALL: return 64;
    case QUIZ_VOLUME_MUTE: return 0;
  }
  return 128;
}

const char* getQuizVolumeKey(QuizVolumeSetting setting) {
  switch (setting) {
    case QUIZ_VOLUME_LARGE: return "large";
    case QUIZ_VOLUME_MEDIUM: return "medium";
    case QUIZ_VOLUME_SMALL: return "small";
    case QUIZ_VOLUME_MUTE: return "mute";
  }
  return "medium";
}

QuizVolumeSetting parseQuizVolumeKey(const char* value) {
  if (value == nullptr) return QUIZ_VOLUME_MEDIUM;
  if (strcmp(value, "large") == 0) return QUIZ_VOLUME_LARGE;
  if (strcmp(value, "small") == 0) return QUIZ_VOLUME_SMALL;
  if (strcmp(value, "mute") == 0) return QUIZ_VOLUME_MUTE;
  return QUIZ_VOLUME_MEDIUM;
}

void applyQuizVolume() {
  M5.Speaker.setVolume(getQuizVolumeValue(quizVolumeSetting));
}

bool saveSettings() {
  JsonDocument doc;
  doc["quiz_volume"] = getQuizVolumeKey(quizVolumeSetting);
  doc["preview_3d"] = preview3dEnabled;

  if (SD.exists(kSettingsPath)) {
    SD.remove(kSettingsPath);
  }
  File file = SD.open(kSettingsPath, FILE_WRITE);
  if (!file) {
    return false;
  }
  const bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

void loadSettings() {
  quizVolumeSetting = QUIZ_VOLUME_MEDIUM;
  preview3dEnabled = false;

  File file = SD.open(kSettingsPath, FILE_READ);
  if (!file) {
    saveSettings();
    applyQuizVolume();
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, file) == DeserializationError::Ok) {
    quizVolumeSetting = parseQuizVolumeKey(doc["quiz_volume"] | "medium");
    preview3dEnabled = doc["preview_3d"] | false;
  }
  file.close();
  applyQuizVolume();
}

bool loadQuizSound(const char* path, QuizSoundCache& cache) {
  if (cache.ready && cache.data != nullptr && cache.size > 0) {
    return true;
  }

  struct __attribute__((packed)) RiffHeader {
    char riff[4];
    uint32_t chunkSize;
    char wave[4];
  };

  struct __attribute__((packed)) WavChunkHeader {
    char identifier[4];
    uint32_t chunkSize;
  };

  struct __attribute__((packed)) WavFmtChunk {
    uint16_t audioFmt;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t bytesPerSec;
    uint16_t blockSize;
    uint16_t bitPerSample;
  };

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  const size_t size = file.size();
  if (size == 0) {
    file.close();
    return false;
  }

  uint8_t* buffer = static_cast<uint8_t*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (buffer == nullptr) {
    buffer = static_cast<uint8_t*>(malloc(size));
  }
  if (buffer == nullptr) {
    file.close();
    return false;
  }

  const size_t readSize = file.read(buffer, size);
  file.close();
  if (readSize != size) {
    free(buffer);
    return false;
  }

  if (size < sizeof(RiffHeader)) {
    free(buffer);
    return false;
  }

  auto* riff = reinterpret_cast<RiffHeader*>(buffer);
  if (memcmp(riff->riff, "RIFF", 4) != 0
      || memcmp(riff->wave, "WAVE", 4) != 0) {
    free(buffer);
    return false;
  }

  bool foundFmt = false;
  bool foundData = false;
  WavFmtChunk fmt = {};
  uint8_t* pcmStart = nullptr;
  uint32_t pcmSize = 0;

  uint8_t* cursor = buffer + sizeof(RiffHeader);
  const uint8_t* bufferEnd = buffer + size;
  while ((cursor + sizeof(WavChunkHeader)) <= bufferEnd) {
    auto* chunk = reinterpret_cast<WavChunkHeader*>(cursor);
    uint8_t* chunkData = cursor + sizeof(WavChunkHeader);
    if ((chunkData + chunk->chunkSize) > bufferEnd) {
      break;
    }

    if (memcmp(chunk->identifier, "fmt ", 4) == 0) {
      if (chunk->chunkSize < sizeof(WavFmtChunk)) {
        break;
      }
      memcpy(&fmt, chunkData, sizeof(WavFmtChunk));
      foundFmt = true;
    } else if (memcmp(chunk->identifier, "data", 4) == 0) {
      pcmStart = chunkData;
      pcmSize = chunk->chunkSize;
      foundData = true;
    }

    cursor = chunkData + chunk->chunkSize + (chunk->chunkSize & 1);
  }

  if (!foundFmt
      || !foundData
      || fmt.audioFmt != 1
      || fmt.channels == 0
      || fmt.channels > 2
      || fmt.bitPerSample != 16) {
    free(buffer);
    return false;
  }

  cache.data = buffer;
  cache.size = size;
  cache.pcm16 = reinterpret_cast<const int16_t*>(pcmStart);
  cache.sampleCount = pcmSize / sizeof(int16_t);
  cache.sampleRate = fmt.sampleRate;
  cache.stereo = fmt.channels > 1;
  cache.ready = true;
  return true;
}

void playQuizSound(QuizPhase phase) {
  QuizSoundCache& cache = (phase == QUIZ_A_SIDE) ? quizAsideSound : quizBsideSound;
  const char* path = (phase == QUIZ_A_SIDE) ? kQuizAsideSoundPath : kQuizBsideSoundPath;
  if (!loadQuizSound(path, cache)) {
    return;
  }

  M5.Speaker.stop();
  M5.Speaker.playRaw(
      cache.pcm16,
      cache.sampleCount,
      cache.sampleRate,
      cache.stereo,
      1,
      0,
      true);
}

uint16_t chooseNextQuizPokemonId(uint16_t lastId) {
  const uint16_t maxPokemonId = getAvailableMaxPokemonId();
  if (maxPokemonId <= MIN_POKEMON_ID) {
    return MIN_POKEMON_ID;
  }

  uint16_t nextId = lastId;
  while (nextId == lastId) {
    nextId = static_cast<uint16_t>(random(MIN_POKEMON_ID, maxPokemonId + 1));
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

bool ensurePreviewPocCacheReady(uint16_t pokemonId, const String& primaryType) {
  if (previewPocCacheReady
      && previewPocCachedId == pokemonId
      && previewPocCachedType == primaryType) {
    return true;
  }

  if (!previewPocImageLoader.begin()) {
    return false;
  }

  if (previewPocShadowSprite == nullptr) {
    previewPocShadowSprite = new LGFX_Sprite(&M5.Display);
    if (previewPocShadowSprite == nullptr) {
      return false;
    }
    previewPocShadowSprite->setColorDepth(16);
    previewPocShadowSprite->setPsram(true);
    if (!previewPocShadowSprite->createSprite(kPreviewPocImageSize, kPreviewPocImageSize)) {
      delete previewPocShadowSprite;
      previewPocShadowSprite = nullptr;
      return false;
    }
  }

  if (previewPocIconSprite == nullptr) {
    previewPocIconSprite = new LGFX_Sprite(&M5.Display);
    if (previewPocIconSprite == nullptr) {
      return false;
    }
    previewPocIconSprite->setColorDepth(16);
    previewPocIconSprite->setPsram(true);
    if (!previewPocIconSprite->createSprite(kPreviewPocImageSize, kPreviewPocImageSize)) {
      delete previewPocIconSprite;
      previewPocIconSprite = nullptr;
      return false;
    }
  }

  if (previewPocBackgroundSprite == nullptr) {
    previewPocBackgroundSprite = new LGFX_Sprite(&M5.Display);
    if (previewPocBackgroundSprite != nullptr) {
      previewPocBackgroundSprite->setColorDepth(16);
      previewPocBackgroundSprite->setPsram(true);
      if (!previewPocBackgroundSprite->createSprite(
              SCREEN_WIDTH + (kPreviewPocBackgroundMargin * 2),
              SCREEN_HEIGHT + (kPreviewPocBackgroundMargin * 2))) {
        delete previewPocBackgroundSprite;
        previewPocBackgroundSprite = nullptr;
      }
    }
  }

  previewPocShadowSprite->fillRect(0, 0, kPreviewPocImageSize, kPreviewPocImageSize, kPreviewPocTransparentColor);
  previewPocIconSprite->fillRect(0, 0, kPreviewPocImageSize, kPreviewPocImageSize, kPreviewPocTransparentColor);
  if (previewPocBackgroundSprite != nullptr) {
    previewPocBackgroundSprite->fillScreen(TFT_WHITE);
  }

  char silhouettePath[64];
  char iconPath[64];
  snprintf(silhouettePath, sizeof(silhouettePath), "/pokemon/silhouettes/%04d.png", pokemonId);
  snprintf(iconPath, sizeof(iconPath), "/pokemon/icons/%04d.png", pokemonId);

  const bool shadowReady = previewPocImageLoader.loadAndDisplayPNGPath(
      *previewPocShadowSprite,
      silhouettePath,
      0,
      0,
      kPreviewPocImageSize,
      kPreviewPocImageSize,
      false);
  const bool iconReady = previewPocImageLoader.loadAndDisplayPNGPath(
      *previewPocIconSprite,
      iconPath,
      0,
      0,
      kPreviewPocImageSize,
      kPreviewPocImageSize,
      false);
  const bool backgroundReady = (previewPocBackgroundSprite == nullptr)
      ? false
      : previewPocImageLoader.loadAndDisplayPNGPath(
            *previewPocBackgroundSprite,
            getParallaxBackgroundPathForType(primaryType),
            0,
            0,
            SCREEN_WIDTH + (kPreviewPocBackgroundMargin * 2),
            SCREEN_HEIGHT + (kPreviewPocBackgroundMargin * 2),
            false);

  previewPocCacheReady = shadowReady && iconReady;
  if (previewPocCacheReady) {
    previewPocCachedId = pokemonId;
    previewPocCachedType = primaryType;
  }
  if (!backgroundReady && previewPocBackgroundSprite != nullptr) {
    previewPocBackgroundSprite->fillScreen(TFT_WHITE);
  }
  return previewPocCacheReady;
}

void renderEvolutionImage(LGFX_Sprite& target, uint16_t pokemonId, uint16_t renderW, uint16_t renderH) {
  target.fillRect(0, 0, kEvolutionImageW, kEvolutionImageH, COLOR_PK_BG);
  evolutionImageLoader.loadAndDisplayPNG(target, pokemonId, 0, 0, renderW, renderH, false);
}

void evolutionImageWorker(void*) {
  EvolutionImageRequest request;

  for (;;) {
    if (xQueueReceive(evolutionRequestQueue, &request, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    bool success = false;
    if (request.imageIndex < kMaxEvolutionCards
        && evolutionImageSprites[request.imageIndex] != nullptr
        && xSemaphoreTake(evolutionSpriteMutex, portMAX_DELAY) == pdTRUE) {
      renderEvolutionImage(
          *evolutionImageSprites[request.imageIndex],
          request.pokemonId,
          request.renderW,
          request.renderH);
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
  for (int i = 0; i < kMaxEvolutionCards; ++i) {
    evolutionRequestedIds[i] = 0;
    evolutionRenderedIds[i] = 0;
  }

  for (size_t i = 0; i < pk.evolutions.size() && i < kMaxEvolutionCards; ++i) {
    evolutionRequestedIds[i] = pk.evolutions[i].id;
    const EvolutionImageRequest request = {
      pk.evolutions[i].id,
      pk.id,
      static_cast<uint8_t>(i),
      kEvolutionImageW,
      kEvolutionImageH,
      evolutionRequestedGeneration,
    };
    xQueueSend(evolutionRequestQueue, &request, 0);
  }
}
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_spk = true;
  cfg.internal_mic = false;
  M5.begin(cfg);
  randomSeed(static_cast<uint32_t>(esp_random()));
  M5.Display.setRotation(1);
  M5.Display.setTextFont(2);
  {
    auto spk_cfg = M5.Speaker.config();
    spk_cfg.sample_rate = 44100;
    M5.Speaker.config(spk_cfg);
  }
  M5.Speaker.begin();
  M5.Speaker.setVolume(128);

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

  loadSettings();

  appearanceRequestQueue = xQueueCreate(1, sizeof(AppearanceImageRequest));
  appearanceResultQueue = xQueueCreate(4, sizeof(AppearanceImageResult));
  appearanceSpriteMutex = xSemaphoreCreateMutex();
  appearanceImageSprite = new LGFX_Sprite(&M5.Display);
  previewRequestQueue = xQueueCreate(1, sizeof(PreviewImageRequest));
  previewResultQueue = xQueueCreate(4, sizeof(PreviewImageResult));
  previewSpriteMutex = xSemaphoreCreateMutex();
  evolutionRequestQueue = xQueueCreate(kMaxEvolutionCards, sizeof(EvolutionImageRequest));
  evolutionResultQueue = xQueueCreate(kMaxEvolutionCards, sizeof(EvolutionImageResult));
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
      || !appearanceImageLoader.begin()
      || !previewImageLoader.begin()
      || !evolutionImageLoader.begin()) {
    M5.Display.print("Image Queue Error");
    while(1) delay(100);
  }

  for (auto* sprite : evolutionImageSprites) {
    if (sprite == nullptr) {
      M5.Display.print("Image Queue Error");
      while(1) delay(100);
    }
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
  static size_t guidePokemonListOffset = 0;

  PressedControl pressedControl = PRESS_NONE;

  if (screenMode == SCREEN_QUIZ && M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    heldControl = PRESS_MENU_QUIZ;
    if (t.wasClicked()) {
      latchedControl = PRESS_MENU_QUIZ;
      pendingAction = makePendingAction(ACTION_CLOSE_QUIZ);
      needsRedraw = true;
    }
  } else if (M5.Touch.getCount() > 0) {
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

      if (screenMode == SCREEN_SEARCH_INPUT) {
        if (pressedControl == PRESS_SEARCH_INPUT_BACK) {
          pendingAction = makePendingAction(ACTION_SEARCH_CLOSE_INPUT);
        } else if (pressedControl == PRESS_SEARCH_INPUT_DELETE) {
          pendingAction = makePendingAction(ACTION_SEARCH_INPUT_DELETE);
        } else if (pressedControl == PRESS_SEARCH_INPUT_CLEAR) {
          pendingAction = makePendingAction(ACTION_SEARCH_INPUT_CLEAR);
        } else if (pressedControl >= PRESS_SEARCH_INPUT_KEY_0 && pressedControl < (PRESS_SEARCH_INPUT_KEY_0 + 12)) {
          const int keyIndex = pressedControl - PRESS_SEARCH_INPUT_KEY_0;
          pendingAction = makePendingAction(
              searchInputVowelMode ? ACTION_SEARCH_INPUT_APPEND : ACTION_SEARCH_INPUT_ROW,
              keyIndex);
        }
      } else if (screenMode == SCREEN_SEARCH) {
        if (pressedControl == PRESS_SEARCH_NAME_QUERY) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN_INPUT);
        } else if (pressedControl == PRESS_SEARCH_MODE) {
          pendingAction = makePendingAction(ACTION_SEARCH_TOGGLE_MODE);
        } else if (pressedControl >= PRESS_SEARCH_DIGIT_UP_0 && pressedControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, digitStep[pressedControl - PRESS_SEARCH_DIGIT_UP_0]);
        } else if (pressedControl >= PRESS_SEARCH_DIGIT_DOWN_0 && pressedControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, -digitStep[pressedControl - PRESS_SEARCH_DIGIT_DOWN_0]);
        } else if (pressedControl == PRESS_SEARCH_CANCEL) {
          pendingAction = makePendingAction(ACTION_SEARCH_CANCEL);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN);
        } else if (pressedControl == PRESS_SEARCH_MENU) {
          pendingAction = makePendingAction(ACTION_SEARCH_TO_MENU);
        } else if (pressedControl == PRESS_SEARCH_NAME_PAGE_PREV) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_PAGE, -10);
        } else if (pressedControl == PRESS_SEARCH_NAME_PAGE_NEXT) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_PAGE, 10);
        } else if (pressedControl >= PRESS_SEARCH_NAME_RESULT_0 && pressedControl <= PRESS_SEARCH_NAME_RESULT_9) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_OPEN, pressedControl - PRESS_SEARCH_NAME_RESULT_0);
        }
      } else if (screenMode == SCREEN_PREVIEW) {
        pendingAction = makePendingAction(ACTION_PREVIEW_CLOSE);
      } else if (screenMode == SCREEN_PREVIEW_POC) {
        pendingAction = makePendingAction(ACTION_CLOSE_PREVIEW_POC);
      } else if (screenMode == SCREEN_SLIDESHOW) {
        pendingAction = makePendingAction(ACTION_CLOSE_SLIDESHOW);
      } else if (screenMode == SCREEN_GUIDE_MENU) {
        if (pressedControl == PRESS_GUIDE_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_MENU);
        } else if (pressedControl == PRESS_GUIDE_POKEMON) {
          pendingAction = makePendingAction(ACTION_OPEN_GUIDE_POKEMON_LIST);
        }
      } else if (screenMode == SCREEN_GUIDE_POKEMON_LIST) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_POKEMON_LIST);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_GUIDE_POKEMON_LIST_PAGE, -10);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_GUIDE_POKEMON_LIST_PAGE, 10);
        }
      } else if (screenMode == SCREEN_MENU) {
        if (pressedControl == PRESS_MENU_POKEDEX) {
          pendingAction = makePendingAction(ACTION_OPEN_POKEDEX);
        } else if (pressedControl == PRESS_MENU_QUIZ) {
          pendingAction = makePendingAction(ACTION_OPEN_QUIZ);
        } else if (pressedControl == PRESS_MENU_SLIDESHOW) {
          pendingAction = makePendingAction(ACTION_OPEN_SLIDESHOW);
        } else if (pressedControl == PRESS_MENU_GUIDE) {
          pendingAction = makePendingAction(ACTION_OPEN_GUIDE_MENU);
        } else if (pressedControl == PRESS_MENU_3D) {
          pendingAction = makePendingAction(ACTION_TOGGLE_PREVIEW_3D);
        } else if (pressedControl >= PRESS_MENU_VOL_LARGE && pressedControl <= PRESS_MENU_VOL_MUTE) {
          pendingAction = makePendingAction(ACTION_SET_QUIZ_VOLUME, pressedControl - PRESS_MENU_VOL_LARGE);
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
            && pressedControl <= (PRESS_EVOLUTION_0 + (kMaxEvolutionCards - 1))) {
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
      case ACTION_OPEN_GUIDE_MENU:
        screenMode = SCREEN_GUIDE_MENU;
        break;
      case ACTION_CLOSE_GUIDE_MENU:
        screenMode = SCREEN_MENU;
        break;
      case ACTION_OPEN_GUIDE_POKEMON_LIST:
        screenMode = SCREEN_GUIDE_POKEMON_LIST;
        guidePokemonListOffset = 0;
        break;
      case ACTION_CLOSE_GUIDE_POKEMON_LIST:
        screenMode = SCREEN_GUIDE_MENU;
        break;
      case ACTION_GUIDE_POKEMON_LIST_PAGE: {
        const int nextOffset = static_cast<int>(guidePokemonListOffset) + pendingAction.value;
        guidePokemonListOffset = static_cast<size_t>(nextOffset < 0 ? 0 : nextOffset);
        break;
      }
      case ACTION_OPEN_POKEDEX:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_OPEN_QUIZ:
        screenMode = SCREEN_QUIZ;
        quizPhase = QUIZ_A_SIDE;
        quizPhaseStartedAt = millis();
        quizPokemonId = chooseNextQuizPokemonId(0);
        playQuizSound(quizPhase);
        break;
      case ACTION_OPEN_SLIDESHOW:
        screenMode = SCREEN_SLIDESHOW;
        slideshowPokemonId = MIN_POKEMON_ID;
        slideshowPhaseStartedAt = millis();
        dataMgr.loadPokemonDetail(slideshowPokemonId);
        if (preview3dEnabled) {
          previewPocCacheReady = false;
          ensurePreviewPocCacheReady(slideshowPokemonId, getPrimaryTypeOrNormal(dataMgr.getCurrentPokemon()));
        }
        break;
      case ACTION_CLOSE_SLIDESHOW:
        screenMode = SCREEN_MENU;
        dataMgr.loadPokemonDetail(currentId);
        break;
      case ACTION_CLOSE_PREVIEW_POC:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_TOGGLE_PREVIEW_3D:
        preview3dEnabled = !preview3dEnabled;
        saveSettings();
        break;
      case ACTION_CLOSE_QUIZ:
        M5.Speaker.stop();
        screenMode = SCREEN_MENU;
        break;
      case ACTION_SET_QUIZ_VOLUME:
        quizVolumeSetting = static_cast<QuizVolumeSetting>(pendingAction.value);
        applyQuizVolume();
        saveSettings();
        break;
      case ACTION_OPEN_SEARCH:
        screenMode = SCREEN_SEARCH;
        returnId = currentId;
        searchId = currentId;
        searchMode = SEARCH_MODE_NUMBER;
        searchNameOffset = 0;
        break;
      case ACTION_SEARCH_TOGGLE_MODE:
        searchMode = (searchMode == SEARCH_MODE_NUMBER) ? SEARCH_MODE_NAME : SEARCH_MODE_NUMBER;
        searchNameOffset = 0;
        break;
      case ACTION_SEARCH_OPEN_INPUT:
        screenMode = SCREEN_SEARCH_INPUT;
        searchInputVowelMode = false;
        searchInputRowIndex = -1;
        break;
      case ACTION_SEARCH_CLOSE_INPUT:
        if (searchInputVowelMode) {
          searchInputVowelMode = false;
          searchInputRowIndex = -1;
        } else {
          screenMode = SCREEN_SEARCH;
        }
        break;
      case ACTION_SEARCH_INPUT_ROW:
        if (pendingAction.value == 10) {
          applySearchDakutenToggle();
          searchNameOffset = 0;
        } else if (pendingAction.value == 11) {
          applySearchSmallToggle();
          searchNameOffset = 0;
        } else if (pendingAction.value >= 0 && pendingAction.value < 10) {
          searchInputVowelMode = true;
          searchInputRowIndex = pendingAction.value;
        }
        break;
      case ACTION_SEARCH_INPUT_APPEND: {
        if (pendingAction.value == 10) {
          applySearchDakutenToggle();
          searchNameOffset = 0;
        } else if (pendingAction.value == 11) {
          applySearchSmallToggle();
          searchNameOffset = 0;
        } else {
          const String input = getSearchInputGlyph(pendingAction.value);
          if (input.length() == 0) {
            break;
          }
          searchNameQuery += input;
          searchNameOffset = 0;
          searchInputVowelMode = false;
          searchInputRowIndex = -1;
        }
        break;
      }
      case ACTION_SEARCH_INPUT_DELETE:
        if (searchNameQuery.length() > 0) {
          removeLastUtf8Glyph(searchNameQuery);
          searchNameOffset = 0;
        }
        break;
      case ACTION_SEARCH_INPUT_CLEAR:
        searchNameQuery = "";
        searchNameOffset = 0;
        searchInputVowelMode = false;
        searchInputRowIndex = -1;
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
      case ACTION_SEARCH_TO_MENU:
        screenMode = SCREEN_MENU;
        break;
      case ACTION_SEARCH_NAME_PAGE: {
        const int nextOffset = static_cast<int>(searchNameOffset) + pendingAction.value;
        searchNameOffset = static_cast<size_t>(nextOffset < 0 ? 0 : nextOffset);
        break;
      }
      case ACTION_SEARCH_NAME_OPEN: {
        auto candidateIds = dataMgr.findPokemonIdsByName(
            searchNameQuery,
            screenMode == SCREEN_SEARCH_INPUT ? 0 : searchNameOffset,
            screenMode == SCREEN_SEARCH_INPUT ? 5 : 10);
        const int index = pendingAction.value;
        if (index >= 0 && index < static_cast<int>(candidateIds.size())) {
          currentId = candidateIds[index];
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      }
      case ACTION_SEARCH_OPEN:
        if (searchId > getAvailableMaxPokemonId()) {
          break;
        }
        if (searchId >= MIN_POKEMON_ID
            && dataMgr.getPokemonName(searchId).length() > 0) {
          currentId = searchId;
          returnId = currentId;
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      case ACTION_PREVIEW_OPEN:
        if (preview3dEnabled) {
          previewPocCacheReady = false;
          {
            const auto& pk = dataMgr.getCurrentPokemon();
            const String previewType = pk.types.empty() ? String("ノーマル") : pk.types[0];
            ensurePreviewPocCacheReady(currentId, previewType);
          }
          screenMode = SCREEN_PREVIEW_POC;
        } else {
          screenMode = SCREEN_PREVIEW;
          queuePreviewImageRequest(currentId);
        }
        break;
      case ACTION_PREVIEW_CLOSE:
        screenMode = SCREEN_DETAIL;
        break;
      case ACTION_NAV_PREV: {
        const uint16_t maxPokemonId = getAvailableMaxPokemonId();
        currentId = (currentId <= MIN_POKEMON_ID) ? maxPokemonId : (currentId - 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      }
      case ACTION_NAV_NEXT: {
        const uint16_t maxPokemonId = getAvailableMaxPokemonId();
        currentId = (currentId >= maxPokemonId) ? MIN_POKEMON_ID : (currentId + 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      }
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
      playQuizSound(quizPhase);
      needsRedraw = true;
    }
  }

  if (screenMode == SCREEN_SLIDESHOW && pendingAction.type == ACTION_NONE) {
    const uint32_t elapsed = millis() - slideshowPhaseStartedAt;
    if (elapsed >= kSlideshowSlideDurationMs) {
      const uint16_t maxPokemonId = getAvailableMaxPokemonId();
      slideshowPokemonId = (slideshowPokemonId >= maxPokemonId) ? MIN_POKEMON_ID : (slideshowPokemonId + 1);
      slideshowPhaseStartedAt = millis();
      dataMgr.loadPokemonDetail(slideshowPokemonId);
      if (preview3dEnabled) {
        previewPocCacheReady = false;
        ensurePreviewPocCacheReady(slideshowPokemonId, getPrimaryTypeOrNormal(dataMgr.getCurrentPokemon()));
      }
      needsRedraw = true;
    }
  }

  static float previewPocShiftXF = 0.0f;
  static float previewPocShiftYF = 0.0f;
  static int previewPocShiftX = 0;
  static int previewPocShiftY = 0;
  if (screenMode == SCREEN_PREVIEW_POC || (screenMode == SCREEN_SLIDESHOW && preview3dEnabled)) {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    if (M5.Imu.getAccel(&ax, &ay, &az)) {
      const float targetShiftX = constrain(ax * -10.0f, -4.0f, 4.0f);
      const float targetShiftY = constrain(ay * 10.0f, -4.0f, 4.0f);
      previewPocShiftXF += (targetShiftX - previewPocShiftXF) * 0.45f;
      previewPocShiftYF += (targetShiftY - previewPocShiftYF) * 0.45f;
      const int nextShiftX = static_cast<int>(roundf(previewPocShiftXF));
      const int nextShiftY = static_cast<int>(roundf(previewPocShiftYF));
      if (nextShiftX != previewPocShiftX || nextShiftY != previewPocShiftY) {
        previewPocShiftX = nextShiftX;
        previewPocShiftY = nextShiftY;
        needsRedraw = true;
      }
    }
    needsRedraw = true;
  } else {
    previewPocShiftXF = 0.0f;
    previewPocShiftYF = 0.0f;
    previewPocShiftX = 0;
    previewPocShiftY = 0;
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
        || evolutionResult.imageIndex >= kMaxEvolutionCards) {
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
          visualControl == PRESS_MENU_QUIZ,
          visualControl == PRESS_MENU_SLIDESHOW,
          visualControl == PRESS_MENU_GUIDE,
          preview3dEnabled,
          visualControl == PRESS_MENU_3D,
          static_cast<int>(quizVolumeSetting),
          (visualControl >= PRESS_MENU_VOL_LARGE && visualControl <= PRESS_MENU_VOL_MUTE)
              ? (visualControl - PRESS_MENU_VOL_LARGE)
              : -1);
    } else if (screenMode == SCREEN_GUIDE_MENU) {
      ui.drawGuideMenuScreen(
          visualControl == PRESS_GUIDE_POKEMON,
          visualControl == PRESS_GUIDE_LOCATION,
          visualControl == PRESS_GUIDE_BACK);
    } else if (screenMode == SCREEN_GUIDE_POKEMON_LIST) {
      const auto pageIds = getGuidePokemonPageIds(guidePokemonListOffset, 10);
      std::vector<String> pageLabels;
      pageLabels.reserve(pageIds.size());
      for (uint16_t id : pageIds) {
        char prefix[8];
        snprintf(prefix, sizeof(prefix), "%04d", id);
        pageLabels.push_back(String(prefix) + " " + dataMgr.getPokemonName(id));
      }
      ui.drawGuidePokemonListScreen(
          pageLabels,
          visualControl == PRESS_GUIDE_LIST_BACK,
          (visualControl >= PRESS_GUIDE_LIST_ITEM_0 && visualControl <= PRESS_GUIDE_LIST_ITEM_9)
              ? (visualControl - PRESS_GUIDE_LIST_ITEM_0)
              : -1,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_QUIZ) {
      ui.drawQuizScreen(quizPhase == QUIZ_B_SIDE, quizPokemonId, dataMgr.getPokemonName(quizPokemonId));
    } else if (screenMode == SCREEN_SEARCH) {
      int pressedDigitDelta = 0;
      std::vector<uint16_t> nameCandidateIds;
      std::vector<String> nameCandidateLabels;
      if (visualControl >= PRESS_SEARCH_DIGIT_UP_0 && visualControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = digitStep[visualControl - PRESS_SEARCH_DIGIT_UP_0];
      } else if (visualControl >= PRESS_SEARCH_DIGIT_DOWN_0 && visualControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = -digitStep[visualControl - PRESS_SEARCH_DIGIT_DOWN_0];
      }
      if (searchMode == SEARCH_MODE_NAME) {
        nameCandidateIds = dataMgr.findPokemonIdsByName(searchNameQuery, searchNameOffset, 10);
        for (uint16_t id : nameCandidateIds) {
          char prefix[8];
          snprintf(prefix, sizeof(prefix), "%04d", id);
          nameCandidateLabels.push_back(String(prefix) + " " + dataMgr.getPokemonName(id));
        }
      }
      ui.drawSearchScreen(
          searchMode == SEARCH_MODE_NAME,
          searchId,
          dataMgr.getPokemonName(searchId),
          searchNameQuery,
          nameCandidateIds,
          nameCandidateLabels,
          pressedDigitDelta,
          visualControl == PRESS_SEARCH_MENU,
          visualControl == PRESS_SEARCH_MODE,
          (visualControl >= PRESS_SEARCH_NAME_RESULT_0 && visualControl <= PRESS_SEARCH_NAME_RESULT_9)
              ? (visualControl - PRESS_SEARCH_NAME_RESULT_0)
              : -1,
          visualControl == PRESS_SEARCH_NAME_PAGE_PREV,
          visualControl == PRESS_SEARCH_NAME_PAGE_NEXT,
          visualControl == PRESS_SEARCH_CANCEL,
          visualControl == PRESS_SEARCH_OPEN);
    } else if (screenMode == SCREEN_SEARCH_INPUT) {
      ui.drawSearchInputScreen(
          searchNameQuery,
          searchInputVowelMode,
          getSearchInputRowLabel(searchInputRowIndex),
          visualControl == PRESS_SEARCH_INPUT_BACK,
          visualControl == PRESS_SEARCH_INPUT_CLEAR,
          visualControl == PRESS_SEARCH_INPUT_DELETE,
          (visualControl >= PRESS_SEARCH_INPUT_KEY_0 && visualControl < (PRESS_SEARCH_INPUT_KEY_0 + 12))
              ? (visualControl - PRESS_SEARCH_INPUT_KEY_0)
              : -1);
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
    } else if (screenMode == SCREEN_PREVIEW_POC) {
      const String previewType = pk.types.empty() ? String("ノーマル") : pk.types[0];
      if (ensurePreviewPocCacheReady(currentId, previewType) && previewPocShadowSprite != nullptr && previewPocIconSprite != nullptr) {
        ui.drawPreviewPocScreenLayered(
            previewPocBackgroundSprite,
            *previewPocShadowSprite,
            *previewPocIconSprite,
            previewPocShiftX,
            previewPocShiftY,
            kPreviewPocTransparentColor);
      } else {
        ui.drawPreviewPocScreen(3, previewPocShiftX, previewPocShiftY);
      }
    } else if (screenMode == SCREEN_SLIDESHOW) {
      if (preview3dEnabled) {
        const String previewType = getPrimaryTypeOrNormal(pk);
        if (ensurePreviewPocCacheReady(slideshowPokemonId, previewType) && previewPocShadowSprite != nullptr && previewPocIconSprite != nullptr) {
          ui.drawPreviewPocScreenLayered(
              previewPocBackgroundSprite,
              *previewPocShadowSprite,
              *previewPocIconSprite,
              previewPocShiftX,
              previewPocShiftY,
              kPreviewPocTransparentColor);
        } else {
          ui.drawPreviewPocScreen(slideshowPokemonId, previewPocShiftX, previewPocShiftY);
        }
      } else {
        ui.drawFullscreenPreview(true, slideshowPokemonId);
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
        if (visualControl == PRESS_APPEARANCE_PREVIEW) {
          ui.drawAppearancePreviewFeedback();
        }
      } else if (currentTab == TAB_DESCRIPTION) {
        ui.drawDescriptionTab(pk);
      } else if (currentTab == TAB_BODY) {
        ui.drawBodyTab(pk);
      } else if (currentTab == TAB_ABILITY) {
        ui.drawAbilityTab(pk);
      } else {
        const int totalEvolutionCount = std::min<int>(pk.evolutions.size(), kMaxEvolutionCards);
        ui.drawEvolutionTab(
            pk,
            (visualControl >= PRESS_EVOLUTION_0
                && visualControl <= (PRESS_EVOLUTION_0 + totalEvolutionCount - 1))
                ? (visualControl - PRESS_EVOLUTION_0)
                : -1,
            false);
        if (evolutionSourcePokemonId != pk.id) {
          queueEvolutionImageRequests(pk);
        }
        for (int i = 0; i < totalEvolutionCount; ++i) {
          if (evolutionRequestedIds[i] != 0
              && evolutionRenderedGeneration == evolutionRequestedGeneration
              && evolutionRenderedIds[i] == evolutionRequestedIds[i]
              && evolutionImageSprites[i] != nullptr
              && xSemaphoreTake(evolutionSpriteMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            ui.blitEvolutionImageToCanvas(*evolutionImageSprites[i], i, totalEvolutionCount);
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
