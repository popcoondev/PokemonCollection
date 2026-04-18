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
#include <vector>
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
  SCREEN_LOCKON,
  SCREEN_SETTINGS,
  SCREEN_ACHIEVEMENTS_MENU,
  SCREEN_ACHIEVEMENTS_LOCKON,
  SCREEN_ACHIEVEMENTS_LOCKON_LIST,
  SCREEN_ACHIEVEMENTS_LOCKON_BADGES,
  SCREEN_GUIDE_MENU,
  SCREEN_GUIDE_POKEMON_LIST,
  SCREEN_GUIDE_LOCATION_LIST,
  SCREEN_GUIDE_LOCATION_DETAIL,
  SCREEN_GUIDE_POKEMON_DETAIL,
  SCREEN_DETAIL,
  SCREEN_SEARCH,
  SCREEN_SEARCH_NUMBER,
  SCREEN_SEARCH_INPUT,
  SCREEN_PREVIEW,
  SCREEN_PREVIEW_POC,
  SCREEN_SLIDESHOW,
  SCREEN_QUIZ,
};

ScreenMode screenMode = SCREEN_MENU;
ScreenMode detailReturnScreen = SCREEN_MENU;

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
constexpr const char* kQuizSoundDirJa = "/pokemon/quiz/sounds/ja";
constexpr const char* kQuizSoundDirEn = "/pokemon/quiz/sounds/en";
constexpr const char* kSettingsPath = "/pokemon/settings.json";
constexpr const char* kGuideCaughtPath = "/pokemon/firered/caught.json";
constexpr const char* kLockOnAchievementsPath = "/pokemon/achievements/lockon.json";
constexpr const char* kAchievementStatsPath = "/pokemon/achievements/stats.json";
constexpr const char* kLockOnSecretIdsPath = "/pokemon/lockon/secret_ids.json";
constexpr uint8_t kDisplayBrightnessOn = 128;
constexpr uint8_t kLtr553Address = 0x23;
constexpr uint8_t kLtr553RegPsContr = 0x81;
constexpr uint8_t kLtr553RegPsLed = 0x82;
constexpr uint8_t kLtr553RegPsNPulses = 0x83;
constexpr uint8_t kLtr553RegPsMeasRate = 0x84;
constexpr uint8_t kLtr553RegPartId = 0x86;
constexpr uint8_t kLtr553RegPsDataLow = 0x8D;
constexpr uint8_t kLtr553RegPsDataHigh = 0x8E;
constexpr uint16_t kCoverClosedThreshold = 900;
constexpr uint16_t kCoverOpenThreshold = 650;
constexpr uint32_t kCoverSleepDelayMs = 700;
constexpr uint32_t kCoverPollIntervalMs = 50;
constexpr uint32_t kWakeSplashDurationMs = 2000;
constexpr gpio_num_t kPortBBackButtonPins[] = {GPIO_NUM_8, GPIO_NUM_9};
constexpr uint32_t kPortBBackButtonDebounceMs = 30;
constexpr uint32_t kLockOnReadyDurationMs = 400;
constexpr uint32_t kLockOnConfirmFreezeMs = 140;
constexpr uint32_t kLockOnSuccessDisplayMs = 4000;
constexpr uint32_t kLockOnFailDisplayMs = 3000;
constexpr int kLockOnBarX = 36;
constexpr int kLockOnBarW = SCREEN_WIDTH - 72;
constexpr uint16_t kLockOnRareIds[] = {
    113, 131, 132, 133, 134, 135, 136, 137, 141, 142, 143,
    196, 197, 212, 214, 229, 233, 241, 242, 248, 306, 330,
};
constexpr uint16_t kLockOnEpicIds[] = {
    3, 6, 9, 149, 348, 350, 359, 373, 376, 392, 395,
    398, 407, 445, 448, 462, 468, 470, 471, 474,
};
constexpr uint16_t kLockOnLegendIds[] = {
    144, 145, 146, 243, 244, 245, 249, 250, 251,
    377, 378, 379, 380, 381, 480, 481, 482, 485, 486, 488,
};
constexpr uint16_t kLockOnMythicIds[] = {
    150, 151, 382, 383, 384, 385, 386,
    483, 484, 487, 489, 490, 491, 492, 493,
};

enum QuizPhase {
  QUIZ_A_SIDE = 0,
  QUIZ_B_SIDE,
};

enum SearchMode {
  SEARCH_MODE_NUMBER = 0,
  SEARCH_MODE_NAME,
};

enum LockOnPhase {
  LOCKON_IDLE = 0,
  LOCKON_READY,
  LOCKON_SEARCH,
  LOCKON_LOCKED,
  LOCKON_RESULT_SUCCESS,
  LOCKON_RESULT_FAIL,
};

enum LockOnRarity {
  LOCKON_RARITY_NORMAL = 0,
  LOCKON_RARITY_RARE,
  LOCKON_RARITY_EPIC,
  LOCKON_RARITY_LEGEND,
  LOCKON_RARITY_MYTHIC,
  LOCKON_RARITY_SECRET,
};

enum LockOnBadgeId {
  LOCKON_BADGE_FIRST_LOCK = 0,
  LOCKON_BADGE_NO_MISS,
  LOCKON_BADGE_LOCK_MASTER,
  LOCKON_BADGE_PERFECT_CHASER,
  LOCKON_BADGE_LOCK_HUNTER,
  LOCKON_BADGE_LOCK_COLLECTOR,
  LOCKON_BADGE_RARE_LOCKER,
  LOCKON_BADGE_EPIC_HUNTER,
  LOCKON_BADGE_LEGEND_BREAKER,
  LOCKON_BADGE_MYTHIC_SEEKER,
  LOCKON_BADGE_SECRET_HOLDER,
  LOCKON_BADGE_FEVER_STAR,
  LOCKON_BADGE_FEVER_MASTER,
  LOCKON_BADGE_FLUSTERED,
  LOCKON_BADGE_WHIFF_MASTER,
  LOCKON_BADGE_CAUTIOUS,
  LOCKON_BADGE_COUNT,
};

struct LockOnBadgeDefinition {
  const char* titleJa;
  const char* titleEn;
  const char* descJa;
  const char* descEn;
};

struct LockOnBadgeRow {
  String title;
  String description;
  bool unlocked = false;
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
  String path;
};

struct GuideLocationEntry {
  String id;
  String name;
  bool postgameOnly = false;
  std::vector<uint16_t> pokemonIds;
};

struct GuidePokemonLocationEntry {
  String area;
  String method;
  String rate;
};

struct GuidePokemonMoveEntry {
  int level = 0;
  String name;
};

