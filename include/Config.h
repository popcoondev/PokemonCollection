#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Screen Dimensions
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Colors (16-bit RGB565)
#define COLOR_PK_RED      0xE000 // #E60012 (公式レッド)
#define COLOR_PK_BG       0xF7BE // #F2F2F5 (背景グレー)
#define COLOR_PK_CARD     0xFFFF // #FFFFFF (カード白)
#define COLOR_PK_TEXT     0x3186 // #333333 (メインテキスト)
#define COLOR_PK_SUB      0x8410 // #848484 (サブテキスト)
#define COLOR_PK_BAR      0xFD20 // #FFD700 (能力値バー)
#define COLOR_PK_BORDER   0xCE79 // #CCCCCC (境界線)

// Layout Constants
#define MARGIN            10
#define HEADER_H          48
#define TAB_BAR_H         36

// Pokemon Data Limits
#define MIN_POKEMON_ID    1
#define MAX_POKEMON_ID    1025

enum TabType {
  TAB_APPEARANCE = 0,
  TAB_DESCRIPTION,
  TAB_BODY,
  TAB_ABILITY,
  TAB_EVOLUTION,
  TAB_COUNT
};

#endif
