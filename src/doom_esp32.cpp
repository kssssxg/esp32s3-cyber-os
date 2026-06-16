// doom_esp32.cpp — DOOM平台适配层 (Arduino + TFT_eSPI)
// 适配自 Komedenden/esp32-s3-doom-port

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

extern "C" {
#include "doomgeneric/doom_config.h"
#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomkeys.h"
}

// 外部TFT对象 (s3.ino中定义)
extern TFT_eSPI tft;

// ===================== 按键状态 =====================
struct ButtonMap {
    int pin;
    unsigned char doom_key;
    bool last_state;
    unsigned long press_time;
};

static ButtonMap doom_btns[] = {
    {PIN_BTN_UP,    KEY_UPARROW,    false, 0},
    {PIN_BTN_DOWN,  KEY_DOWNARROW,  false, 0},
    {PIN_BTN_LEFT,  KEY_LEFTARROW,  false, 0},
    {PIN_BTN_RIGHT, KEY_RIGHTARROW, false, 0},
    {PIN_BTN_FIRE,  KEY_RCTRL,      false, 0},  // OK键=开火
};
#define NUM_DOOM_BTNS (sizeof(doom_btns) / sizeof(doom_btns[0]))

// 长按OK=ENTER(确认/开门), 长按LEFT=ESCAPE(菜单)
static unsigned long longPressStart = 0;
static bool longPressHandled = false;
#define LONG_PRESS_MS 500

// ===================== DMA分块传输 =====================
static uint16_t *line_buffers[2] = {NULL, NULL};
static int current_buf_idx = 0;

// ===================== 平台函数实现 =====================

extern "C" void DG_Init() {
    // TFT_eSPI已在s3.ino的setup()中初始化，这里不需要重复
    // 按键GPIO已在s3.ino中初始化(pullup)

    // 分配DMA行缓冲 (内部SRAM)
    for (int i = 0; i < 2; i++) {
        if (!line_buffers[i]) {
            line_buffers[i] = (uint16_t*)heap_caps_malloc(
                DOOMGENERIC_RESX * CHUNK_LINES * sizeof(uint16_t),
                MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
            );
        }
    }

    // DG_ScreenBuffer在doomgeneric内部已用PSRAM分配
    Serial.printf("[DOOM] Init OK. ScreenBuffer=%p\n", DG_ScreenBuffer);
}

extern "C" void DG_DrawFrame() {
    if (!DG_ScreenBuffer) return;

    uint32_t *src = (uint32_t *)DG_ScreenBuffer;

    for (int y = 0; y < DOOMGENERIC_RESY; y += CHUNK_LINES) {
        int lines = (y + CHUNK_LINES > DOOMGENERIC_RESY) ? (DOOMGENERIC_RESY - y) : CHUNK_LINES;
        uint16_t *dest = line_buffers[current_buf_idx];

        // RGBA8888 → RGB565 转换
        for (int j = 0; j < lines; j++) {
            uint32_t *src_line = &src[(y + j) * DOOMGENERIC_RESX];
            for (int x = 0; x < DOOMGENERIC_RESX; x++) {
                uint32_t p = *src_line++;
                uint16_t color = ((p >> 8) & 0xF800) | ((p >> 5) & 0x07E0) | ((p >> 3) & 0x001F);
                *dest++ = (color >> 8) | (color << 8); // 字节序翻转
            }
        }

        // 推送到TFT (分块)
        tft.pushImage(0, y, DOOMGENERIC_RESX, lines, line_buffers[current_buf_idx]);
        current_buf_idx = 1 - current_buf_idx;
    }
}

extern "C" int DG_GetKey(int *pressed, unsigned char *key) {
    // 检测按键状态变化
    for (int i = 0; i < (int)NUM_DOOM_BTNS; i++) {
        bool current = (digitalRead(doom_btns[i].pin) == LOW);

        if (current != doom_btns[i].last_state) {
            doom_btns[i].last_state = current;
            *pressed = current ? 1 : 0;
            *key = doom_btns[i].doom_key;

            // 记录按下时间用于长按检测
            if (current) {
                doom_btns[i].press_time = millis();
                longPressHandled = false;
            }
            return 1;
        }

        // 长按检测：OK键长按→ENTER, LEFT键长按→ESCAPE
        if (current && !longPressHandled && (millis() - doom_btns[i].press_time > LONG_PRESS_MS)) {
            longPressHandled = true;
            if (doom_btns[i].doom_key == KEY_RCTRL) {
                // 长按OK → ENTER (确认/开门)
                *pressed = 1;
                *key = KEY_ENTER;
                return 1;
            } else if (doom_btns[i].doom_key == KEY_LEFTARROW) {
                // 长按LEFT → ESCAPE (菜单)
                *pressed = 1;
                *key = KEY_ESCAPE;
                return 1;
            }
        }
    }
    return 0;
}

extern "C" uint32_t DG_GetTicksMs() {
    return millis();
}

extern "C" void DG_SleepMs(uint32_t ms) {
    delay(ms);
}

extern "C" void DG_SetWindowTitle(const char *title) {
    // 不需要
}