struct GuidePokemonDetailEntry {
  bool loaded = false;
  std::vector<GuidePokemonLocationEntry> locations;
  std::vector<String> evolutions;
  std::vector<GuidePokemonMoveEntry> levelUpMoves;
  std::vector<String> machines;
  std::vector<String> hms;
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
AppLanguage appLanguage = APP_LANGUAGE_JA;
std::vector<GuideLocationEntry> guideLocations;
std::vector<uint16_t> lockOnSecretIds;
std::vector<uint8_t> lockOnAchievementHighestRarity;
std::vector<bool> pokedexAchievementViewed;
std::vector<bool> slideshowAchievementViewed;
GuidePokemonDetailEntry guidePokemonDetail;
uint16_t guidePokemonSelectedId = 0;
int guidePokemonTab = 0;
int guidePokemonPageIndex = 0;
ScreenMode guidePokemonDetailReturnScreen = SCREEN_GUIDE_POKEMON_LIST;
String guideLocationSelectedName;
std::vector<uint16_t> guideLocationPokemonIds;
size_t guideLocationPokemonListOffset = 0;
std::vector<bool> guideCaughtFlags(387, false);
bool guideCaughtDirty = false;
unsigned long guideCaughtSaveAt = 0;
bool lockOnAchievementsDirty = false;
unsigned long lockOnAchievementsSaveAt = 0;
bool achievementStatsDirty = false;
unsigned long achievementStatsSaveAt = 0;
uint32_t quizQuestionsViewedCount = 0;
uint32_t lockOnTotalSuccessCount = 0;
uint32_t lockOnTotalFailCount = 0;
uint32_t lockOnCurrentSuccessStreak = 0;
uint32_t lockOnMaxSuccessStreak = 0;
uint32_t lockOnCurrentFailStreak = 0;
uint32_t lockOnMaxFailStreak = 0;
uint32_t lockOnRarityCaptureCounts[LOCKON_RARITY_SECRET + 1] = {};
uint32_t lockOnTotalFeverSuccessCount = 0;
uint32_t lockOnCurrentFeverSuccessCount = 0;
uint32_t lockOnMaxFeverSuccessCount = 0;
uint32_t lockOnCurrentNormalCaptureStreak = 0;
uint32_t lockOnMaxNormalCaptureStreak = 0;
uint32_t lockOnCurrentOutsideMissStreak = 0;
uint32_t lockOnMaxOutsideMissStreak = 0;
bool lockOnBadgeFeverSessionActive = false;
bool preview3dEnabled = false;
bool previewCaptionEnabled = true;
bool guideHallOfFameEnabled = false;
UIThemeStyle uiThemeStyle = UI_THEME_CLASSIC;
int settingsTabIndex = 0;
bool settingsDirty = false;
unsigned long settingsSaveAt = 0;
bool coverProximityReady = false;
bool coverDisplaySleeping = false;
bool wakeSplashActive = false;
unsigned long wakeSplashStartedAt = 0;
unsigned long coverClosedSince = 0;
unsigned long coverLastPollAt = 0;
uint16_t coverLastProximityValue = 0;
bool portBBackButtonStablePressed = false;
bool portBBackButtonLastRawPressed = false;
unsigned long portBBackButtonLastChangeAt = 0;
bool portBBackButtonCapturedMarkerValid = false;
int portBBackButtonCapturedMarkerX = kLockOnBarX + (kLockOnBarW / 2);
SearchMode searchMode = SEARCH_MODE_NUMBER;
size_t searchNameOffset = 0;
String searchNameQuery = "";
bool searchInputVowelMode = false;
int searchInputRowIndex = -1;
unsigned long searchFlashUntil = 0;
uint16_t slideshowPokemonId = MIN_POKEMON_ID;
uint32_t slideshowPhaseStartedAt = 0;
LockOnPhase lockOnPhase = LOCKON_IDLE;
LockOnRarity lockOnRarity = LOCKON_RARITY_NORMAL;
uint16_t lockOnPokemonId = MIN_POKEMON_ID;
uint16_t lockOnLastPokemonId = 0;
uint32_t lockOnPhaseStartedAt = 0;
uint32_t lockOnSearchDurationMs = 3000;
uint16_t lockOnZoneWidth = 48;
int lockOnZoneCenterX = kLockOnBarX + (kLockOnBarW / 2);
int lockOnConfirmedMarkerX = kLockOnBarX + (kLockOnBarW / 2);
bool lockOnPendingSuccess = false;
uint32_t lockOnLastPulseAt = 0;
uint32_t lockOnCapturedCount = 0;
uint32_t lockOnNextFeverCaptureCount = 10;
uint32_t lockOnFeverEndsAt = 0;
size_t lockOnAchievementsListOffset = 0;
size_t lockOnBadgeListOffset = 0;
unsigned long vibrationStopAt = 0;

uint16_t getAvailableMaxPokemonId();

bool hitTest(int tx, int ty, int x, int y, int w, int h, int pad = 0) {
  return tx >= (x - pad) && tx <= (x + w + pad)
      && ty >= (y - pad) && ty <= (y + h + pad);
}

bool initCoverProximitySensor() {
  const uint8_t partId = M5.In_I2C.readRegister8(kLtr553Address, kLtr553RegPartId, 400000);
  if (partId == 0x00 || partId == 0xFF) {
    return false;
  }

  if (!M5.In_I2C.writeRegister8(kLtr553Address, kLtr553RegPsLed, 0x3F, 400000)) return false;
  if (!M5.In_I2C.writeRegister8(kLtr553Address, kLtr553RegPsNPulses, 0x04, 400000)) return false;
  if (!M5.In_I2C.writeRegister8(kLtr553Address, kLtr553RegPsMeasRate, 0x02, 400000)) return false;
  if (!M5.In_I2C.writeRegister8(kLtr553Address, kLtr553RegPsContr, 0x03, 400000)) return false;
  delay(10);
  return true;
}

uint16_t readCoverProximityValue() {
  uint8_t valueLow = 0;
  uint8_t valueHigh = 0;
  if (!M5.In_I2C.readRegister(kLtr553Address, kLtr553RegPsDataLow, &valueLow, 1, 400000)) {
    return coverLastProximityValue;
  }
  if (!M5.In_I2C.readRegister(kLtr553Address, kLtr553RegPsDataHigh, &valueHigh, 1, 400000)) {
    return coverLastProximityValue;
  }
  return static_cast<uint16_t>(((valueHigh & 0x07) << 8) | valueLow);
}

void initPortBBackButton() {
  for (gpio_num_t pin : kPortBBackButtonPins) {
    pinMode(static_cast<int>(pin), INPUT_PULLUP);
  }
}

bool readPortBBackButtonRawPressed() {
  for (gpio_num_t pin : kPortBBackButtonPins) {
    if (digitalRead(static_cast<int>(pin)) == LOW) {
      return true;
    }
  }
  return false;
}

void triggerVibration(uint8_t level, uint32_t durationMs, unsigned long now) {
  M5.Power.setVibration(level);
  vibrationStopAt = (level == 0 || durationMs == 0) ? 0 : (now + durationMs);
}

bool isLockOnFeverActive(unsigned long now) {
  return lockOnFeverEndsAt > now;
}

uint32_t getLockOnFeverRemainingMs(unsigned long now) {
  return isLockOnFeverActive(now) ? (lockOnFeverEndsAt - now) : 0;
}

void startLockOnFever(unsigned long now) {
  lockOnFeverEndsAt = now + static_cast<uint32_t>(random(60000UL, 120001UL));
}

void recordLockOnCapture(unsigned long now) {
  if (isLockOnFeverActive(now)) {
    return;
  }
  lockOnCapturedCount += 1;
  while (lockOnCapturedCount >= lockOnNextFeverCaptureCount) {
    startLockOnFever(now);
    lockOnNextFeverCaptureCount += 10;
  }
}

LockOnRarity chooseLockOnRarity(bool feverActive) {
  const uint32_t roll = esp_random() % 100;
  if (feverActive) {
    if (roll < 9) return LOCKON_RARITY_SECRET;
    if (roll < 34) return LOCKON_RARITY_MYTHIC;
    if (roll < 99) return LOCKON_RARITY_LEGEND;
    return LOCKON_RARITY_EPIC;
  }
  if (roll < 1) return LOCKON_RARITY_SECRET;
  if (roll < 2) return LOCKON_RARITY_MYTHIC;
  if (roll < 5) return LOCKON_RARITY_LEGEND;
  if (roll < 12) return LOCKON_RARITY_EPIC;
  if (roll < 28) return LOCKON_RARITY_RARE;
  return LOCKON_RARITY_NORMAL;
}

bool isPokemonIdInTable(uint16_t pokemonId, const uint16_t* ids, size_t count) {
  if (ids == nullptr) return false;
  for (size_t i = 0; i < count; ++i) {
    if (ids[i] == pokemonId) return true;
  }
  return false;
}

bool isPokemonIdInVector(uint16_t pokemonId, const std::vector<uint16_t>& ids) {
  for (uint16_t id : ids) {
    if (id == pokemonId) return true;
  }
  return false;
}

bool isLockOnSpecialPokemonId(uint16_t pokemonId) {
  return isPokemonIdInTable(pokemonId, kLockOnRareIds, sizeof(kLockOnRareIds) / sizeof(kLockOnRareIds[0]))
      || isPokemonIdInTable(pokemonId, kLockOnEpicIds, sizeof(kLockOnEpicIds) / sizeof(kLockOnEpicIds[0]))
      || isPokemonIdInTable(pokemonId, kLockOnLegendIds, sizeof(kLockOnLegendIds) / sizeof(kLockOnLegendIds[0]))
      || isPokemonIdInTable(pokemonId, kLockOnMythicIds, sizeof(kLockOnMythicIds) / sizeof(kLockOnMythicIds[0]))
      || isPokemonIdInVector(pokemonId, lockOnSecretIds);
}

uint16_t chooseRandomPokemonIdExcluding(uint16_t excludedId) {
  const uint16_t maxPokemonId = getAvailableMaxPokemonId();
  if (maxPokemonId <= MIN_POKEMON_ID) {
    return MIN_POKEMON_ID;
  }

  for (int attempt = 0; attempt < 64; ++attempt) {
    const uint16_t candidate = static_cast<uint16_t>(MIN_POKEMON_ID + (esp_random() % (maxPokemonId - MIN_POKEMON_ID + 1)));
    if (candidate == excludedId) continue;
    if (dataMgr.getPokemonName(candidate).length() == 0) continue;
    return candidate;
  }

  return MIN_POKEMON_ID;
}

uint16_t chooseRandomNormalPokemonId(uint16_t excludedId) {
  const uint16_t maxPokemonId = getAvailableMaxPokemonId();
  if (maxPokemonId <= MIN_POKEMON_ID) {
    return MIN_POKEMON_ID;
  }

  for (int attempt = 0; attempt < 128; ++attempt) {
    const uint16_t candidate = static_cast<uint16_t>(MIN_POKEMON_ID + (esp_random() % (maxPokemonId - MIN_POKEMON_ID + 1)));
    if (candidate == excludedId) continue;
    if (isLockOnSpecialPokemonId(candidate)) continue;
    if (dataMgr.getPokemonName(candidate).length() == 0) continue;
    return candidate;
  }

  return chooseRandomPokemonIdExcluding(excludedId);
}

uint16_t choosePokemonIdFromTable(const uint16_t* ids, size_t count, uint16_t excludedId) {
  if (ids == nullptr || count == 0) {
    return chooseRandomPokemonIdExcluding(excludedId);
  }
  for (int attempt = 0; attempt < 64; ++attempt) {
    const uint16_t candidate = ids[esp_random() % count];
    if (candidate == excludedId) continue;
    if (dataMgr.getPokemonName(candidate).length() == 0) continue;
    return candidate;
  }
  return chooseRandomPokemonIdExcluding(excludedId);
}

uint16_t choosePokemonIdFromTableExcludingSecret(const uint16_t* ids, size_t count, uint16_t excludedId) {
  if (ids == nullptr || count == 0) {
    return chooseRandomPokemonIdExcluding(excludedId);
  }
  for (int attempt = 0; attempt < 64; ++attempt) {
    const uint16_t candidate = ids[esp_random() % count];
    if (candidate == excludedId) continue;
    if (isPokemonIdInVector(candidate, lockOnSecretIds)) continue;
    if (dataMgr.getPokemonName(candidate).length() == 0) continue;
    return candidate;
  }
  return chooseRandomPokemonIdExcluding(excludedId);
}

uint16_t choosePokemonIdFromVector(const std::vector<uint16_t>& ids, uint16_t excludedId) {
  if (ids.empty()) {
    return chooseRandomPokemonIdExcluding(excludedId);
  }
  for (int attempt = 0; attempt < 64; ++attempt) {
    const uint16_t candidate = ids[esp_random() % ids.size()];
    if (candidate == excludedId) continue;
    if (dataMgr.getPokemonName(candidate).length() == 0) continue;
    return candidate;
  }
  return chooseRandomPokemonIdExcluding(excludedId);
}

uint16_t chooseLockOnPokemonId(LockOnRarity rarity, uint16_t excludedId) {
  switch (rarity) {
    case LOCKON_RARITY_SECRET:
      if (!lockOnSecretIds.empty()) {
        return choosePokemonIdFromVector(lockOnSecretIds, excludedId);
      }
      return choosePokemonIdFromTableExcludingSecret(kLockOnMythicIds, sizeof(kLockOnMythicIds) / sizeof(kLockOnMythicIds[0]), excludedId);
    case LOCKON_RARITY_MYTHIC:
      return choosePokemonIdFromTableExcludingSecret(kLockOnMythicIds, sizeof(kLockOnMythicIds) / sizeof(kLockOnMythicIds[0]), excludedId);
    case LOCKON_RARITY_LEGEND:
      return choosePokemonIdFromTableExcludingSecret(kLockOnLegendIds, sizeof(kLockOnLegendIds) / sizeof(kLockOnLegendIds[0]), excludedId);
    case LOCKON_RARITY_EPIC:
      return choosePokemonIdFromTableExcludingSecret(kLockOnEpicIds, sizeof(kLockOnEpicIds) / sizeof(kLockOnEpicIds[0]), excludedId);
    case LOCKON_RARITY_RARE:
      return choosePokemonIdFromTableExcludingSecret(kLockOnRareIds, sizeof(kLockOnRareIds) / sizeof(kLockOnRareIds[0]), excludedId);
    case LOCKON_RARITY_NORMAL:
    default:
      return chooseRandomNormalPokemonId(excludedId);
  }
}

const char* getLockOnRarityLabel(LockOnRarity rarity) {
  switch (rarity) {
    case LOCKON_RARITY_SECRET:
      return "SECRET";
    case LOCKON_RARITY_MYTHIC:
      return "MYTHIC";
    case LOCKON_RARITY_LEGEND:
      return "LEGEND";
    case LOCKON_RARITY_EPIC:
      return "EPIC";
    case LOCKON_RARITY_RARE:
      return "RARE";
    case LOCKON_RARITY_NORMAL:
    default:
      return "NORMAL";
  }
}

void playLockOnResultSound(bool success, LockOnRarity rarity) {
  M5.Speaker.stop();
  if (!success) {
    M5.Speaker.tone(420, 90, 0, true);
    M5.Speaker.tone(280, 180, 0, false);
    return;
  }

  switch (rarity) {
    case LOCKON_RARITY_SECRET:
      M5.Speaker.tone(1200, 40, 0, true);
      M5.Speaker.tone(1700, 50, 0, false);
      M5.Speaker.tone(2100, 60, 0, false);
      M5.Speaker.tone(2600, 70, 0, false);
      M5.Speaker.tone(3200, 220, 0, false);
      break;
    case LOCKON_RARITY_MYTHIC:
      M5.Speaker.tone(1100, 50, 0, true);
      M5.Speaker.tone(1480, 60, 0, false);
      M5.Speaker.tone(1860, 70, 0, false);
      M5.Speaker.tone(2340, 180, 0, false);
      break;
    case LOCKON_RARITY_LEGEND:
      M5.Speaker.tone(1320, 70, 0, true);
      M5.Speaker.tone(1760, 80, 0, false);
      M5.Speaker.tone(2210, 110, 0, false);
      M5.Speaker.tone(2640, 220, 0, false);
      break;
    case LOCKON_RARITY_EPIC:
      M5.Speaker.tone(980, 60, 0, true);
      M5.Speaker.tone(1320, 70, 0, false);
      M5.Speaker.tone(1640, 80, 0, false);
      M5.Speaker.tone(1980, 150, 0, false);
      break;
    case LOCKON_RARITY_RARE:
      M5.Speaker.tone(1040, 70, 0, true);
      M5.Speaker.tone(1390, 80, 0, false);
      M5.Speaker.tone(1760, 180, 0, false);
      break;
    case LOCKON_RARITY_NORMAL:
    default:
      M5.Speaker.tone(880, 80, 0, true);
      M5.Speaker.tone(1320, 180, 0, false);
      break;
  }
}

uint32_t getLockOnSearchDurationMs(LockOnRarity rarity) {
  (void)rarity;
  return 3800;
}

uint16_t getLockOnZoneWidth(LockOnRarity rarity) {
  (void)rarity;
  return static_cast<uint16_t>(random(30, 47));
}

float getLockOnMarkerRatio(unsigned long elapsedMs, uint32_t durationMs, LockOnRarity rarity) {
  if (durationMs == 0) {
    return 0.5f;
  }
  const float t = constrain(static_cast<float>(elapsedMs) / static_cast<float>(durationMs), 0.0f, 1.0f);
  (void)rarity;
  const float baseSweep = 2.0f;
  const float accelSweep = 1.4f;
  const float phase = (t * baseSweep) + ((t * t) * accelSweep);
  const float wrapped = phase - floorf(phase);
  return (wrapped <= 0.5f) ? (wrapped * 2.0f) : (2.0f - (wrapped * 2.0f));
}

int chooseLockOnZoneCenterX(uint16_t zoneWidth) {
  const int half = zoneWidth / 2;
  const int minCenter = kLockOnBarX + half + 6;
  const int maxCenter = kLockOnBarX + kLockOnBarW - half - 6;
  if (maxCenter <= minCenter) {
    return kLockOnBarX + (kLockOnBarW / 2);
  }
  return random(minCenter, maxCenter + 1);
}

int getLockOnMarkerCenterX(unsigned long elapsedMs, uint32_t durationMs, LockOnRarity rarity) {
  const float ratio = getLockOnMarkerRatio(elapsedMs, durationMs, rarity);
  return kLockOnBarX + static_cast<int>(lroundf(ratio * static_cast<float>(kLockOnBarW - 1)));
}

size_t getLockOnAchievementCaughtCount() {
  size_t count = 0;
  for (size_t id = 1; id < lockOnAchievementHighestRarity.size(); ++id) {
    if (lockOnAchievementHighestRarity[id] != 0xFF) {
      count += 1;
    }
  }
  return count;
}

void ensureAchievementStatsStorageReady() {
  const size_t requiredSize = static_cast<size_t>(getAvailableMaxPokemonId()) + 1;
  if (pokedexAchievementViewed.size() != requiredSize) {
    pokedexAchievementViewed.assign(requiredSize, false);
  }
  if (slideshowAchievementViewed.size() != requiredSize) {
    slideshowAchievementViewed.assign(requiredSize, false);
  }
}

size_t countTrueFlags(const std::vector<bool>& flags) {
  size_t count = 0;
  for (bool flag : flags) {
    if (flag) count += 1;
  }
  return count;
}

const char* trApp(const char* ja, const char* en) {
  return (appLanguage == APP_LANGUAGE_EN) ? en : ja;
}

const LockOnBadgeDefinition kLockOnBadgeDefinitions[LOCKON_BADGE_COUNT] = {
    {"はじめてのロック", "First Lock", "ロックオンに1回成功", "Succeed once in Lock-On"},
    {"ミスしらず", "No Miss", "1回も失敗せずに5回成功", "Get 5 successes without a miss"},
    {"ロックマスター", "Lock Master", "連続で10回ロックオン成功", "Get 10 consecutive Lock-On wins"},
    {"パーフェクトチェイサー", "Perfect Chaser", "連続で25回ロックオン成功", "Get 25 consecutive Lock-On wins"},
    {"ロックハンター", "Lock Hunter", "ロックオンに30回成功", "Get 30 Lock-On wins"},
    {"ロックコレクター", "Lock Collector", "ロックオンに100回成功", "Get 100 Lock-On wins"},
    {"レアロッカー", "Rare Locker", "RAREを20ひき獲得", "Catch 20 RARE Pokemon"},
    {"エピックハンター", "Epic Hunter", "EPICを12ひき獲得", "Catch 12 EPIC Pokemon"},
    {"レジェンドブレイカー", "Legend Breaker", "LEGENDを8ひき獲得", "Catch 8 LEGEND Pokemon"},
    {"ミシックシーカー", "Mythic Seeker", "MYTHICを5ひき獲得", "Catch 5 MYTHIC Pokemon"},
    {"シークレットホルダー", "Secret Holder", "SECRETを2ひき獲得", "Catch 2 SECRET Pokemon"},
    {"フィーバースター", "Fever Star", "フィーバータイム中に5回成功", "Get 5 FEVER successes"},
    {"フィーバーマスター", "Fever Master", "1回のフィーバーで10回成功", "Get 10 wins in one FEVER"},
    {"あわてんぼう", "Flustered", "ハイライトの外で5回れんぞく失敗", "Miss outside the zone 5 times in a row"},
    {"からぶりめいじん", "Whiff Master", "ロックオンに20回失敗", "Fail Lock-On 20 times"},
    {"びびりや", "Cautious", "10回連続でNORMALだけ獲得", "Catch only NORMAL 10 times in a row"},
};

void markAchievementStatsDirty(unsigned long now) {
  achievementStatsDirty = true;
  achievementStatsSaveAt = now + 500;
}

void resetLockOnBadgeSessionState() {
  lockOnBadgeFeverSessionActive = false;
  lockOnCurrentFeverSuccessCount = 0;
}

void updateLockOnBadgeFeverState(unsigned long now) {
  const bool feverActive = isLockOnFeverActive(now);
  if (feverActive && !lockOnBadgeFeverSessionActive) {
    lockOnBadgeFeverSessionActive = true;
    lockOnCurrentFeverSuccessCount = 0;
  } else if (!feverActive && lockOnBadgeFeverSessionActive) {
    lockOnBadgeFeverSessionActive = false;
    lockOnCurrentFeverSuccessCount = 0;
  }
}

void recordLockOnBadgeSuccess(LockOnRarity rarity, bool feverActive, unsigned long now) {
  lockOnTotalSuccessCount += 1;
  lockOnCurrentSuccessStreak += 1;
  lockOnMaxSuccessStreak = max(lockOnMaxSuccessStreak, lockOnCurrentSuccessStreak);
  lockOnCurrentFailStreak = 0;
  lockOnCurrentOutsideMissStreak = 0;
  if (rarity >= LOCKON_RARITY_NORMAL && rarity <= LOCKON_RARITY_SECRET) {
    lockOnRarityCaptureCounts[rarity] += 1;
  }
  if (rarity == LOCKON_RARITY_NORMAL) {
    lockOnCurrentNormalCaptureStreak += 1;
    lockOnMaxNormalCaptureStreak = max(lockOnMaxNormalCaptureStreak, lockOnCurrentNormalCaptureStreak);
  } else {
    lockOnCurrentNormalCaptureStreak = 0;
  }
  if (feverActive) {
    lockOnTotalFeverSuccessCount += 1;
    if (!lockOnBadgeFeverSessionActive) {
      lockOnBadgeFeverSessionActive = true;
      lockOnCurrentFeverSuccessCount = 0;
    }
    lockOnCurrentFeverSuccessCount += 1;
    lockOnMaxFeverSuccessCount = max(lockOnMaxFeverSuccessCount, lockOnCurrentFeverSuccessCount);
  }
  markAchievementStatsDirty(now);
}

void recordLockOnBadgeFailure(bool outsideZoneMiss, unsigned long now) {
  lockOnTotalFailCount += 1;
  lockOnCurrentSuccessStreak = 0;
  lockOnCurrentFailStreak += 1;
  lockOnMaxFailStreak = max(lockOnMaxFailStreak, lockOnCurrentFailStreak);
  lockOnCurrentNormalCaptureStreak = 0;
  if (outsideZoneMiss) {
    lockOnCurrentOutsideMissStreak += 1;
    lockOnMaxOutsideMissStreak = max(lockOnMaxOutsideMissStreak, lockOnCurrentOutsideMissStreak);
  } else {
    lockOnCurrentOutsideMissStreak = 0;
  }
  markAchievementStatsDirty(now);
}

bool isLockOnBadgeUnlocked(LockOnBadgeId badgeId) {
  switch (badgeId) {
    case LOCKON_BADGE_FIRST_LOCK:
      return lockOnTotalSuccessCount >= 1;
    case LOCKON_BADGE_NO_MISS:
      return lockOnMaxSuccessStreak >= 5;
    case LOCKON_BADGE_LOCK_MASTER:
      return lockOnMaxSuccessStreak >= 10;
    case LOCKON_BADGE_PERFECT_CHASER:
      return lockOnMaxSuccessStreak >= 25;
    case LOCKON_BADGE_LOCK_HUNTER:
      return lockOnTotalSuccessCount >= 30;
    case LOCKON_BADGE_LOCK_COLLECTOR:
      return lockOnTotalSuccessCount >= 100;
    case LOCKON_BADGE_RARE_LOCKER:
      return lockOnRarityCaptureCounts[LOCKON_RARITY_RARE] >= 20;
    case LOCKON_BADGE_EPIC_HUNTER:
      return lockOnRarityCaptureCounts[LOCKON_RARITY_EPIC] >= 12;
    case LOCKON_BADGE_LEGEND_BREAKER:
      return lockOnRarityCaptureCounts[LOCKON_RARITY_LEGEND] >= 8;
    case LOCKON_BADGE_MYTHIC_SEEKER:
      return lockOnRarityCaptureCounts[LOCKON_RARITY_MYTHIC] >= 5;
    case LOCKON_BADGE_SECRET_HOLDER:
      return lockOnRarityCaptureCounts[LOCKON_RARITY_SECRET] >= 2;
    case LOCKON_BADGE_FEVER_STAR:
      return lockOnTotalFeverSuccessCount >= 5;
    case LOCKON_BADGE_FEVER_MASTER:
      return lockOnMaxFeverSuccessCount >= 10;
    case LOCKON_BADGE_FLUSTERED:
      return lockOnMaxOutsideMissStreak >= 5;
    case LOCKON_BADGE_WHIFF_MASTER:
      return lockOnTotalFailCount >= 20;
    case LOCKON_BADGE_CAUTIOUS:
      return lockOnMaxNormalCaptureStreak >= 10;
    case LOCKON_BADGE_COUNT:
    default:
      return false;
  }
}

size_t getUnlockedLockOnBadgeCount() {
  size_t count = 0;
  for (int badgeId = 0; badgeId < LOCKON_BADGE_COUNT; ++badgeId) {
    if (isLockOnBadgeUnlocked(static_cast<LockOnBadgeId>(badgeId))) {
      count += 1;
    }
  }
  return count;
}

std::vector<LockOnBadgeRow> buildLockOnBadgeRows() {
  std::vector<LockOnBadgeRow> rows;
  rows.reserve(LOCKON_BADGE_COUNT);
  for (int badgeId = 0; badgeId < LOCKON_BADGE_COUNT; ++badgeId) {
    const auto& def = kLockOnBadgeDefinitions[badgeId];
    LockOnBadgeRow row;
    row.title = trApp(def.titleJa, def.titleEn);
    row.description = trApp(def.descJa, def.descEn);
    row.unlocked = isLockOnBadgeUnlocked(static_cast<LockOnBadgeId>(badgeId));
    rows.push_back(row);
  }
  return rows;
}

std::vector<LockOnBadgeRow> getLockOnBadgePageRows(size_t offset, size_t pageSize) {
  const auto rows = buildLockOnBadgeRows();
  if (offset >= rows.size()) return {};
  const size_t end = std::min(rows.size(), offset + pageSize);
  return std::vector<LockOnBadgeRow>(rows.begin() + offset, rows.begin() + end);
}

size_t getGuideCaughtCount() {
  size_t count = 0;
  for (int id = 1; id <= 386 && id < static_cast<int>(guideCaughtFlags.size()); ++id) {
    if (guideCaughtFlags[id]) count += 1;
  }
  return count;
}

bool saveAchievementStats() {
  ensureAchievementStatsStorageReady();
  SD.mkdir("/pokemon");
  SD.mkdir("/pokemon/achievements");
  JsonDocument doc;
  doc["quiz_questions_viewed"] = quizQuestionsViewedCount;
  JsonObject lockOnStats = doc["lockon"].to<JsonObject>();
  lockOnStats["total_successes"] = lockOnTotalSuccessCount;
  lockOnStats["total_fails"] = lockOnTotalFailCount;
  lockOnStats["current_success_streak"] = lockOnCurrentSuccessStreak;
  lockOnStats["max_success_streak"] = lockOnMaxSuccessStreak;
  lockOnStats["current_fail_streak"] = lockOnCurrentFailStreak;
  lockOnStats["max_fail_streak"] = lockOnMaxFailStreak;
  lockOnStats["total_fever_successes"] = lockOnTotalFeverSuccessCount;
  lockOnStats["max_fever_successes"] = lockOnMaxFeverSuccessCount;
  lockOnStats["current_normal_streak"] = lockOnCurrentNormalCaptureStreak;
  lockOnStats["max_normal_streak"] = lockOnMaxNormalCaptureStreak;
  lockOnStats["current_outside_miss_streak"] = lockOnCurrentOutsideMissStreak;
  lockOnStats["max_outside_miss_streak"] = lockOnMaxOutsideMissStreak;
  JsonArray rarityCounts = lockOnStats["rarity_counts"].to<JsonArray>();
  for (int rarity = LOCKON_RARITY_NORMAL; rarity <= LOCKON_RARITY_SECRET; ++rarity) {
    rarityCounts.add(lockOnRarityCaptureCounts[rarity]);
  }

  JsonArray pokedexArr = doc["pokedex_viewed"].to<JsonArray>();
  for (size_t id = 1; id < pokedexAchievementViewed.size(); ++id) {
    if (pokedexAchievementViewed[id]) pokedexArr.add(static_cast<uint16_t>(id));
  }

  JsonArray slideshowArr = doc["slideshow_viewed"].to<JsonArray>();
  for (size_t id = 1; id < slideshowAchievementViewed.size(); ++id) {
    if (slideshowAchievementViewed[id]) slideshowArr.add(static_cast<uint16_t>(id));
  }

  if (SD.exists(kAchievementStatsPath)) {
    SD.remove(kAchievementStatsPath);
  }
  File file = SD.open(kAchievementStatsPath, FILE_WRITE);
  if (!file) return false;
  const bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

bool loadAchievementStats() {
  ensureAchievementStatsStorageReady();
  quizQuestionsViewedCount = 0;
  lockOnTotalSuccessCount = 0;
  lockOnTotalFailCount = 0;
  lockOnCurrentSuccessStreak = 0;
  lockOnMaxSuccessStreak = 0;
  lockOnCurrentFailStreak = 0;
  lockOnMaxFailStreak = 0;
  for (uint32_t& count : lockOnRarityCaptureCounts) {
    count = 0;
  }
  lockOnTotalFeverSuccessCount = 0;
  lockOnCurrentFeverSuccessCount = 0;
  lockOnMaxFeverSuccessCount = 0;
  lockOnCurrentNormalCaptureStreak = 0;
  lockOnMaxNormalCaptureStreak = 0;
  lockOnCurrentOutsideMissStreak = 0;
  lockOnMaxOutsideMissStreak = 0;
  resetLockOnBadgeSessionState();
  File file = SD.open(kAchievementStatsPath, FILE_READ);
  if (!file) return true;

  JsonDocument doc;
  if (deserializeJson(doc, file) != DeserializationError::Ok) {
    file.close();
    return false;
  }
  file.close();

  quizQuestionsViewedCount = doc["quiz_questions_viewed"] | 0;
  JsonObject lockOnStats = doc["lockon"].as<JsonObject>();
  if (!lockOnStats.isNull()) {
    lockOnTotalSuccessCount = lockOnStats["total_successes"] | 0;
    lockOnTotalFailCount = lockOnStats["total_fails"] | 0;
    lockOnCurrentSuccessStreak = lockOnStats["current_success_streak"] | 0;
    lockOnMaxSuccessStreak = lockOnStats["max_success_streak"] | 0;
    lockOnCurrentFailStreak = lockOnStats["current_fail_streak"] | 0;
    lockOnMaxFailStreak = lockOnStats["max_fail_streak"] | 0;
    lockOnTotalFeverSuccessCount = lockOnStats["total_fever_successes"] | 0;
    lockOnMaxFeverSuccessCount = lockOnStats["max_fever_successes"] | 0;
    lockOnCurrentNormalCaptureStreak = lockOnStats["current_normal_streak"] | 0;
    lockOnMaxNormalCaptureStreak = lockOnStats["max_normal_streak"] | 0;
    lockOnCurrentOutsideMissStreak = lockOnStats["current_outside_miss_streak"] | 0;
    lockOnMaxOutsideMissStreak = lockOnStats["max_outside_miss_streak"] | 0;
    JsonArray rarityCounts = lockOnStats["rarity_counts"].as<JsonArray>();
    if (!rarityCounts.isNull()) {
      int rarity = LOCKON_RARITY_NORMAL;
      for (JsonVariant v : rarityCounts) {
        if (rarity > LOCKON_RARITY_SECRET) break;
        lockOnRarityCaptureCounts[rarity] = v.as<uint32_t>();
        rarity += 1;
      }
    }
  }
  JsonArray pokedexArr = doc["pokedex_viewed"].as<JsonArray>();
  if (!pokedexArr.isNull()) {
    for (JsonVariant v : pokedexArr) {
      const uint16_t id = v.as<uint16_t>();
      if (id < pokedexAchievementViewed.size()) pokedexAchievementViewed[id] = true;
    }
  }
  JsonArray slideshowArr = doc["slideshow_viewed"].as<JsonArray>();
  if (!slideshowArr.isNull()) {
    for (JsonVariant v : slideshowArr) {
      const uint16_t id = v.as<uint16_t>();
      if (id < slideshowAchievementViewed.size()) slideshowAchievementViewed[id] = true;
    }
  }
  return true;
}

void recordPokedexAchievementView(uint16_t pokemonId, unsigned long now) {
  ensureAchievementStatsStorageReady();
  if (pokemonId >= pokedexAchievementViewed.size()) return;
  if (!pokedexAchievementViewed[pokemonId]) {
    pokedexAchievementViewed[pokemonId] = true;
    markAchievementStatsDirty(now);
  }
}

void recordSlideshowAchievementView(uint16_t pokemonId, unsigned long now) {
  ensureAchievementStatsStorageReady();
  if (pokemonId >= slideshowAchievementViewed.size()) return;
  if (!slideshowAchievementViewed[pokemonId]) {
    slideshowAchievementViewed[pokemonId] = true;
    markAchievementStatsDirty(now);
  }
}

void recordQuizQuestionViewed(unsigned long now) {
  quizQuestionsViewedCount += 1;
  markAchievementStatsDirty(now);
}

void ensureLockOnAchievementsStorageReady() {
  const size_t requiredSize = static_cast<size_t>(getAvailableMaxPokemonId()) + 1;
  if (lockOnAchievementHighestRarity.size() != requiredSize) {
    lockOnAchievementHighestRarity.assign(requiredSize, 0xFF);
  }
}

bool saveLockOnAchievements() {
  ensureLockOnAchievementsStorageReady();
  SD.mkdir("/pokemon");
  SD.mkdir("/pokemon/achievements");
  JsonDocument doc;
  JsonArray arr = doc["entries"].to<JsonArray>();
  for (size_t id = 1; id < lockOnAchievementHighestRarity.size(); ++id) {
    const uint8_t rarity = lockOnAchievementHighestRarity[id];
    if (rarity == 0xFF) continue;
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = static_cast<uint16_t>(id);
    obj["rarity"] = rarity;
  }

  if (SD.exists(kLockOnAchievementsPath)) {
    SD.remove(kLockOnAchievementsPath);
  }
  File file = SD.open(kLockOnAchievementsPath, FILE_WRITE);
  if (!file) {
    return false;
  }
  const bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

bool loadLockOnAchievements() {
  ensureLockOnAchievementsStorageReady();
  File file = SD.open(kLockOnAchievementsPath, FILE_READ);
  if (!file) {
    return true;
  }

  JsonDocument doc;
  if (deserializeJson(doc, file) != DeserializationError::Ok) {
    file.close();
    return false;
  }
  file.close();

  JsonArray entries = doc["entries"].as<JsonArray>();
  if (entries.isNull()) {
    return true;
  }
  for (JsonVariant v : entries) {
    const uint16_t id = v["id"] | 0;
    const int rarity = v["rarity"] | -1;
    if (id >= lockOnAchievementHighestRarity.size()) continue;
    if (rarity < 0 || rarity > LOCKON_RARITY_SECRET) continue;
    lockOnAchievementHighestRarity[id] = static_cast<uint8_t>(rarity);
  }
  return true;
}

void recordLockOnAchievement(uint16_t pokemonId, LockOnRarity rarity, unsigned long now) {
  ensureLockOnAchievementsStorageReady();
  if (pokemonId >= lockOnAchievementHighestRarity.size()) {
    return;
  }
  const uint8_t current = lockOnAchievementHighestRarity[pokemonId];
  if (current == 0xFF || static_cast<uint8_t>(rarity) > current) {
    lockOnAchievementHighestRarity[pokemonId] = static_cast<uint8_t>(rarity);
    lockOnAchievementsDirty = true;
    lockOnAchievementsSaveAt = now + 500;
  }
}

struct LockOnAchievementListRow {
  String label;
  bool header = false;
};

std::vector<LockOnAchievementListRow> buildLockOnAchievementRows() {
  ensureLockOnAchievementsStorageReady();
  std::vector<LockOnAchievementListRow> rows;
  for (int rarity = LOCKON_RARITY_SECRET; rarity >= LOCKON_RARITY_NORMAL; --rarity) {
    std::vector<uint16_t> ids;
    for (size_t id = 1; id < lockOnAchievementHighestRarity.size(); ++id) {
      if (lockOnAchievementHighestRarity[id] == rarity) {
        ids.push_back(static_cast<uint16_t>(id));
      }
    }
    if (ids.empty()) continue;

    LockOnAchievementListRow headerRow;
    headerRow.label = String(getLockOnRarityLabel(static_cast<LockOnRarity>(rarity)));
    headerRow.header = true;
    rows.push_back(headerRow);
    for (uint16_t id : ids) {
      char prefix[12];
      snprintf(prefix, sizeof(prefix), "No.%04d ", id);
      LockOnAchievementListRow itemRow;
      itemRow.label = String(prefix) + dataMgr.getPokemonName(id);
      itemRow.header = false;
      rows.push_back(itemRow);
    }
  }
  return rows;
}

std::vector<LockOnAchievementListRow> getLockOnAchievementPageRows(size_t offset, size_t pageSize) {
  const auto rows = buildLockOnAchievementRows();
  if (offset >= rows.size()) return {};
  const size_t end = std::min(rows.size(), offset + pageSize);
  return std::vector<LockOnAchievementListRow>(rows.begin() + offset, rows.begin() + end);
}

uint8_t getWakeSplashProgressPercent(unsigned long elapsedMs) {
  static constexpr struct {
    uint32_t ms;
    uint8_t progress;
  } kKeyframes[] = {
      {0, 0},
      {220, 15},
      {460, 30},
      {820, 45},
      {1120, 45},
      {1450, 80},
      {1760, 95},
      {2000, 100},
  };

  if (elapsedMs >= kWakeSplashDurationMs) return 100;

  for (size_t i = 1; i < (sizeof(kKeyframes) / sizeof(kKeyframes[0])); ++i) {
    if (elapsedMs <= kKeyframes[i].ms) {
      const uint32_t startMs = kKeyframes[i - 1].ms;
      const uint8_t startProgress = kKeyframes[i - 1].progress;
      const uint32_t endMs = kKeyframes[i].ms;
      const uint8_t endProgress = kKeyframes[i].progress;
      if (endMs <= startMs || endProgress == startProgress) {
        return startProgress;
      }
      const uint32_t span = endMs - startMs;
      const uint32_t local = elapsedMs - startMs;
      return static_cast<uint8_t>(startProgress + ((endProgress - startProgress) * local) / span);
    }
  }
  return 100;
}

const char* getWakeSplashStatusText(uint8_t progressPercent) {
  if (progressPercent >= 100) return "SUCCESS!";
  if (progressPercent >= 95) return "SYSTEM READY";
  if (progressPercent >= 80) return "ESTABLISHING NETWORK...";
  if (progressPercent >= 60) return "LOADING BIOMETRIC SCANNER";
  if (progressPercent >= 45) return "SYNCING POKE-DATABASE";
  if (progressPercent >= 30) return "PROXIMITY SENSOR ACTIVE";
  if (progressPercent >= 15) return "CORE S3 INITIALIZING";
  return "SYSTEM BOOTING...";
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

bool isEnglishSearchInputMode() {
  return appLanguage == APP_LANGUAGE_EN;
}

String getSearchInputGlyph(int keyIndex) {
  if (isEnglishSearchInputMode()) {
    static constexpr const char* kEnglishCharGroups[10][7] = {
        {"A", "B", "C", "", "", "", ""},
        {"D", "E", "F", "", "", "", ""},
        {"G", "H", "I", "", "", "", ""},
        {"J", "K", "L", "", "", "", ""},
        {"M", "N", "O", "", "", "", ""},
        {"P", "Q", "R", "", "", "", ""},
        {"S", "T", "U", "", "", "", ""},
        {"V", "W", "X", "", "", "", ""},
        {"Y", "Z", "", "", "", "", ""},
        {"-", ".", "'", ":", " ", "\xE2\x99\x80", "\xE2\x99\x82"},
    };
    if (searchInputVowelMode
        && searchInputRowIndex >= 0
        && searchInputRowIndex < 10
        && keyIndex >= 0
        && keyIndex < 7) {
      return kEnglishCharGroups[searchInputRowIndex][keyIndex];
    }
    return "";
  }

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
  if (isEnglishSearchInputMode()) {
    static constexpr const char* kEnglishRowLabels[10] = {"ABC", "DEF", "GHI", "JKL", "MNO", "PQR", "STU", "VWX", "YZ", "SYM"};
    if (rowIndex < 0 || rowIndex >= 10) {
      return "";
    }
    return kEnglishRowLabels[rowIndex];
  }

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

bool isHallOfFameSearchCommand(const String& query) {
  return query == "デンドウイリ";
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
  PRESS_MENU_SETTINGS,
  PRESS_MENU_ACHIEVEMENTS,
  PRESS_MENU_3D,
  PRESS_MENU_PREVIEW_CAPTION,
  PRESS_MENU_SETTINGS_TAB_0,
  PRESS_MENU_SETTINGS_TAB_1,
  PRESS_MENU_THEME_0,
  PRESS_MENU_THEME_1,
  PRESS_MENU_LANG_JA,
  PRESS_MENU_LANG_EN,
  PRESS_MENU_VOL_LARGE,
  PRESS_MENU_VOL_MEDIUM,
  PRESS_MENU_VOL_SMALL,
  PRESS_MENU_VOL_MUTE,
  PRESS_LOCKON_ACTION,
  PRESS_SEARCH_INPUT_BACK,
  PRESS_SEARCH_INPUT_DELETE,
  PRESS_SEARCH_INPUT_CLEAR,
  PRESS_SEARCH_INPUT_KEY_0,
  PRESS_GUIDE_BACK,
  PRESS_GUIDE_POKEMON,
  PRESS_GUIDE_LOCATION,
  PRESS_ACHIEVEMENT_LOCKON,
  PRESS_ACHIEVEMENT_LIST,
  PRESS_ACHIEVEMENT_BADGES,
  PRESS_GUIDE_HALL_OF_FAME,
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
  PRESS_GUIDE_DETAIL_BACK,
  PRESS_GUIDE_DETAIL_TAB_0,
  PRESS_GUIDE_DETAIL_TAB_1,
  PRESS_GUIDE_DETAIL_TAB_2,
  PRESS_GUIDE_DETAIL_TAB_3,
  PRESS_GUIDE_DETAIL_TAB_4,
  PRESS_GUIDE_DETAIL_CAUGHT,
  PRESS_GUIDE_DETAIL_PAGE,
  PRESS_GUIDE_DETAIL_PREV,
  PRESS_GUIDE_DETAIL_NEXT,
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
    if (hitTest(tx, ty, 166, 106, 130, 34, 8)) return PRESS_MENU_SETTINGS;
    if (hitTest(tx, ty, 166, 148, 130, 34, 8)) return PRESS_MENU_ACHIEVEMENTS;
    return PRESS_NONE;
  }

  if (mode == SCREEN_LOCKON) {
    return PRESS_LOCKON_ACTION;
  }

  if (mode == SCREEN_SETTINGS) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_BACK;
    if (hitTest(tx, ty, 0, TAB_BAR_Y, SCREEN_WIDTH / 2, TAB_BAR_H, 0)) return PRESS_MENU_SETTINGS_TAB_0;
    if (hitTest(tx, ty, SCREEN_WIDTH / 2, TAB_BAR_Y, SCREEN_WIDTH / 2, TAB_BAR_H, 0)) return PRESS_MENU_SETTINGS_TAB_1;
    if (settingsTabIndex == 0) {
      if (hitTest(tx, ty, 202, 60, 94, 30, 8)) return PRESS_MENU_3D;
      if (hitTest(tx, ty, 202, 98, 94, 30, 8)) return PRESS_MENU_PREVIEW_CAPTION;
      if (hitTest(tx, ty, 24, 152, 128, 30, 6)) return PRESS_MENU_THEME_0;
      if (hitTest(tx, ty, 164, 152, 128, 30, 6)) return PRESS_MENU_THEME_1;
    } else {
      if (hitTest(tx, ty, 24, 84, 128, 30, 6)) return PRESS_MENU_LANG_JA;
      if (hitTest(tx, ty, 164, 84, 128, 30, 6)) return PRESS_MENU_LANG_EN;
      if (hitTest(tx, ty, 24, 152, 58, 28, 6)) return PRESS_MENU_VOL_LARGE;
      if (hitTest(tx, ty, 94, 152, 58, 28, 6)) return PRESS_MENU_VOL_MEDIUM;
      if (hitTest(tx, ty, 164, 152, 58, 28, 6)) return PRESS_MENU_VOL_SMALL;
      if (hitTest(tx, ty, 234, 152, 58, 28, 6)) return PRESS_MENU_VOL_MUTE;
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_LOCATION_DETAIL) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_LIST_BACK;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_NEXT;
    for (int i = 0; i < 10; ++i) {
      const int col = i % 2;
      const int row = i / 2;
      const int x = (col == 0) ? 44 : 162;
      const int y = 48 + (row * 30);
      if (hitTest(tx, ty, x, y, 114, 24, 8)) {
        return static_cast<PressedControl>(PRESS_GUIDE_LIST_ITEM_0 + i);
      }
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_MENU) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_BACK;
    if (hitTest(tx, ty, 32, 72, SCREEN_WIDTH - 64, 42, 8)) return PRESS_GUIDE_POKEMON;
    if (hitTest(tx, ty, 32, 126, SCREEN_WIDTH - 64, 42, 8)) return PRESS_GUIDE_LOCATION;
    return PRESS_NONE;
  }

  if (mode == SCREEN_ACHIEVEMENTS_MENU) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_BACK;
    if (hitTest(tx, ty, 32, 160, SCREEN_WIDTH - 64, 42, 8)) return PRESS_ACHIEVEMENT_LOCKON;
    return PRESS_NONE;
  }

  if (mode == SCREEN_ACHIEVEMENTS_LOCKON) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_BACK;
    if (hitTest(tx, ty, 32, 114, SCREEN_WIDTH - 64, 38, 8)) return PRESS_ACHIEVEMENT_LIST;
    if (hitTest(tx, ty, 32, 162, SCREEN_WIDTH - 64, 38, 8)) return PRESS_ACHIEVEMENT_BADGES;
    return PRESS_NONE;
  }

  if (mode == SCREEN_ACHIEVEMENTS_LOCKON_LIST) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_LIST_BACK;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_NEXT;
    return PRESS_NONE;
  }

