#pragma once

// DOOM 分辨率 (240x240 TFT)
#define DOOMGENERIC_RESX 240
#define DOOMGENERIC_RESY 240

// 按键映射 (ESP32-S3 GPIO, active LOW)
#define PIN_BTN_UP     4
#define PIN_BTN_DOWN   16
#define PIN_BTN_LEFT   17
#define PIN_BTN_RIGHT  47
#define PIN_BTN_FIRE   5    // OK键 = 开火
#define PIN_BTN_ESC    17   // 长按LEFT = ESC菜单

// WAD文件路径
#define DOOM_WAD_PATH "/doom1.wad"

// 分块传输行数
#define CHUNK_LINES 40
