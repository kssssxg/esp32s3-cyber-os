// gnuboy_adapter.cpp — GameBoy/GBC 模拟器适配层 (Arduino + TFT_eSPI)
// 适配自 ducalex/retro-go 的 gnuboy 核心

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <esp_heap_caps.h>

extern "C" {
#include "gnuboy.h"
}

// 外部TFT对象 (s3.ino中定义)
extern TFT_eSPI tft;
extern void unloadFontSafe();
extern void loadFontSafe();

// GB分辨率
#define GB_W 160
#define GB_H 144

// 居中显示偏移 (240x240 TFT)
#define GB_OFS_X ((240 - GB_W) / 2)  // = 40
#define GB_OFS_Y ((240 - GB_H) / 2)  // = 48

// 按键引脚
#define BTN_UP     4
#define BTN_OK     5
#define BTN_DOWN   16
#define BTN_LEFT   17
#define BTN_RIGHT  47

// 帧缓冲 (RGB565, PSRAM)
static uint16_t *framebuf = NULL;

// 视频回调 — gnuboy每帧渲染完调用
static void video_callback(void *buffer) {
    // buffer就是framebuf，直接推送到TFT
    tft.pushImage(GB_OFS_X, GB_OFS_Y, GB_W, GB_H, framebuf);
}

// 音频回调 (暂时不用)
static void audio_callback(void *buffer, size_t length) {
    // TODO: I2S音频输出
}

// 读取按钮状态 → GB按键位掩码
static int read_gb_buttons() {
    int pad = 0;
    if (digitalRead(BTN_UP) == LOW)    pad |= GB_PAD_UP;
    if (digitalRead(BTN_DOWN) == LOW)  pad |= GB_PAD_DOWN;
    if (digitalRead(BTN_LEFT) == LOW)  pad |= GB_PAD_LEFT;
    if (digitalRead(BTN_RIGHT) == LOW) pad |= GB_PAD_RIGHT;
    if (digitalRead(BTN_OK) == LOW)    pad |= GB_PAD_A;
    // TODO: 组合键映射 B/Start/Select
    return pad;
}

// 运行GameBoy模拟器
void runGnuboy(String romPath) {
    Serial.printf("[GNUBOY] 启动GameBoy: %s\n", romPath.c_str());

    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Loading GB...", 70, 110);

    // 检查ROM文件
    if (!SD.exists(romPath)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("ROM not found!", 60, 110);
        delay(2000);
        loadFontSafe();
        return;
    }

    // 分配帧缓冲 (PSRAM)
    framebuf = (uint16_t*)heap_caps_malloc(GB_W * GB_H * 2, MALLOC_CAP_SPIRAM);
    if (!framebuf) {
        tft.drawString("PSRAM alloc failed!", 40, 130);
        delay(2000);
        loadFontSafe();
        return;
    }

    // 初始化gnuboy
    int err = gnuboy_init(22050, GB_AUDIO_STEREO_S16, GB_PIXEL_565_LE, video_callback, audio_callback);
    if (err != 0) {
        Serial.printf("[GNUBOY] Init failed: %d\n", err);
        heap_caps_free(framebuf);
        loadFontSafe();
        return;
    }

    gnuboy_set_framebuffer(framebuf);

    // 加载ROM
    char romPathBuf[64];
    romPath.toCharArray(romPathBuf, sizeof(romPathBuf));
    err = gnuboy_load_rom_file(romPathBuf);
    if (err != 0) {
        Serial.printf("[GNUBOY] Load ROM failed: %d\n", err);
        heap_caps_free(framebuf);
        loadFontSafe();
        return;
    }

    gnuboy_reset(true);

    Serial.println("[GNUBOY] Running...");

    // 主循环
    while (true) {
        // 读取按键
        int pad = read_gb_buttons();
        gnuboy_set_pad(pad);

        // 运行一帧
        gnuboy_run(true);

        // 检测退出 (LEFT+RIGHT同时按)
        if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == LOW) {
            Serial.println("[GNUBOY] Exit combo detected");
            break;
        }
    }

    // 清理
    gnuboy_free_rom();
    heap_caps_free(framebuf);
    framebuf = NULL;

    Serial.println("[GNUBOY] Exited");
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
}