  if (mode == SCREEN_ACHIEVEMENTS_LOCKON_BADGES) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_LIST_BACK;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_NEXT;
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_POKEMON_LIST) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_LIST_BACK;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_NEXT;
    for (int i = 0; i < 10; ++i) {
      const int col = i % 2;
      const int row = i / 2;
      const int x = (col == 0) ? 44 : 162;
      const int y = 48 + (row * 30);
      if (hitTest(tx, ty, x, y, 114, 24, 8)) {
        return static_cast<PressedControl>(PRESS_GUIDE_LIST_ITEM_0 + i);
      }
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_LOCATION_LIST) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_LIST_BACK;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_GUIDE_LIST_NEXT;
    for (int i = 0; i < 10; ++i) {
      const int col = i % 2;
      const int row = i / 2;
      const int x = (col == 0) ? 44 : 162;
      const int y = 48 + (row * 30);
      if (hitTest(tx, ty, x, y, 114, 24, 8)) {
        return static_cast<PressedControl>(PRESS_GUIDE_LIST_ITEM_0 + i);
      }
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_GUIDE_POKEMON_DETAIL) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_GUIDE_DETAIL_BACK;
    if (hitTest(tx, ty, 0, 0, 40, TAB_BAR_Y, 0)) return PRESS_GUIDE_DETAIL_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, TAB_BAR_Y, 0)) return PRESS_GUIDE_DETAIL_NEXT;
    if (guidePokemonTab == 0 && hitTest(tx, ty, 16, 152, 84, 22, 6)) return PRESS_GUIDE_DETAIL_CAUGHT;
    if (guidePokemonTab == 0) {
      if (hitTest(tx, ty, 112, 56, SCREEN_WIDTH - 124, 122, 6)) return PRESS_GUIDE_DETAIL_PAGE;
    } else {
      if (hitTest(tx, ty, 12, 56, SCREEN_WIDTH - 24, 122, 6)) return PRESS_GUIDE_DETAIL_PAGE;
    }
    for (int i = 0; i < 5; ++i) {
      const int x = i * (SCREEN_WIDTH / 5);
      if (hitTest(tx, ty, x, TAB_BAR_Y, SCREEN_WIDTH / 5, TAB_BAR_H, 8)) {
        return static_cast<PressedControl>(PRESS_GUIDE_DETAIL_TAB_0 + i);
      }
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_QUIZ) {
    return PRESS_MENU_QUIZ;
  }

  if (mode == SCREEN_SLIDESHOW) {
    return PRESS_MENU_SLIDESHOW;
  }

  if (mode == SCREEN_SEARCH) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_SEARCH_MENU;
    if (hitTest(tx, ty, 44, 52, 114, 24, 6)) return PRESS_SEARCH_NAME_QUERY;
    if (hitTest(tx, ty, 162, 52, 114, 24, 6)) return PRESS_SEARCH_MODE;
    if (hitTest(tx, ty, 0, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_SEARCH_NAME_PAGE_PREV;
    if (hitTest(tx, ty, SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, 0)) return PRESS_SEARCH_NAME_PAGE_NEXT;
    const int resultX[2] = {44, 162};
    const int resultY = 82;
    const int resultW = 114;
    const int resultH = 24;
    for (int i = 0; i < 10; ++i) {
      const int col = i % 2;
      const int row = i / 2;
      if (hitTest(tx, ty, resultX[col], resultY + (row * 28), resultW, resultH, 6)) {
        return static_cast<PressedControl>(PRESS_SEARCH_NAME_RESULT_0 + i);
      }
    }
    return PRESS_NONE;
  }

  if (mode == SCREEN_SEARCH_NUMBER) {
    if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_SEARCH_MENU;
    const int digitX[4] = {60, 110, 160, 210};
    for (int i = 0; i < 4; ++i) {
      if (hitTest(tx, ty, digitX[i], 52, 40, 34, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_UP_0 + i);
      if (hitTest(tx, ty, digitX[i], 120, 40, 34, 8)) return static_cast<PressedControl>(PRESS_SEARCH_DIGIT_DOWN_0 + i);
    }
    if (hitTest(tx, ty, 20, 178, 280, 40, 10)) return PRESS_SEARCH_OPEN;
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

  if (hitTest(tx, ty, MARGIN, 6, SCREEN_WIDTH - (MARGIN * 2), HEADER_H - 12, 6)) return PRESS_SEARCH_HEADER;
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
  ACTION_OPEN_SETTINGS,
  ACTION_CLOSE_SETTINGS,
  ACTION_OPEN_ACHIEVEMENTS_MENU,
  ACTION_CLOSE_ACHIEVEMENTS_MENU,
  ACTION_OPEN_ACHIEVEMENTS_LOCKON,
  ACTION_CLOSE_ACHIEVEMENTS_LOCKON,
  ACTION_OPEN_ACHIEVEMENTS_LOCKON_LIST,
  ACTION_CLOSE_ACHIEVEMENTS_LOCKON_LIST,
  ACTION_ACHIEVEMENTS_LOCKON_LIST_PAGE,
  ACTION_OPEN_ACHIEVEMENTS_LOCKON_BADGES,
  ACTION_CLOSE_ACHIEVEMENTS_LOCKON_BADGES,
  ACTION_ACHIEVEMENTS_LOCKON_BADGES_PAGE,
  ACTION_OPEN_GUIDE_POKEMON_LIST,
  ACTION_CLOSE_GUIDE_POKEMON_LIST,
  ACTION_GUIDE_POKEMON_LIST_PAGE,
  ACTION_OPEN_GUIDE_LOCATION_LIST,
  ACTION_CLOSE_GUIDE_LOCATION_LIST,
  ACTION_GUIDE_LOCATION_LIST_PAGE,
  ACTION_OPEN_GUIDE_LOCATION_DETAIL,
  ACTION_CLOSE_GUIDE_LOCATION_DETAIL,
  ACTION_GUIDE_LOCATION_DETAIL_PAGE,
  ACTION_OPEN_GUIDE_POKEMON_DETAIL,
  ACTION_CLOSE_GUIDE_POKEMON_DETAIL,
  ACTION_SET_GUIDE_POKEMON_TAB,
  ACTION_TOGGLE_GUIDE_CAUGHT,
  ACTION_NEXT_GUIDE_PAGE,
  ACTION_GUIDE_DETAIL_PREV,
  ACTION_GUIDE_DETAIL_NEXT,
  ACTION_OPEN_SEARCH,
  ACTION_OPEN_POKEDEX,
  ACTION_OPEN_LOCKON,
  ACTION_CLOSE_LOCKON,
  ACTION_OPEN_QUIZ,
  ACTION_OPEN_SLIDESHOW,
  ACTION_CLOSE_SLIDESHOW,
  ACTION_CLOSE_PREVIEW_POC,
  ACTION_CLOSE_QUIZ,
  ACTION_TOGGLE_PREVIEW_3D,
  ACTION_TOGGLE_PREVIEW_CAPTION,
  ACTION_SET_SETTINGS_TAB,
  ACTION_SET_UI_THEME,
  ACTION_SET_APP_LANGUAGE,
  ACTION_SEARCH_TO_MENU,
  ACTION_OPEN_SEARCH_NUMBER,
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
  ACTION_DETAIL_BACK,
  ACTION_PREVIEW_OPEN,
  ACTION_PREVIEW_CLOSE,
  ACTION_NAV_PREV,
  ACTION_NAV_NEXT,
  ACTION_OPEN_EVOLUTION,
  ACTION_SET_TAB,
  ACTION_LOCKON_CONFIRM,
  ACTION_LOCKON_OPEN_DETAIL,
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

PressedControl getBackPressedControlForScreen(ScreenMode mode) {
  switch (mode) {
    case SCREEN_LOCKON:
      return PRESS_LOCKON_ACTION;
    case SCREEN_SETTINGS:
      return PRESS_GUIDE_BACK;
    case SCREEN_ACHIEVEMENTS_MENU:
    case SCREEN_ACHIEVEMENTS_LOCKON:
      return PRESS_GUIDE_BACK;
    case SCREEN_ACHIEVEMENTS_LOCKON_LIST:
    case SCREEN_ACHIEVEMENTS_LOCKON_BADGES:
      return PRESS_GUIDE_LIST_BACK;
    case SCREEN_GUIDE_MENU:
      return PRESS_GUIDE_BACK;
    case SCREEN_GUIDE_POKEMON_LIST:
    case SCREEN_GUIDE_LOCATION_LIST:
    case SCREEN_GUIDE_LOCATION_DETAIL:
      return PRESS_GUIDE_LIST_BACK;
    case SCREEN_GUIDE_POKEMON_DETAIL:
      return PRESS_GUIDE_DETAIL_BACK;
    case SCREEN_DETAIL:
      return PRESS_SEARCH_HEADER;
    case SCREEN_SEARCH:
    case SCREEN_SEARCH_NUMBER:
      return PRESS_SEARCH_MENU;
    case SCREEN_SEARCH_INPUT:
      return PRESS_SEARCH_INPUT_BACK;
    case SCREEN_PREVIEW:
      return PRESS_APPEARANCE_PREVIEW;
    case SCREEN_PREVIEW_POC:
      return PRESS_MENU_3D;
    case SCREEN_SLIDESHOW:
      return PRESS_MENU_SLIDESHOW;
    case SCREEN_QUIZ:
      return PRESS_MENU_QUIZ;
    case SCREEN_MENU:
    default:
      return PRESS_NONE;
  }
}

PendingAction getBackPendingActionForScreen(ScreenMode mode) {
  switch (mode) {
    case SCREEN_LOCKON:
      return {};
    case SCREEN_SETTINGS:
      return makePendingAction(ACTION_CLOSE_SETTINGS);
    case SCREEN_ACHIEVEMENTS_MENU:
      return makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_MENU);
    case SCREEN_ACHIEVEMENTS_LOCKON:
      return makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON);
    case SCREEN_ACHIEVEMENTS_LOCKON_LIST:
      return makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON_LIST);
    case SCREEN_ACHIEVEMENTS_LOCKON_BADGES:
      return makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON_BADGES);
    case SCREEN_GUIDE_MENU:
      return makePendingAction(ACTION_CLOSE_GUIDE_MENU);
    case SCREEN_GUIDE_POKEMON_LIST:
      return makePendingAction(ACTION_CLOSE_GUIDE_POKEMON_LIST);
    case SCREEN_GUIDE_LOCATION_LIST:
      return makePendingAction(ACTION_CLOSE_GUIDE_LOCATION_LIST);
    case SCREEN_GUIDE_LOCATION_DETAIL:
      return makePendingAction(ACTION_CLOSE_GUIDE_LOCATION_DETAIL);
    case SCREEN_GUIDE_POKEMON_DETAIL:
      return makePendingAction(ACTION_CLOSE_GUIDE_POKEMON_DETAIL);
    case SCREEN_DETAIL:
      return makePendingAction(ACTION_DETAIL_BACK);
    case SCREEN_SEARCH:
    case SCREEN_SEARCH_NUMBER:
      return makePendingAction(ACTION_SEARCH_TO_MENU);
    case SCREEN_SEARCH_INPUT:
      return makePendingAction(ACTION_SEARCH_CLOSE_INPUT);
    case SCREEN_PREVIEW:
      return makePendingAction(ACTION_PREVIEW_CLOSE);
    case SCREEN_PREVIEW_POC:
      return makePendingAction(ACTION_CLOSE_PREVIEW_POC);
    case SCREEN_SLIDESHOW:
      return makePendingAction(ACTION_CLOSE_SLIDESHOW);
    case SCREEN_QUIZ:
      return makePendingAction(ACTION_CLOSE_QUIZ);
    case SCREEN_MENU:
    default:
      return {};
  }
}

bool pollPortBBackButton(unsigned long now, ScreenMode mode, LockOnPhase currentLockOnPhase, PressedControl& pressedControl, PendingAction& clickedAction) {
  pressedControl = PRESS_NONE;
  clickedAction = {};

  const bool rawPressed = readPortBBackButtonRawPressed();
  if (rawPressed != portBBackButtonLastRawPressed) {
    portBBackButtonLastRawPressed = rawPressed;
    portBBackButtonLastChangeAt = now;
    if (rawPressed) {
      if (mode == SCREEN_LOCKON && currentLockOnPhase == LOCKON_SEARCH) {
        const unsigned long elapsedMs = now - lockOnPhaseStartedAt;
        portBBackButtonCapturedMarkerX =
            getLockOnMarkerCenterX(elapsedMs, lockOnSearchDurationMs, lockOnRarity);
        portBBackButtonCapturedMarkerValid = true;
      } else {
        portBBackButtonCapturedMarkerValid = false;
      }
    } else {
      portBBackButtonCapturedMarkerValid = false;
    }
  }

  if ((now - portBBackButtonLastChangeAt) >= kPortBBackButtonDebounceMs
      && portBBackButtonStablePressed != rawPressed) {
    portBBackButtonStablePressed = rawPressed;
    if (portBBackButtonStablePressed) {
      if (mode == SCREEN_MENU) {
        clickedAction = makePendingAction(ACTION_OPEN_LOCKON);
      } else if (mode == SCREEN_LOCKON) {
        if (currentLockOnPhase == LOCKON_SEARCH) {
          clickedAction = makePendingAction(
              ACTION_LOCKON_CONFIRM,
              portBBackButtonCapturedMarkerValid
                  ? portBBackButtonCapturedMarkerX
                  : getLockOnMarkerCenterX(now - lockOnPhaseStartedAt, lockOnSearchDurationMs, lockOnRarity));
        } else if (currentLockOnPhase == LOCKON_RESULT_SUCCESS) {
          clickedAction = makePendingAction(ACTION_LOCKON_OPEN_DETAIL);
        } else if (currentLockOnPhase == LOCKON_RESULT_FAIL) {
          clickedAction = makePendingAction(ACTION_CLOSE_LOCKON);
        }
      } else {
        clickedAction = getBackPendingActionForScreen(mode);
      }
    }
  }

  if (portBBackButtonStablePressed) {
    pressedControl = (mode == SCREEN_MENU || mode == SCREEN_LOCKON)
        ? PRESS_LOCKON_ACTION
        : getBackPressedControlForScreen(mode);
  }

  return clickedAction.type != ACTION_NONE;
}

uint16_t getAvailableMaxPokemonId() {
  const uint16_t maxId = dataMgr.getMaxPokemonId();
  return (maxId >= MIN_POKEMON_ID) ? maxId : MIN_POKEMON_ID;
}

uint16_t getGuideDexMaxPokemonId() {
  const uint16_t availableMax = getAvailableMaxPokemonId();
  const uint16_t guideMax = guideHallOfFameEnabled ? 386 : 151;
  return (availableMax < guideMax) ? availableMax : guideMax;
}

std::vector<uint16_t> getGuidePokemonPageIds(size_t offset, size_t limit) {
  std::vector<uint16_t> ids;
  const uint16_t maxId = getGuideDexMaxPokemonId();
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

size_t getGuidePokemonCount() {
  size_t count = 0;
  const uint16_t maxId = getGuideDexMaxPokemonId();
  for (uint16_t id = MIN_POKEMON_ID; id <= maxId; ++id) {
    if (dataMgr.getPokemonName(id).length() > 0) {
      ++count;
    }
  }
  return count;
}

bool loadGuideLocations() {
  guideLocations.clear();

  File file = SD.open("/pokemon/firered/locations.json");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error || !doc.is<JsonArray>()) {
    return false;
  }

  for (JsonObject item : doc.as<JsonArray>()) {
    GuideLocationEntry entry;
    entry.id = item["id"].as<String>();
    entry.name = item["name"].as<String>();
    entry.postgameOnly = item["postgame_only"] | false;
    if (item["pokemon"].is<JsonArray>()) {
      for (JsonVariant v : item["pokemon"].as<JsonArray>()) {
        const uint16_t id = v | 0;
        if (id >= 1 && id <= 386) {
          entry.pokemonIds.push_back(id);
        }
      }
    }
    if (entry.name.length() > 0) {
      guideLocations.push_back(entry);
    }
  }

  return !guideLocations.empty();
}

std::vector<String> getGuideLocationPageLabels(size_t offset, size_t limit) {
  std::vector<String> labels;
  auto isGuideLocationUnlocked = [](const GuideLocationEntry& entry) {
    if (guideHallOfFameEnabled) {
      return true;
    }

    const String& id = entry.id;
    const String& name = entry.name;
    if (id == "island04" || id == "island05" || id == "island06" || id == "island07"
        || id == "cerulean_cave" || id == "champion_road_hidden") {
      return false;
    }
    if (name.indexOf("4のしま") >= 0 || name.indexOf("5のしま") >= 0
        || name.indexOf("6のしま") >= 0 || name.indexOf("7のしま") >= 0
        || name.indexOf("ハナダのどうくつ") >= 0
        || name.indexOf("チャンピオンロード") >= 0) {
      return false;
    }
    return true;
  };

  for (size_t i = 0; i < guideLocations.size(); ++i) {
    if (!isGuideLocationUnlocked(guideLocations[i])) {
      continue;
    }
    if (offset > 0) {
      --offset;
      continue;
    }
    labels.push_back(guideLocations[i].name);
    if (labels.size() >= limit) {
      break;
    }
  }
  return labels;
}

size_t getGuideLocationCount() {
  size_t count = 0;
  for (const auto& entry : guideLocations) {
    if (guideHallOfFameEnabled) {
      ++count;
      continue;
    }
    const String& id = entry.id;
    const String& name = entry.name;
    if (id == "island04" || id == "island05" || id == "island06" || id == "island07"
        || id == "cerulean_cave" || id == "champion_road_hidden") {
      continue;
    }
    if (name.indexOf("4のしま") >= 0 || name.indexOf("5のしま") >= 0
        || name.indexOf("6のしま") >= 0 || name.indexOf("7のしま") >= 0
        || name.indexOf("ハナダのどうくつ") >= 0
        || name.indexOf("チャンピオンロード") >= 0) {
      continue;
    }
    ++count;
  }
  return count;
}

std::vector<size_t> getGuideLocationPageIndices(size_t offset, size_t limit) {
  std::vector<size_t> indices;
  auto isGuideLocationUnlocked = [](const GuideLocationEntry& entry) {
    if (guideHallOfFameEnabled) {
      return true;
    }
    const String& id = entry.id;
    const String& name = entry.name;
    if (id == "island04" || id == "island05" || id == "island06" || id == "island07"
        || id == "cerulean_cave" || id == "champion_road_hidden") {
      return false;
    }
    if (name.indexOf("4のしま") >= 0 || name.indexOf("5のしま") >= 0
        || name.indexOf("6のしま") >= 0 || name.indexOf("7のしま") >= 0
        || name.indexOf("ハナダのどうくつ") >= 0
        || name.indexOf("チャンピオンロード") >= 0) {
      return false;
    }
    return true;
  };

  for (size_t i = 0; i < guideLocations.size(); ++i) {
    if (!isGuideLocationUnlocked(guideLocations[i])) continue;
    if (offset > 0) {
      --offset;
      continue;
    }
    indices.push_back(i);
    if (indices.size() >= limit) break;
  }
  return indices;
}

std::vector<uint16_t> getSearchFilteredIds() {
  return dataMgr.findPokemonIdsByName(searchNameQuery, 0, getAvailableMaxPokemonId());
}

bool loadGuidePokemonDetail(uint16_t pokemonId) {
  guidePokemonDetail = GuidePokemonDetailEntry{};

  char path[48];
  snprintf(path, sizeof(path), "/pokemon/firered/pokemon/%04d.json", pokemonId);
  File file = SD.open(path);
  if (!file) {
    return false;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error || !doc.is<JsonObject>()) {
    return false;
  }

  JsonObject root = doc.as<JsonObject>();

  if (root["locations"].is<JsonArray>()) {
    for (JsonObject item : root["locations"].as<JsonArray>()) {
      GuidePokemonLocationEntry entry;
      entry.area = item["area"].as<String>();
      entry.method = item["method"].as<String>();
      entry.rate = item["rate"].as<String>();
      guidePokemonDetail.locations.push_back(entry);
    }
  }

  if (root["evolution"].is<JsonArray>()) {
    for (JsonVariant item : root["evolution"].as<JsonArray>()) {
      const String value = item.as<String>();
      if (value.length() > 0) {
        guidePokemonDetail.evolutions.push_back(value);
      }
    }
  }

  if (root["level_up_moves"].is<JsonArray>()) {
    for (JsonObject item : root["level_up_moves"].as<JsonArray>()) {
      GuidePokemonMoveEntry entry;
      entry.level = item["level"] | 0;
      entry.name = item["name"].as<String>();
      if (entry.name.length() > 0) {
        guidePokemonDetail.levelUpMoves.push_back(entry);
      }
    }
  }

  if (root["machines"].is<JsonArray>()) {
    for (JsonVariant item : root["machines"].as<JsonArray>()) {
      const String value = item.as<String>();
      if (value.length() > 0) {
        guidePokemonDetail.machines.push_back(value);
      }
    }
  }

  if (root["hms"].is<JsonArray>()) {
    for (JsonVariant item : root["hms"].as<JsonArray>()) {
      const String value = item.as<String>();
      if (value.length() > 0) {
        guidePokemonDetail.hms.push_back(value);
      }
    }
  }

  guidePokemonDetail.loaded = true;
  return true;
}

std::vector<String> getGuidePokemonTabLines(int tabIndex) {
  std::vector<String> lines;
  switch (tabIndex) {
    case 0:
      lines = guidePokemonDetail.evolutions;
      std::sort(lines.begin(), lines.end());
      break;
    case 1: {
      std::vector<String> locationLines;
      for (const auto& entry : guidePokemonDetail.locations) {
        String line = entry.area;
        if (entry.method.length() > 0) line += " / " + entry.method;
        if (entry.rate.length() > 0) line += " / " + entry.rate;
        locationLines.push_back(line);
      }
      std::sort(locationLines.begin(), locationLines.end());
      lines = std::move(locationLines);
      break;
    }
    case 2: {
      auto moves = guidePokemonDetail.levelUpMoves;
      std::sort(moves.begin(), moves.end(), [](const GuidePokemonMoveEntry& a, const GuidePokemonMoveEntry& b) {
        if (a.level != b.level) return a.level < b.level;
        return a.name < b.name;
      });
      for (const auto& entry : moves) {
        char prefix[12];
        snprintf(prefix, sizeof(prefix), "Lv%d ", entry.level);
        lines.push_back(String(prefix) + entry.name);
      }
      break;
    }
    case 3:
      lines = guidePokemonDetail.machines;
      std::sort(lines.begin(), lines.end());
      break;
    case 4:
      lines = guidePokemonDetail.hms;
      std::sort(lines.begin(), lines.end());
      break;
  }
  if (lines.empty()) {
    lines.push_back("データなし");
  }
  return lines;
}

int getGuidePokemonPageCount(int tabIndex) {
  constexpr int kGuideLinesPerPage = 8;
  const auto lines = getGuidePokemonTabLines(tabIndex);
  const int lineCount = static_cast<int>(lines.size());
  return std::max(1, (lineCount + kGuideLinesPerPage - 1) / kGuideLinesPerPage);
}

uint16_t getGuidePokemonMaxId() {
  return guideHallOfFameEnabled ? 386 : 151;
}

void loadGuideLocationPokemonList(const GuideLocationEntry& location) {
  guideLocationPokemonIds.clear();
  const uint16_t maxId = getGuidePokemonMaxId();
  for (uint16_t id : location.pokemonIds) {
    if (id >= 1 && id <= maxId) {
      guideLocationPokemonIds.push_back(id);
    }
  }
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

const char* getQuizVolumeStatusLabel(QuizVolumeSetting setting) {
  switch (setting) {
    case QUIZ_VOLUME_LARGE: return "L";
    case QUIZ_VOLUME_MEDIUM: return "M";
    case QUIZ_VOLUME_SMALL: return "S";
    case QUIZ_VOLUME_MUTE: return "OFF";
  }
  return "M";
}

const char* getUIThemeKey(UIThemeStyle theme) {
  switch (theme) {
    case UI_THEME_CYBER: return "cyber";
    case UI_THEME_CLASSIC:
    default:
      return "classic";
  }
}

UIThemeStyle parseUIThemeKey(const char* value) {
  if (value == nullptr) return UI_THEME_CLASSIC;
  if (strcmp(value, "cyber") == 0) return UI_THEME_CYBER;
  return UI_THEME_CLASSIC;
}

const char* getAppLanguageKey(AppLanguage language) {
  switch (language) {
    case APP_LANGUAGE_EN: return "en";
    case APP_LANGUAGE_JA:
    default:
      return "ja";
  }
}

AppLanguage parseAppLanguageKey(const char* value) {
  if (value == nullptr) return APP_LANGUAGE_JA;
  if (strcmp(value, "en") == 0) return APP_LANGUAGE_EN;
  return APP_LANGUAGE_JA;
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

const char* getQuizSoundLanguageDir(AppLanguage language) {
  return (language == APP_LANGUAGE_EN) ? kQuizSoundDirEn : kQuizSoundDirJa;
}

String getQuizSoundFilename(QuizPhase phase) {
  return (phase == QUIZ_A_SIDE) ? "Eyecatch_Aside.wav" : "Eyecatch_Bside.wav";
}

String getQuizLegacySoundPath(QuizPhase phase) {
  return String("/pokemon/quiz/sounds/") + getQuizSoundFilename(phase);
}

String getQuizSoundPath(QuizPhase phase, AppLanguage language) {
  return String(getQuizSoundLanguageDir(language)) + "/" + getQuizSoundFilename(phase);
}

void releaseQuizSoundCache(QuizSoundCache& cache) {
  if (cache.data != nullptr) {
    free(cache.data);
  }
  cache.data = nullptr;
  cache.size = 0;
  cache.pcm16 = nullptr;
  cache.sampleCount = 0;
  cache.sampleRate = 44100;
  cache.stereo = false;
  cache.ready = false;
  cache.path = "";
}

void invalidateQuizSoundCaches() {
  releaseQuizSoundCache(quizAsideSound);
  releaseQuizSoundCache(quizBsideSound);
}

bool saveSettings() {
  JsonDocument doc;
  doc["app_language"] = getAppLanguageKey(appLanguage);
  doc["quiz_volume"] = getQuizVolumeKey(quizVolumeSetting);
  doc["preview_3d"] = preview3dEnabled;
  doc["preview_caption"] = previewCaptionEnabled;
  doc["guide_hall_of_fame"] = guideHallOfFameEnabled;
  doc["ui_theme"] = getUIThemeKey(uiThemeStyle);

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

bool loadGuideCaughtFlags() {
  guideCaughtFlags.assign(387, false);
  File file = SD.open(kGuideCaughtPath, FILE_READ);
  if (!file) {
    return true;
  }
  JsonDocument doc;
  if (deserializeJson(doc, file) != DeserializationError::Ok) {
    file.close();
    return false;
  }
  file.close();

  JsonArray caught = doc["caught"].as<JsonArray>();
  if (caught.isNull()) {
    return true;
  }
  for (JsonVariant v : caught) {
    const int id = v.as<int>();
    if (id >= 1 && id <= 386) {
      guideCaughtFlags[id] = true;
    }
  }
  return true;
}

bool saveGuideCaughtFlags() {
  JsonDocument doc;
  JsonArray arr = doc["caught"].to<JsonArray>();
  for (int id = 1; id <= 386; ++id) {
    if (guideCaughtFlags[id]) {
      arr.add(id);
    }
  }

  if (SD.exists(kGuideCaughtPath)) {
    SD.remove(kGuideCaughtPath);
  }
  File file = SD.open(kGuideCaughtPath, FILE_WRITE);
  if (!file) {
    return false;
  }
  const bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

bool loadLockOnSecretIds() {
  lockOnSecretIds.clear();

  File file = SD.open(kLockOnSecretIdsPath, FILE_READ);
  if (!file) {
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, file) != DeserializationError::Ok) {
    file.close();
    return false;
  }
  file.close();

  JsonArray ids = doc["ids"].as<JsonArray>();
  if (ids.isNull() && doc.is<JsonArray>()) {
    ids = doc.as<JsonArray>();
  }
  if (ids.isNull()) {
    return false;
  }

  for (JsonVariant v : ids) {
    const int id = v.as<int>();
    if (id >= MIN_POKEMON_ID && id <= getAvailableMaxPokemonId()) {
      if (!isPokemonIdInVector(static_cast<uint16_t>(id), lockOnSecretIds)) {
        lockOnSecretIds.push_back(static_cast<uint16_t>(id));
      }
    }
  }

  return !lockOnSecretIds.empty();
}

void loadSettings() {
  appLanguage = APP_LANGUAGE_JA;
  quizVolumeSetting = QUIZ_VOLUME_MEDIUM;
  preview3dEnabled = false;
  previewCaptionEnabled = true;
  guideHallOfFameEnabled = false;
  uiThemeStyle = UI_THEME_CLASSIC;

  File file = SD.open(kSettingsPath, FILE_READ);
  if (!file) {
    saveSettings();
    ui.setLanguage(appLanguage);
    invalidateQuizSoundCaches();
    applyQuizVolume();
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, file) == DeserializationError::Ok) {
    appLanguage = parseAppLanguageKey(doc["app_language"] | "ja");
    quizVolumeSetting = parseQuizVolumeKey(doc["quiz_volume"] | "medium");
    preview3dEnabled = doc["preview_3d"] | false;
    previewCaptionEnabled = doc["preview_caption"] | true;
    guideHallOfFameEnabled = doc["guide_hall_of_fame"] | false;
    uiThemeStyle = parseUIThemeKey(doc["ui_theme"] | "classic");
  }
  file.close();
  dataMgr.setLanguage(appLanguage);
  ui.setLanguage(appLanguage);
  invalidateQuizSoundCaches();
  ui.setTheme(uiThemeStyle);
  applyQuizVolume();
}

bool loadQuizSound(const char* path, QuizSoundCache& cache) {
  if (cache.ready
      && cache.data != nullptr
      && cache.size > 0
      && cache.path == path) {
    return true;
  }

  if (cache.data != nullptr) {
    releaseQuizSoundCache(cache);
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
  cache.path = path;
  return true;
}

void playQuizSound(QuizPhase phase) {
  if (phase == QUIZ_A_SIDE) {
    recordQuizQuestionViewed(millis());
  }
  QuizSoundCache& cache = (phase == QUIZ_A_SIDE) ? quizAsideSound : quizBsideSound;
  String path = getQuizSoundPath(phase, appLanguage);
  if (!SD.exists(path.c_str())) {
    path = getQuizLegacySoundPath(phase);
  }
  if (!loadQuizSound(path.c_str(), cache)) {
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

void resetQuizState(QuizPhase& quizPhase, uint32_t& quizPhaseStartedAt, uint16_t& quizPokemonId, uint32_t now) {
  quizPhase = QUIZ_A_SIDE;
  quizPhaseStartedAt = now;
  quizPokemonId = chooseNextQuizPokemonId(0);
}

void renderAppearanceImage(LGFX_Sprite& target, uint16_t pokemonId) {
  target.fillRect(0, 0, kAppearanceImageW, kAppearanceImageH, ui.getBackgroundColor());
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
  target.fillRect(0, 0, kEvolutionImageW, kEvolutionImageH, ui.getBackgroundColor());
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
  M5.Display.setBrightness(kDisplayBrightnessOn);

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    M5.Display.print("SD Init Error");
    while(1) delay(100);
  }

  if (!dataMgr.begin()) {
    M5.Display.print("SD Error");
    while(1) delay(100);
  }
  loadLockOnSecretIds();
  loadGuideLocations();

  if (!ui.begin()) {
    M5.Display.print("UI Error");
    while(1) delay(100);
  }

  loadSettings();
  ui.setTheme(uiThemeStyle);
  loadGuideCaughtFlags();
  loadLockOnAchievements();
  loadAchievementStats();
  coverProximityReady = initCoverProximitySensor();
  initPortBBackButton();
  wakeSplashActive = true;
  wakeSplashStartedAt = millis();

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
  static size_t guideLocationListOffset = 0;

  PressedControl pressedControl = PRESS_NONE;
  PressedControl portBPressedControl = PRESS_NONE;
  PendingAction portBClickedAction;
  const unsigned long now = millis();
  updateLockOnBadgeFeverState(now);

  if (coverProximityReady && (now - coverLastPollAt) >= kCoverPollIntervalMs) {
    coverLastPollAt = now;
    coverLastProximityValue = readCoverProximityValue();

    const bool coverClosed = coverLastProximityValue >= kCoverClosedThreshold;
    const bool coverOpened = coverLastProximityValue <= kCoverOpenThreshold;

    if (coverDisplaySleeping) {
      if (coverOpened) {
        M5.Display.wakeup();
        M5.Display.setBrightness(kDisplayBrightnessOn);
        coverDisplaySleeping = false;
        wakeSplashActive = true;
        wakeSplashStartedAt = now;
        coverClosedSince = 0;
        heldControl = PRESS_NONE;
        latchedControl = PRESS_NONE;
        pendingAction = {};
        if (screenMode == SCREEN_QUIZ) {
          quizPhaseStartedAt = now;
        } else if (screenMode == SCREEN_SLIDESHOW) {
          slideshowPhaseStartedAt = now;
        }
        needsRedraw = true;
      }
    } else {
      if (coverClosed) {
        if (coverClosedSince == 0) {
          coverClosedSince = now;
        } else if ((now - coverClosedSince) >= kCoverSleepDelayMs) {
          M5.Display.sleep();
          coverDisplaySleeping = true;
          wakeSplashActive = false;
          heldControl = PRESS_NONE;
          latchedControl = PRESS_NONE;
          pendingAction = {};
        }
      } else if (coverOpened) {
        coverClosedSince = 0;
      }
    }
  }

  if (wakeSplashActive) {
    if ((now - wakeSplashStartedAt) >= kWakeSplashDurationMs) {
      wakeSplashActive = false;
      needsRedraw = true;
      if (screenMode == SCREEN_QUIZ) {
        quizPhaseStartedAt = now;
      } else if (screenMode == SCREEN_SLIDESHOW) {
        slideshowPhaseStartedAt = now;
      }
    } else {
      needsRedraw = true;
    }
  }

  if (lockOnFeverEndsAt != 0 && !isLockOnFeverActive(now)) {
    lockOnFeverEndsAt = 0;
    if (screenMode == SCREEN_LOCKON) {
      needsRedraw = true;
    }
  }

  if (coverDisplaySleeping) {
    if (guideCaughtDirty && now >= guideCaughtSaveAt) {
      saveGuideCaughtFlags();
      guideCaughtDirty = false;
    }
    if (lockOnAchievementsDirty && now >= lockOnAchievementsSaveAt) {
      saveLockOnAchievements();
      lockOnAchievementsDirty = false;
    }
    if (achievementStatsDirty && now >= achievementStatsSaveAt) {
      saveAchievementStats();
      achievementStatsDirty = false;
    }
    if (settingsDirty && now >= settingsSaveAt) {
      saveSettings();
      settingsDirty = false;
    }
    delay(20);
    return;
  }

  if (!wakeSplashActive) {
    pollPortBBackButton(now, screenMode, lockOnPhase, portBPressedControl, portBClickedAction);
    if (pendingAction.type == ACTION_NONE && portBClickedAction.type != ACTION_NONE) {
      heldControl = portBPressedControl;
      latchedControl = portBPressedControl;
      pendingAction = portBClickedAction;
      needsRedraw = true;
    }
  }

  if (!wakeSplashActive && screenMode == SCREEN_QUIZ && M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    heldControl = PRESS_MENU_QUIZ;
    if (t.wasClicked()) {
      latchedControl = PRESS_MENU_QUIZ;
      pendingAction = makePendingAction(ACTION_CLOSE_QUIZ);
      needsRedraw = true;
    }
  } else if (!wakeSplashActive && M5.Touch.getCount() > 0) {
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
          pendingAction = makePendingAction(ACTION_OPEN_SEARCH_NUMBER);
        } else if (pressedControl == PRESS_SEARCH_MENU) {
          pendingAction = makePendingAction(ACTION_SEARCH_TO_MENU);
        } else if (pressedControl == PRESS_SEARCH_NAME_PAGE_PREV) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_PAGE, -10);
        } else if (pressedControl == PRESS_SEARCH_NAME_PAGE_NEXT) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_PAGE, 10);
        } else if (pressedControl >= PRESS_SEARCH_NAME_RESULT_0 && pressedControl <= PRESS_SEARCH_NAME_RESULT_9) {
          pendingAction = makePendingAction(ACTION_SEARCH_NAME_OPEN, pressedControl - PRESS_SEARCH_NAME_RESULT_0);
        }
      } else if (screenMode == SCREEN_SEARCH_NUMBER) {
        if (pressedControl >= PRESS_SEARCH_DIGIT_UP_0 && pressedControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, digitStep[pressedControl - PRESS_SEARCH_DIGIT_UP_0]);
        } else if (pressedControl >= PRESS_SEARCH_DIGIT_DOWN_0 && pressedControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
          const int digitStep[4] = {1000, 100, 10, 1};
          pendingAction = makePendingAction(ACTION_SEARCH_ADJUST, -digitStep[pressedControl - PRESS_SEARCH_DIGIT_DOWN_0]);
        } else if (pressedControl == PRESS_SEARCH_OPEN) {
          pendingAction = makePendingAction(ACTION_SEARCH_OPEN);
        } else if (pressedControl == PRESS_SEARCH_MENU) {
          pendingAction = makePendingAction(ACTION_SEARCH_TO_MENU);
        }
      } else if (screenMode == SCREEN_LOCKON) {
        if (lockOnPhase == LOCKON_SEARCH) {
          pendingAction = makePendingAction(
              ACTION_LOCKON_CONFIRM,
              getLockOnMarkerCenterX(now - lockOnPhaseStartedAt, lockOnSearchDurationMs, lockOnRarity));
        } else if (lockOnPhase == LOCKON_RESULT_SUCCESS) {
          pendingAction = makePendingAction(ACTION_LOCKON_OPEN_DETAIL);
        } else if (lockOnPhase == LOCKON_RESULT_FAIL) {
          pendingAction = makePendingAction(ACTION_CLOSE_LOCKON);
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
        } else if (pressedControl == PRESS_GUIDE_LOCATION) {
          pendingAction = makePendingAction(ACTION_OPEN_GUIDE_LOCATION_LIST);
        }
      } else if (screenMode == SCREEN_ACHIEVEMENTS_MENU) {
        if (pressedControl == PRESS_GUIDE_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_MENU);
        } else if (pressedControl == PRESS_ACHIEVEMENT_LOCKON) {
          pendingAction = makePendingAction(ACTION_OPEN_ACHIEVEMENTS_LOCKON);
        }
      } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON) {
        if (pressedControl == PRESS_GUIDE_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON);
        } else if (pressedControl == PRESS_ACHIEVEMENT_LIST) {
          pendingAction = makePendingAction(ACTION_OPEN_ACHIEVEMENTS_LOCKON_LIST);
        } else if (pressedControl == PRESS_ACHIEVEMENT_BADGES) {
          pendingAction = makePendingAction(ACTION_OPEN_ACHIEVEMENTS_LOCKON_BADGES);
        }
      } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON_LIST) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON_LIST);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_ACHIEVEMENTS_LOCKON_LIST_PAGE, -8);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_ACHIEVEMENTS_LOCKON_LIST_PAGE, 8);
        }
      } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON_BADGES) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_ACHIEVEMENTS_LOCKON_BADGES);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_ACHIEVEMENTS_LOCKON_BADGES_PAGE, -3);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_ACHIEVEMENTS_LOCKON_BADGES_PAGE, 3);
        }
      } else if (screenMode == SCREEN_GUIDE_POKEMON_LIST) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_POKEMON_LIST);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_GUIDE_POKEMON_LIST_PAGE, -10);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_GUIDE_POKEMON_LIST_PAGE, 10);
        } else if (pressedControl >= PRESS_GUIDE_LIST_ITEM_0 && pressedControl <= PRESS_GUIDE_LIST_ITEM_9) {
          const auto pageIds = getGuidePokemonPageIds(guidePokemonListOffset, 10);
          const int pageIndex = pressedControl - PRESS_GUIDE_LIST_ITEM_0;
          if (pageIndex >= 0 && pageIndex < static_cast<int>(pageIds.size())) {
            pendingAction = makePendingAction(ACTION_OPEN_GUIDE_POKEMON_DETAIL, pageIds[pageIndex]);
          }
        }
      } else if (screenMode == SCREEN_GUIDE_LOCATION_LIST) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_LOCATION_LIST);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_GUIDE_LOCATION_LIST_PAGE, -10);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_GUIDE_LOCATION_LIST_PAGE, 10);
        } else if (pressedControl >= PRESS_GUIDE_LIST_ITEM_0 && pressedControl <= PRESS_GUIDE_LIST_ITEM_9) {
          const auto pageIndices = getGuideLocationPageIndices(guideLocationListOffset, 10);
          const int pageIndex = pressedControl - PRESS_GUIDE_LIST_ITEM_0;
          if (pageIndex >= 0 && pageIndex < static_cast<int>(pageIndices.size())) {
            pendingAction = makePendingAction(ACTION_OPEN_GUIDE_LOCATION_DETAIL, static_cast<int>(pageIndices[pageIndex]));
          }
        }
      } else if (screenMode == SCREEN_GUIDE_LOCATION_DETAIL) {
        if (pressedControl == PRESS_GUIDE_LIST_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_LOCATION_DETAIL);
        } else if (pressedControl == PRESS_GUIDE_LIST_PREV) {
          pendingAction = makePendingAction(ACTION_GUIDE_LOCATION_DETAIL_PAGE, -10);
        } else if (pressedControl == PRESS_GUIDE_LIST_NEXT) {
          pendingAction = makePendingAction(ACTION_GUIDE_LOCATION_DETAIL_PAGE, 10);
        } else if (pressedControl >= PRESS_GUIDE_LIST_ITEM_0 && pressedControl <= PRESS_GUIDE_LIST_ITEM_9) {
          const int pageIndex = pressedControl - PRESS_GUIDE_LIST_ITEM_0;
          const size_t itemIndex = guideLocationPokemonListOffset + static_cast<size_t>(pageIndex);
          if (itemIndex < guideLocationPokemonIds.size()) {
            pendingAction = makePendingAction(ACTION_OPEN_GUIDE_POKEMON_DETAIL, guideLocationPokemonIds[itemIndex]);
          }
        }
      } else if (screenMode == SCREEN_GUIDE_POKEMON_DETAIL) {
        if (pressedControl == PRESS_GUIDE_DETAIL_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_GUIDE_POKEMON_DETAIL);
        } else if (pressedControl == PRESS_GUIDE_DETAIL_PREV) {
          pendingAction = makePendingAction(ACTION_GUIDE_DETAIL_PREV);
        } else if (pressedControl == PRESS_GUIDE_DETAIL_NEXT) {
          pendingAction = makePendingAction(ACTION_GUIDE_DETAIL_NEXT);
        } else if (pressedControl == PRESS_GUIDE_DETAIL_CAUGHT) {
          pendingAction = makePendingAction(ACTION_TOGGLE_GUIDE_CAUGHT);
        } else if (pressedControl == PRESS_GUIDE_DETAIL_PAGE) {
          pendingAction = makePendingAction(ACTION_NEXT_GUIDE_PAGE);
        } else if (pressedControl >= PRESS_GUIDE_DETAIL_TAB_0 && pressedControl <= PRESS_GUIDE_DETAIL_TAB_4) {
          pendingAction = makePendingAction(ACTION_SET_GUIDE_POKEMON_TAB, pressedControl - PRESS_GUIDE_DETAIL_TAB_0);
        }
      } else if (screenMode == SCREEN_SETTINGS) {
        if (pressedControl == PRESS_GUIDE_BACK) {
          pendingAction = makePendingAction(ACTION_CLOSE_SETTINGS);
        } else if (pressedControl == PRESS_MENU_SETTINGS_TAB_0 || pressedControl == PRESS_MENU_SETTINGS_TAB_1) {
          pendingAction = makePendingAction(ACTION_SET_SETTINGS_TAB, pressedControl - PRESS_MENU_SETTINGS_TAB_0);
        } else if (pressedControl == PRESS_MENU_3D) {
          pendingAction = makePendingAction(ACTION_TOGGLE_PREVIEW_3D);
        } else if (pressedControl == PRESS_MENU_PREVIEW_CAPTION) {
          pendingAction = makePendingAction(ACTION_TOGGLE_PREVIEW_CAPTION);
        } else if (pressedControl == PRESS_MENU_THEME_0 || pressedControl == PRESS_MENU_THEME_1) {
          pendingAction = makePendingAction(ACTION_SET_UI_THEME, pressedControl - PRESS_MENU_THEME_0);
        } else if (pressedControl == PRESS_MENU_LANG_JA || pressedControl == PRESS_MENU_LANG_EN) {
          pendingAction = makePendingAction(ACTION_SET_APP_LANGUAGE, pressedControl - PRESS_MENU_LANG_JA);
        } else if (pressedControl >= PRESS_MENU_VOL_LARGE && pressedControl <= PRESS_MENU_VOL_MUTE) {
          pendingAction = makePendingAction(ACTION_SET_QUIZ_VOLUME, pressedControl - PRESS_MENU_VOL_LARGE);
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
        } else if (pressedControl == PRESS_MENU_SETTINGS) {
          pendingAction = makePendingAction(ACTION_OPEN_SETTINGS);
        } else if (pressedControl == PRESS_MENU_ACHIEVEMENTS) {
          pendingAction = makePendingAction(ACTION_OPEN_ACHIEVEMENTS_MENU);
        }
      } else {
        if (pressedControl == PRESS_SEARCH_HEADER) {
          pendingAction = makePendingAction(ACTION_DETAIL_BACK);
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
  } else if (portBPressedControl != PRESS_NONE) {
    if (heldControl != portBPressedControl) {
      heldControl = portBPressedControl;
      if (latchedControl == PRESS_NONE) {
        needsRedraw = true;
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
      case ACTION_OPEN_ACHIEVEMENTS_MENU:
        screenMode = SCREEN_ACHIEVEMENTS_MENU;
        break;
      case ACTION_CLOSE_ACHIEVEMENTS_MENU:
        screenMode = SCREEN_MENU;
        break;
      case ACTION_OPEN_ACHIEVEMENTS_LOCKON:
        screenMode = SCREEN_ACHIEVEMENTS_LOCKON;
        break;
      case ACTION_CLOSE_ACHIEVEMENTS_LOCKON:
        screenMode = SCREEN_ACHIEVEMENTS_MENU;
        break;
      case ACTION_OPEN_ACHIEVEMENTS_LOCKON_LIST:
        screenMode = SCREEN_ACHIEVEMENTS_LOCKON_LIST;
        lockOnAchievementsListOffset = 0;
        break;
      case ACTION_CLOSE_ACHIEVEMENTS_LOCKON_LIST:
        screenMode = SCREEN_ACHIEVEMENTS_LOCKON;
        break;
      case ACTION_ACHIEVEMENTS_LOCKON_LIST_PAGE: {
        const size_t totalCount = buildLockOnAchievementRows().size();
        const size_t pageSize = 8;
        const size_t maxOffset = (totalCount > pageSize) ? (((totalCount - 1) / pageSize) * pageSize) : 0;
        if (totalCount == 0) {
          lockOnAchievementsListOffset = 0;
          break;
        }
        if (pendingAction.value < 0) {
          lockOnAchievementsListOffset =
              (lockOnAchievementsListOffset == 0) ? maxOffset : (lockOnAchievementsListOffset - pageSize);
        } else if (pendingAction.value > 0) {
          lockOnAchievementsListOffset =
              (lockOnAchievementsListOffset >= maxOffset) ? 0 : (lockOnAchievementsListOffset + pageSize);
        }
        break;
      }
      case ACTION_OPEN_ACHIEVEMENTS_LOCKON_BADGES:
        screenMode = SCREEN_ACHIEVEMENTS_LOCKON_BADGES;
        lockOnBadgeListOffset = 0;
        break;
      case ACTION_CLOSE_ACHIEVEMENTS_LOCKON_BADGES:
        screenMode = SCREEN_ACHIEVEMENTS_LOCKON;
        break;
      case ACTION_ACHIEVEMENTS_LOCKON_BADGES_PAGE: {
        const size_t totalCount = buildLockOnBadgeRows().size();
        const size_t pageSize = 3;
        const size_t maxOffset = (totalCount > pageSize) ? (((totalCount - 1) / pageSize) * pageSize) : 0;
        if (totalCount == 0) {
          lockOnBadgeListOffset = 0;
          break;
        }
        if (pendingAction.value < 0) {
          lockOnBadgeListOffset =
              (lockOnBadgeListOffset == 0) ? maxOffset : (lockOnBadgeListOffset - pageSize);
        } else if (pendingAction.value > 0) {
          lockOnBadgeListOffset =
              (lockOnBadgeListOffset >= maxOffset) ? 0 : (lockOnBadgeListOffset + pageSize);
        }
        break;
      }
      case ACTION_OPEN_GUIDE_MENU:
        screenMode = SCREEN_GUIDE_MENU;
        break;
      case ACTION_CLOSE_GUIDE_MENU:
        screenMode = SCREEN_MENU;
        break;
      case ACTION_OPEN_SETTINGS:
        screenMode = SCREEN_SETTINGS;
        settingsTabIndex = 0;
        break;
      case ACTION_CLOSE_SETTINGS:
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
        const size_t totalCount = getGuidePokemonCount();
        const size_t maxOffset = (totalCount > 10) ? (((totalCount - 1) / 10) * 10) : 0;
        if (totalCount == 0) {
          guidePokemonListOffset = 0;
          break;
        }
        if (pendingAction.value < 0) {
          guidePokemonListOffset = (guidePokemonListOffset == 0) ? maxOffset : guidePokemonListOffset - 10;
        } else if (pendingAction.value > 0) {
          guidePokemonListOffset = (guidePokemonListOffset >= maxOffset) ? 0 : (guidePokemonListOffset + 10);
        }
        break;
      }
      case ACTION_OPEN_GUIDE_LOCATION_LIST:
        screenMode = SCREEN_GUIDE_LOCATION_LIST;
        guideLocationListOffset = 0;
        break;
      case ACTION_CLOSE_GUIDE_LOCATION_LIST:
        screenMode = SCREEN_GUIDE_MENU;
        break;
      case ACTION_OPEN_GUIDE_LOCATION_DETAIL:
        if (pendingAction.value >= 0 && pendingAction.value < static_cast<int>(guideLocations.size())) {
          const GuideLocationEntry& selectedLocation = guideLocations[pendingAction.value];
          guideLocationSelectedName = selectedLocation.name;
          loadGuideLocationPokemonList(selectedLocation);
          guideLocationPokemonListOffset = 0;
          screenMode = SCREEN_GUIDE_LOCATION_DETAIL;
        }
        break;
      case ACTION_CLOSE_GUIDE_LOCATION_DETAIL:
        screenMode = SCREEN_GUIDE_LOCATION_LIST;
        break;
      case ACTION_GUIDE_LOCATION_LIST_PAGE: {
        const size_t totalCount = getGuideLocationCount();
        const size_t maxOffset = (totalCount > 10) ? (((totalCount - 1) / 10) * 10) : 0;
        if (totalCount == 0) {
          guideLocationListOffset = 0;
          break;
        }
        if (pendingAction.value < 0) {
          guideLocationListOffset = (guideLocationListOffset == 0) ? maxOffset : guideLocationListOffset - 10;
        } else if (pendingAction.value > 0) {
          guideLocationListOffset = (guideLocationListOffset >= maxOffset) ? 0 : (guideLocationListOffset + 10);
        }
        break;
      }
      case ACTION_GUIDE_LOCATION_DETAIL_PAGE: {
        const size_t totalCount = guideLocationPokemonIds.size();
        const size_t maxOffset = (totalCount > 10) ? (((totalCount - 1) / 10) * 10) : 0;
        if (totalCount == 0) {
          guideLocationPokemonListOffset = 0;
          break;
        }
        if (pendingAction.value < 0) {
          guideLocationPokemonListOffset = (guideLocationPokemonListOffset == 0) ? maxOffset : guideLocationPokemonListOffset - 10;
        } else if (pendingAction.value > 0) {
          guideLocationPokemonListOffset = (guideLocationPokemonListOffset >= maxOffset) ? 0 : (guideLocationPokemonListOffset + 10);
        }
        break;
      }
      case ACTION_OPEN_GUIDE_POKEMON_DETAIL:
        guidePokemonSelectedId = static_cast<uint16_t>(pendingAction.value);
        guidePokemonTab = 0;
        guidePokemonPageIndex = 0;
        guidePokemonDetailReturnScreen = (screenMode == SCREEN_GUIDE_LOCATION_DETAIL) ? SCREEN_GUIDE_LOCATION_DETAIL : SCREEN_GUIDE_POKEMON_LIST;
        loadGuidePokemonDetail(guidePokemonSelectedId);
        screenMode = SCREEN_GUIDE_POKEMON_DETAIL;
        break;
      case ACTION_CLOSE_GUIDE_POKEMON_DETAIL:
        screenMode = guidePokemonDetailReturnScreen;
        break;
      case ACTION_SET_GUIDE_POKEMON_TAB:
        guidePokemonTab = constrain(pendingAction.value, 0, 4);
        guidePokemonPageIndex = 0;
        break;
      case ACTION_TOGGLE_GUIDE_CAUGHT:
        if (guidePokemonSelectedId >= 1 && guidePokemonSelectedId <= 386) {
          guideCaughtFlags[guidePokemonSelectedId] = !guideCaughtFlags[guidePokemonSelectedId];
          guideCaughtDirty = true;
          guideCaughtSaveAt = millis() + 250;
        }
        break;
      case ACTION_NEXT_GUIDE_PAGE: {
        const int pageCount = getGuidePokemonPageCount(guidePokemonTab);
        guidePokemonPageIndex = (guidePokemonPageIndex + 1) % pageCount;
        break;
      }
      case ACTION_GUIDE_DETAIL_PREV:
        guidePokemonSelectedId = (guidePokemonSelectedId <= 1) ? getGuidePokemonMaxId() : (guidePokemonSelectedId - 1);
        guidePokemonTab = 0;
        guidePokemonPageIndex = 0;
        loadGuidePokemonDetail(guidePokemonSelectedId);
        break;
      case ACTION_GUIDE_DETAIL_NEXT:
        guidePokemonSelectedId = (guidePokemonSelectedId >= getGuidePokemonMaxId()) ? 1 : (guidePokemonSelectedId + 1);
        guidePokemonTab = 0;
        guidePokemonPageIndex = 0;
        loadGuidePokemonDetail(guidePokemonSelectedId);
        break;
      case ACTION_OPEN_POKEDEX:
        screenMode = SCREEN_SEARCH;
        searchMode = SEARCH_MODE_NAME;
        searchNameOffset = 0;
        searchNameQuery = "";
        searchInputVowelMode = false;
        searchInputRowIndex = -1;
        break;
      case ACTION_OPEN_LOCKON:
        screenMode = SCREEN_LOCKON;
        lockOnPhase = LOCKON_READY;
        lockOnPhaseStartedAt = now;
        lockOnLastPulseAt = now;
        lockOnRarity = chooseLockOnRarity(isLockOnFeverActive(now));
        lockOnPokemonId = chooseLockOnPokemonId(lockOnRarity, lockOnLastPokemonId);
        lockOnSearchDurationMs = getLockOnSearchDurationMs(lockOnRarity);
        lockOnZoneWidth = getLockOnZoneWidth(lockOnRarity);
        lockOnZoneCenterX = chooseLockOnZoneCenterX(lockOnZoneWidth);
        M5.Speaker.stop();
        M5.Speaker.tone(1240, 90);
        triggerVibration(72, 60, now);
        break;
      case ACTION_CLOSE_LOCKON:
        screenMode = SCREEN_MENU;
        lockOnPhase = LOCKON_IDLE;
        M5.Speaker.stop();
        triggerVibration(0, 0, now);
        break;
      case ACTION_OPEN_QUIZ:
        screenMode = SCREEN_QUIZ;
        resetQuizState(quizPhase, quizPhaseStartedAt, quizPokemonId, now);
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
      case ACTION_TOGGLE_PREVIEW_CAPTION:
        previewCaptionEnabled = !previewCaptionEnabled;
        saveSettings();
        break;
      case ACTION_SET_SETTINGS_TAB:
        settingsTabIndex = constrain(pendingAction.value, 0, 1);
        break;
      case ACTION_SET_UI_THEME:
        uiThemeStyle = static_cast<UIThemeStyle>(constrain(pendingAction.value, 0, static_cast<int>(UI_THEME_COUNT) - 1));
        ui.setTheme(uiThemeStyle);
        saveSettings();
        break;
      case ACTION_SET_APP_LANGUAGE: {
        const AppLanguage nextLanguage = static_cast<AppLanguage>(constrain(pendingAction.value, 0, static_cast<int>(APP_LANGUAGE_COUNT) - 1));
        if (appLanguage != nextLanguage) {
          appLanguage = nextLanguage;
          dataMgr.setLanguage(appLanguage);
          ui.setLanguage(appLanguage);
          invalidateQuizSoundCaches();
          saveSettings();
        }
        break;
      }
      case ACTION_CLOSE_QUIZ:
        M5.Speaker.stop();
        resetQuizState(quizPhase, quizPhaseStartedAt, quizPokemonId, now);
        screenMode = SCREEN_MENU;
        break;
      case ACTION_SET_QUIZ_VOLUME:
        quizVolumeSetting = static_cast<QuizVolumeSetting>(pendingAction.value);
        applyQuizVolume();
        saveSettings();
        break;
      case ACTION_OPEN_SEARCH:
        screenMode = SCREEN_SEARCH_NUMBER;
        returnId = currentId;
        searchId = currentId;
        searchMode = SEARCH_MODE_NUMBER;
        searchNameOffset = 0;
        break;
      case ACTION_OPEN_SEARCH_NUMBER:
        screenMode = SCREEN_SEARCH_NUMBER;
        searchMode = SEARCH_MODE_NUMBER;
        searchId = currentId;
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
          if (isHallOfFameSearchCommand(searchNameQuery)) {
            guideHallOfFameEnabled = !guideHallOfFameEnabled;
            searchNameQuery = "";
            searchNameOffset = 0;
            settingsDirty = true;
            settingsSaveAt = millis() + 250;
            searchFlashUntil = millis() + 180;
          }
          screenMode = SCREEN_SEARCH;
        }
        break;
      case ACTION_SEARCH_INPUT_ROW:
        if (isEnglishSearchInputMode()) {
          if (pendingAction.value >= 0 && pendingAction.value < 10) {
            searchInputVowelMode = true;
            searchInputRowIndex = pendingAction.value;
          }
        } else if (pendingAction.value == 10) {
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
        if (isEnglishSearchInputMode()) {
          const String input = getSearchInputGlyph(pendingAction.value);
          if (input.length() == 0) {
            break;
          }
          searchNameQuery += input;
          searchNameOffset = 0;
          searchInputVowelMode = false;
          searchInputRowIndex = -1;
        } else if (pendingAction.value == 10) {
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
        screenMode = (screenMode == SCREEN_SEARCH_NUMBER) ? SCREEN_SEARCH : SCREEN_MENU;
        break;
      case ACTION_SEARCH_NAME_PAGE: {
        const size_t totalCount = getSearchFilteredIds().size();
        if (totalCount == 0) {
          searchNameOffset = 0;
          break;
        }
        const int pageSize = 10;
        const int lastPageOffset = static_cast<int>(((totalCount - 1) / pageSize) * pageSize);
        const int nextOffset = static_cast<int>(searchNameOffset) + pendingAction.value;
        if (nextOffset < 0) {
          searchNameOffset = static_cast<size_t>(lastPageOffset);
        } else if (nextOffset >= static_cast<int>(totalCount)) {
          searchNameOffset = 0;
        } else {
          searchNameOffset = static_cast<size_t>(nextOffset);
        }
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
          detailReturnScreen = SCREEN_SEARCH;
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
          detailReturnScreen = SCREEN_SEARCH_NUMBER;
          screenMode = SCREEN_DETAIL;
          dataMgr.loadPokemonDetail(currentId);
        }
        break;
      case ACTION_DETAIL_BACK:
        screenMode = detailReturnScreen;
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
      case ACTION_LOCKON_CONFIRM: {
        if (lockOnPhase != LOCKON_SEARCH) {
          break;
        }
        const int markerX = pendingAction.value;
        lockOnConfirmedMarkerX = markerX;
        const int zoneLeft = lockOnZoneCenterX - (static_cast<int>(lockOnZoneWidth) / 2);
        const int zoneRight = zoneLeft + static_cast<int>(lockOnZoneWidth) - 1;
        lockOnPendingSuccess = markerX >= zoneLeft && markerX <= zoneRight;
        lockOnPhase = LOCKON_LOCKED;
        lockOnPhaseStartedAt = now;
        M5.Speaker.stop();
        M5.Speaker.tone(lockOnPendingSuccess ? 1760 : 320, lockOnPendingSuccess ? 100 : 120);
        triggerVibration(lockOnPendingSuccess ? 180 : 96, lockOnPendingSuccess ? 220 : 120, now);
        break;
      }
      case ACTION_LOCKON_OPEN_DETAIL:
        if (lockOnPhase == LOCKON_RESULT_SUCCESS) {
          currentId = lockOnPokemonId;
          returnId = currentId;
          detailReturnScreen = SCREEN_MENU;
          dataMgr.loadPokemonDetail(currentId);
          screenMode = SCREEN_DETAIL;
          lockOnPhase = LOCKON_IDLE;
        }
        break;
      case ACTION_NAV_PREV: {
        if (detailReturnScreen == SCREEN_SEARCH && searchMode == SEARCH_MODE_NAME) {
          const auto ids = getSearchFilteredIds();
          if (!ids.empty()) {
            auto it = std::find(ids.begin(), ids.end(), currentId);
            if (it == ids.end() || it == ids.begin()) {
              currentId = ids.back();
            } else {
              currentId = *(it - 1);
            }
            dataMgr.loadPokemonDetail(currentId);
            break;
          }
        }
        const uint16_t maxPokemonId = getAvailableMaxPokemonId();
        currentId = (currentId <= MIN_POKEMON_ID) ? maxPokemonId : (currentId - 1);
        dataMgr.loadPokemonDetail(currentId);
        break;
      }
      case ACTION_NAV_NEXT: {
        if (detailReturnScreen == SCREEN_SEARCH && searchMode == SEARCH_MODE_NAME) {
          const auto ids = getSearchFilteredIds();
          if (!ids.empty()) {
            auto it = std::find(ids.begin(), ids.end(), currentId);
            if (it == ids.end() || (it + 1) == ids.end()) {
              currentId = ids.front();
            } else {
              currentId = *(it + 1);
            }
            dataMgr.loadPokemonDetail(currentId);
            break;
          }
        }
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

  if (!wakeSplashActive && screenMode == SCREEN_QUIZ && pendingAction.type == ACTION_NONE) {
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

  if (!wakeSplashActive && screenMode == SCREEN_SLIDESHOW && pendingAction.type == ACTION_NONE) {
    recordSlideshowAchievementView(slideshowPokemonId, now);
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

  if (vibrationStopAt != 0 && now >= vibrationStopAt) {
    M5.Power.setVibration(0);
    vibrationStopAt = 0;
  }

  if (!wakeSplashActive && screenMode == SCREEN_LOCKON && pendingAction.type == ACTION_NONE) {
    const unsigned long elapsedMs = now - lockOnPhaseStartedAt;
    if (lockOnPhase == LOCKON_READY) {
      if (elapsedMs >= kLockOnReadyDurationMs) {
        lockOnPhase = LOCKON_SEARCH;
        lockOnPhaseStartedAt = now;
        lockOnLastPulseAt = now;
      }
      needsRedraw = true;
    } else if (lockOnPhase == LOCKON_SEARCH) {
      uint32_t pulseIntervalMs = 210;
      float rarityFrequencyBoost = 720.0f;
      uint8_t vibrationLevel = 32;
      switch (lockOnRarity) {
        case LOCKON_RARITY_SECRET:
          pulseIntervalMs = 70;
          rarityFrequencyBoost = 1560.0f;
          vibrationLevel = 68;
          break;
        case LOCKON_RARITY_MYTHIC:
          pulseIntervalMs = 90;
          rarityFrequencyBoost = 1380.0f;
          vibrationLevel = 58;
          break;
        case LOCKON_RARITY_LEGEND:
          pulseIntervalMs = 110;
          rarityFrequencyBoost = 1200.0f;
          vibrationLevel = 48;
          break;
        case LOCKON_RARITY_EPIC:
          pulseIntervalMs = 130;
          rarityFrequencyBoost = 980.0f;
          vibrationLevel = 42;
          break;
        case LOCKON_RARITY_RARE:
          pulseIntervalMs = 150;
          rarityFrequencyBoost = 840.0f;
          vibrationLevel = 36;
          break;
        case LOCKON_RARITY_NORMAL:
        default:
          break;
      }
      if ((now - lockOnLastPulseAt) >= pulseIntervalMs) {
        lockOnLastPulseAt = now;
        const float progress = constrain(static_cast<float>(elapsedMs) / static_cast<float>(lockOnSearchDurationMs), 0.0f, 1.0f);
        const float frequency = 680.0f + (progress * rarityFrequencyBoost);
        M5.Speaker.tone(frequency, 45, 0, false);
        triggerVibration(vibrationLevel, 30, now);
      }
      if (elapsedMs >= lockOnSearchDurationMs) {
        lockOnPhase = LOCKON_RESULT_FAIL;
        lockOnPhaseStartedAt = now;
        recordLockOnBadgeFailure(false, now);
        playLockOnResultSound(false, lockOnRarity);
        triggerVibration(80, 100, now);
      }
      needsRedraw = true;
    } else if (lockOnPhase == LOCKON_LOCKED) {
      if (elapsedMs >= kLockOnConfirmFreezeMs) {
        lockOnPhase = lockOnPendingSuccess ? LOCKON_RESULT_SUCCESS : LOCKON_RESULT_FAIL;
        lockOnPhaseStartedAt = now;
        if (lockOnPendingSuccess) {
          recordLockOnAchievement(lockOnPokemonId, lockOnRarity, now);
          recordLockOnCapture(now);
          recordLockOnBadgeSuccess(lockOnRarity, isLockOnFeverActive(now), now);
          lockOnLastPokemonId = lockOnPokemonId;
        } else {
          const int zoneLeft = lockOnZoneCenterX - (lockOnZoneWidth / 2);
          const int zoneRight = zoneLeft + lockOnZoneWidth - 1;
          const bool outsideZoneMiss =
              lockOnConfirmedMarkerX < zoneLeft || lockOnConfirmedMarkerX > zoneRight;
          recordLockOnBadgeFailure(outsideZoneMiss, now);
        }
        playLockOnResultSound(lockOnPendingSuccess, lockOnRarity);
      }
      needsRedraw = true;
    } else if (lockOnPhase == LOCKON_RESULT_SUCCESS) {
      if (elapsedMs >= kLockOnSuccessDisplayMs) {
        screenMode = SCREEN_MENU;
        lockOnPhase = LOCKON_IDLE;
        needsRedraw = true;
      }
    } else if (lockOnPhase == LOCKON_RESULT_FAIL) {
      if (elapsedMs >= kLockOnFailDisplayMs) {
        screenMode = SCREEN_MENU;
        lockOnPhase = LOCKON_IDLE;
        needsRedraw = true;
      }
    }
  }

  static float previewPocShiftXF = 0.0f;
  static float previewPocShiftYF = 0.0f;
  static int previewPocShiftX = 0;
  static int previewPocShiftY = 0;
  if (!wakeSplashActive && (screenMode == SCREEN_PREVIEW_POC || (screenMode == SCREEN_SLIDESHOW && preview3dEnabled))) {
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
      if (previewCaptionEnabled) {
        ui.redrawPreviewCaptionToDisplay(previewResult.pokemonId, dataMgr.getPokemonName(previewResult.pokemonId));
      }
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

    if (screenMode == SCREEN_DETAIL) {
      recordPokedexAchievementView(currentId, now);
    }

    if (wakeSplashActive) {
      const uint8_t progressPercent = getWakeSplashProgressPercent(now - wakeSplashStartedAt);
      ui.drawWakeSplashScreen(progressPercent, getWakeSplashStatusText(progressPercent));
    } else if (screenMode == SCREEN_LOCKON) {
      const unsigned long phaseElapsedMs = now - lockOnPhaseStartedAt;
      const bool feverActive = isLockOnFeverActive(now);
      const uint16_t feverSecondsRemaining = static_cast<uint16_t>((getLockOnFeverRemainingMs(now) + 999UL) / 1000UL);
      const uint8_t capturesUntilFever = (lockOnCapturedCount >= lockOnNextFeverCaptureCount)
          ? 0
          : static_cast<uint8_t>(min<uint32_t>(lockOnNextFeverCaptureCount - lockOnCapturedCount, 255));
      const int jitterOffset = (lockOnPhase == LOCKON_SEARCH)
          ? static_cast<int>((phaseElapsedMs / 70) % 3) - 1
          : 0;
      const int markerCenterX = (lockOnPhase == LOCKON_SEARCH)
          ? getLockOnMarkerCenterX(phaseElapsedMs, lockOnSearchDurationMs, lockOnRarity)
          : lockOnConfirmedMarkerX;
      const uint8_t progressPercent = (lockOnPhase == LOCKON_SEARCH && lockOnSearchDurationMs > 0)
          ? static_cast<uint8_t>(constrain((phaseElapsedMs * 100UL) / lockOnSearchDurationMs, 0UL, 100UL))
          : 0;
      ui.drawLockOnScreen(
          static_cast<int>(lockOnPhase),
          static_cast<int>(lockOnRarity),
          getLockOnRarityLabel(lockOnRarity),
          feverActive,
          feverSecondsRemaining,
          capturesUntilFever,
          markerCenterX,
          lockOnZoneCenterX,
          lockOnZoneWidth,
          jitterOffset,
          progressPercent,
          lockOnPokemonId,
          dataMgr.getPokemonName(lockOnPokemonId),
          visualControl == PRESS_LOCKON_ACTION,
          lockOnPhase == LOCKON_RESULT_SUCCESS);
    } else if (screenMode == SCREEN_MENU) {
      const int batteryLevel = M5.Power.getBatteryLevel();
      const bool batteryCharging =
          M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
      ui.drawMenuScreen(
          visualControl == PRESS_MENU_POKEDEX,
          visualControl == PRESS_MENU_QUIZ,
          visualControl == PRESS_MENU_SLIDESHOW,
          visualControl == PRESS_MENU_GUIDE,
          visualControl == PRESS_MENU_ACHIEVEMENTS,
          visualControl == PRESS_MENU_SETTINGS,
          getQuizVolumeStatusLabel(quizVolumeSetting),
          batteryLevel,
          batteryCharging);
    } else if (screenMode == SCREEN_SETTINGS) {
      ui.drawSettingsScreen(
          visualControl == PRESS_GUIDE_BACK,
          settingsTabIndex,
          (visualControl >= PRESS_MENU_SETTINGS_TAB_0 && visualControl <= PRESS_MENU_SETTINGS_TAB_1)
              ? (visualControl - PRESS_MENU_SETTINGS_TAB_0)
              : -1,
          preview3dEnabled,
          visualControl == PRESS_MENU_3D,
          previewCaptionEnabled,
          visualControl == PRESS_MENU_PREVIEW_CAPTION,
          static_cast<int>(uiThemeStyle),
          (visualControl >= PRESS_MENU_THEME_0 && visualControl <= PRESS_MENU_THEME_1)
              ? (visualControl - PRESS_MENU_THEME_0)
              : -1,
          static_cast<int>(appLanguage),
          (visualControl >= PRESS_MENU_LANG_JA && visualControl <= PRESS_MENU_LANG_EN)
              ? (visualControl - PRESS_MENU_LANG_JA)
              : -1,
          static_cast<int>(quizVolumeSetting),
          (visualControl >= PRESS_MENU_VOL_LARGE && visualControl <= PRESS_MENU_VOL_MUTE)
              ? (visualControl - PRESS_MENU_VOL_LARGE)
              : -1);
    } else if (screenMode == SCREEN_GUIDE_MENU) {
      ui.drawGuideMenuScreen(
          visualControl == PRESS_GUIDE_POKEMON,
          visualControl == PRESS_GUIDE_LOCATION,
          visualControl == PRESS_GUIDE_BACK);
    } else if (screenMode == SCREEN_ACHIEVEMENTS_MENU) {
      ui.drawAchievementsMenuScreen(
          countTrueFlags(pokedexAchievementViewed),
          quizQuestionsViewedCount,
          getGuideCaughtCount(),
          countTrueFlags(slideshowAchievementViewed),
          visualControl == PRESS_ACHIEVEMENT_LOCKON,
          visualControl == PRESS_GUIDE_BACK);
    } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON) {
      ui.drawLockOnAchievementsSummaryScreen(
          getLockOnAchievementCaughtCount(),
          visualControl == PRESS_ACHIEVEMENT_LIST,
          visualControl == PRESS_ACHIEVEMENT_BADGES,
          visualControl == PRESS_GUIDE_BACK);
    } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON_LIST) {
      const auto rows = getLockOnAchievementPageRows(lockOnAchievementsListOffset, 8);
      std::vector<String> labels;
      std::vector<bool> headerFlags;
      labels.reserve(rows.size());
      headerFlags.reserve(rows.size());
      for (const auto& row : rows) {
        labels.push_back(row.label);
        headerFlags.push_back(row.header);
      }
      ui.drawLockOnAchievementsListScreen(
          labels,
          headerFlags,
          visualControl == PRESS_GUIDE_LIST_BACK,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_ACHIEVEMENTS_LOCKON_BADGES) {
      const auto rows = getLockOnBadgePageRows(lockOnBadgeListOffset, 3);
      std::vector<String> titles;
      std::vector<String> descriptions;
      std::vector<bool> unlockedFlags;
      titles.reserve(rows.size());
      descriptions.reserve(rows.size());
      unlockedFlags.reserve(rows.size());
      for (const auto& row : rows) {
        titles.push_back(row.title);
        descriptions.push_back(row.description);
        unlockedFlags.push_back(row.unlocked);
      }
      ui.drawLockOnAchievementBadgesScreen(
          titles,
          descriptions,
          unlockedFlags,
          getUnlockedLockOnBadgeCount(),
          LOCKON_BADGE_COUNT,
          visualControl == PRESS_GUIDE_LIST_BACK,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_GUIDE_POKEMON_LIST) {
      const auto pageIds = getGuidePokemonPageIds(guidePokemonListOffset, 10);
      std::vector<String> pageLabels;
      std::vector<bool> pageCaughtFlags;
      pageLabels.reserve(pageIds.size());
      pageCaughtFlags.reserve(pageIds.size());
      for (uint16_t id : pageIds) {
        char prefix[8];
        snprintf(prefix, sizeof(prefix), "%04d", id);
        pageLabels.push_back(String(prefix) + " " + dataMgr.getPokemonName(id));
        pageCaughtFlags.push_back(id >= 1 && id <= 386 ? guideCaughtFlags[id] : false);
      }
      ui.drawGuidePokemonListScreen(
          guideHallOfFameEnabled ? "全国図鑑" : "カントー図鑑",
          pageLabels,
          pageCaughtFlags,
          visualControl == PRESS_GUIDE_LIST_BACK,
          (visualControl >= PRESS_GUIDE_LIST_ITEM_0 && visualControl <= PRESS_GUIDE_LIST_ITEM_9)
              ? (visualControl - PRESS_GUIDE_LIST_ITEM_0)
              : -1,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_GUIDE_LOCATION_LIST) {
      ui.drawGuideLocationListScreen(
          getGuideLocationPageLabels(guideLocationListOffset, 10),
          visualControl == PRESS_GUIDE_LIST_BACK,
          (visualControl >= PRESS_GUIDE_LIST_ITEM_0 && visualControl <= PRESS_GUIDE_LIST_ITEM_9)
              ? (visualControl - PRESS_GUIDE_LIST_ITEM_0)
              : -1,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_GUIDE_LOCATION_DETAIL) {
      std::vector<String> pageLabels;
      std::vector<bool> pageCaughtFlags;
      const size_t endIndex = std::min(guideLocationPokemonIds.size(), guideLocationPokemonListOffset + 10);
      pageLabels.reserve(endIndex - guideLocationPokemonListOffset);
      pageCaughtFlags.reserve(endIndex - guideLocationPokemonListOffset);
      for (size_t i = guideLocationPokemonListOffset; i < endIndex; ++i) {
        const uint16_t id = guideLocationPokemonIds[i];
        char prefix[8];
        snprintf(prefix, sizeof(prefix), "%04d", id);
        pageLabels.push_back(String(prefix) + " " + dataMgr.getPokemonName(id));
        pageCaughtFlags.push_back(id >= 1 && id <= 386 ? guideCaughtFlags[id] : false);
      }
      ui.drawGuidePokemonListScreen(
          guideLocationSelectedName.c_str(),
          pageLabels,
          pageCaughtFlags,
          visualControl == PRESS_GUIDE_LIST_BACK,
          (visualControl >= PRESS_GUIDE_LIST_ITEM_0 && visualControl <= PRESS_GUIDE_LIST_ITEM_9)
              ? (visualControl - PRESS_GUIDE_LIST_ITEM_0)
              : -1,
          visualControl == PRESS_GUIDE_LIST_PREV,
          visualControl == PRESS_GUIDE_LIST_NEXT);
    } else if (screenMode == SCREEN_GUIDE_POKEMON_DETAIL) {
      ui.drawGuidePokemonDetailScreen(
          guidePokemonSelectedId,
          dataMgr.getPokemonName(guidePokemonSelectedId),
          getGuidePokemonTabLines(guidePokemonTab),
          guidePokemonTab,
          guidePokemonPageIndex,
          getGuidePokemonPageCount(guidePokemonTab),
          (guidePokemonSelectedId >= 1 && guidePokemonSelectedId <= 386) ? guideCaughtFlags[guidePokemonSelectedId] : false,
          visualControl == PRESS_GUIDE_DETAIL_CAUGHT,
          visualControl == PRESS_GUIDE_DETAIL_BACK,
          (visualControl >= PRESS_GUIDE_DETAIL_TAB_0 && visualControl <= PRESS_GUIDE_DETAIL_TAB_4)
              ? (visualControl - PRESS_GUIDE_DETAIL_TAB_0)
              : -1,
          visualControl == PRESS_GUIDE_DETAIL_PAGE,
          visualControl == PRESS_GUIDE_DETAIL_PREV,
          visualControl == PRESS_GUIDE_DETAIL_NEXT);
    } else if (screenMode == SCREEN_QUIZ) {
      ui.drawQuizScreen(quizPhase == QUIZ_B_SIDE, quizPokemonId, dataMgr.getPokemonName(quizPokemonId));
    } else if (screenMode == SCREEN_SEARCH) {
      std::vector<uint16_t> nameCandidateIds
          = dataMgr.findPokemonIdsByName(searchNameQuery, searchNameOffset, 10);
      std::vector<String> nameCandidateLabels;
      for (uint16_t id : nameCandidateIds) {
        char prefix[8];
        snprintf(prefix, sizeof(prefix), "%04d", id);
        nameCandidateLabels.push_back(String(prefix) + " " + dataMgr.getPokemonName(id));
      }
      ui.drawSearchScreen(
          true,
          searchId,
          dataMgr.getPokemonName(searchId),
          searchNameQuery,
          nameCandidateIds,
          nameCandidateLabels,
          0,
          visualControl == PRESS_SEARCH_MENU,
          visualControl == PRESS_SEARCH_MODE,
          (visualControl >= PRESS_SEARCH_NAME_RESULT_0 && visualControl <= PRESS_SEARCH_NAME_RESULT_9)
              ? (visualControl - PRESS_SEARCH_NAME_RESULT_0)
              : -1,
          visualControl == PRESS_SEARCH_NAME_PAGE_PREV,
          visualControl == PRESS_SEARCH_NAME_PAGE_NEXT,
          millis() < searchFlashUntil,
          visualControl == PRESS_SEARCH_NAME_QUERY,
          false);
    } else if (screenMode == SCREEN_SEARCH_NUMBER) {
      int pressedDigitDelta = 0;
      if (visualControl >= PRESS_SEARCH_DIGIT_UP_0 && visualControl <= (PRESS_SEARCH_DIGIT_UP_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = digitStep[visualControl - PRESS_SEARCH_DIGIT_UP_0];
      } else if (visualControl >= PRESS_SEARCH_DIGIT_DOWN_0 && visualControl <= (PRESS_SEARCH_DIGIT_DOWN_0 + 3)) {
        const int digitStep[4] = {1000, 100, 10, 1};
        pressedDigitDelta = -digitStep[visualControl - PRESS_SEARCH_DIGIT_DOWN_0];
      }
      ui.drawSearchScreen(
          false,
          searchId,
          dataMgr.getPokemonName(searchId),
          "",
          {},
          {},
          pressedDigitDelta,
          visualControl == PRESS_SEARCH_MENU,
          false,
          -1,
          false,
          false,
          false,
          false,
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
      if (previewCaptionEnabled) {
        ui.drawPreviewCaption(currentId, dataMgr.getPokemonName(currentId));
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
      if (previewCaptionEnabled) {
        ui.drawPreviewCaption(currentId, dataMgr.getPokemonName(currentId));
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
      if (previewCaptionEnabled) {
        ui.drawPreviewCaption(slideshowPokemonId, dataMgr.getPokemonName(slideshowPokemonId));
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

  if (guideCaughtDirty && millis() >= guideCaughtSaveAt) {
    saveGuideCaughtFlags();
    guideCaughtDirty = false;
  }

  if (lockOnAchievementsDirty && millis() >= lockOnAchievementsSaveAt) {
    saveLockOnAchievements();
    lockOnAchievementsDirty = false;
  }

  if (achievementStatsDirty && millis() >= achievementStatsSaveAt) {
    saveAchievementStats();
    achievementStatsDirty = false;
  }

  if (settingsDirty && millis() >= settingsSaveAt) {
    saveSettings();
    settingsDirty = false;
  }

  delay(10);
}
