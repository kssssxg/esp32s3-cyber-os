// smsplus_adapter.cpp — SMS/GameGear 模拟器适配层 (Arduino + TFT_eSPI)
// 适配自 ducalex/retro-go 的 smsplus 核心

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <esp_heap_caps.h>

extern "C" {
#include "shared.h"
#include "system.h"
#include "loadrom.h"
#include "render.h"
#include "sms.h"
}

// 外部TFT对象 (s3.ino中定义)
extern TFT_eSPI tft;
extern void unloadFontSafe();
extern void loadFontSafe();

// SMS分辨率
#define SMS_W 256
#define SMS_H 192

// GG分辨率
#define GG_W 160
#define GG_H 144

// 按键引脚
#define BTN_UP     4
#define BTN_OK     5
#define BTN_DOWN   16
#define BTN_LEFT   17
#define BTN_RIGHT  47

// 调色板 (256色 RGB565)
static uint16_t palette_buf[256];

// RGB565转换缓冲 (最大256x192)
static uint16_t *conv_buf = NULL;

// 运行SMS/GameGear模拟器
void runSmsPlus(String romPath) {
    Serial.printf("[SMS] 启动SMS/GG: %s\n", romPath.c_str());

    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Loading SMS...", 65, 110);

    // 检查ROM文件
    if (!SD.exists(romPath)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("ROM not found!", 60, 110);
        delay(2000);
        loadFontSafe();
        return;
    }

    // 分配转换缓冲 (PSRAM)
    conv_buf = (uint16_t*)heap_caps_malloc(SMS_W * SMS_H * 2, MALLOC_CAP_SPIRAM);
    if (!conv_buf) {
        tft.drawString("PSRAM alloc failed!", 40, 130);
        delay(2000);
        loadFontSafe();
        return;
    }

    // 初始化SMS Plus
    system_reset_config();
    option.sndrate = 22050;
    option.overscan = 0;
    option.extra_gg = 0;
    option.tms_pal = 0;

    // 判断是SMS还是GG
    String ext;
    int dotIdx = romPath.lastIndexOf('.');
    if (dotIdx > 0) ext = romPath.substring(dotIdx + 1);
    ext.toLowerCase();

    if (ext == "gg") {
        option.console = CONSOLE_GG;
    } else {
        option.console = CONSOLE_SMS;
    }

    // 加载ROM
    char romPathBuf[64];
    romPath.toCharArray(romPathBuf, sizeof(romPathBuf));
    if (!load_rom_file(romPathBuf)) {
        Serial.println("[SMS] ROM加载失败");
        heap_caps_free(conv_buf);
        loadFontSafe();
        return;
    }

    // 设置bitmap
    bitmap.width = SMS_W;
    bitmap.height = SMS_H;
    bitmap.pitch = bitmap.width;
    bitmap.data = (unsigned char*)heap_caps_malloc(SMS_W * SMS_H, MALLOC_CAP_SPIRAM);
    if (!bitmap.data) {
        Serial.println("[SMS] Bitmap alloc failed");
        heap_caps_free(conv_buf);
        loadFontSafe();
        return;
    }

    system_poweron();

    Serial.println("[SMS] Running...");

    // 主循环
    while (true) {
        // 读取按键
        uint8_t pad = 0;
        if (digitalRead(BTN_UP) == LOW)    pad |= INPUT_UP;
        if (digitalRead(BTN_DOWN) == LOW)  pad |= INPUT_DOWN;
        if (digitalRead(BTN_LEFT) == LOW)  pad |= INPUT_LEFT;
        if (digitalRead(BTN_RIGHT) == LOW) pad |= INPUT_RIGHT;
        if (digitalRead(BTN_OK) == LOW)    pad |= INPUT_BUTTON1;
        input.pad[0] = pad;
        input.system = 0;

        // 运行一帧
        system_frame(0);

        // 获取调色板
        render_copy_palette(palette_buf);

        // 8-bit paletted → RGB565 转换
        int w = bitmap.viewport.w;
        int h = bitmap.viewport.h;
        int ox = bitmap.viewport.x;
        int oy = bitmap.viewport.y;

        if (option.console == CONSOLE_GG) {
            // GameGear: 160x144，居中显示
            for (int y = 0; y < h && y < GG_H; y++) {
                uint8_t *src = bitmap.data + (oy + y) * SMS_W + ox;
                uint16_t *dst = conv_buf + y * w;
                for (int x = 0; x < w && x < GG_W; x++) {
                    dst[x] = palette_buf[src[x]];
                }
            }
            int dispX = (240 - w) / 2;
            int dispY = (240 - h) / 2;
            tft.pushImage(dispX, dispY, w, h, conv_buf);
        } else {
            // SMS: 256x192，居中显示
            for (int y = 0; y < h && y < SMS_H; y++) {
                uint8_t *src = bitmap.data + (oy + y) * SMS_W + ox;
                uint16_t *dst = conv_buf + y * w;
                for (int x = 0; x < w && x < SMS_W; x++) {
                    dst[x] = palette_buf[src[x]];
                }
            }
            int dispX = (240 - w) / 2;
            int dispY = (240 - h) / 2;
            tft.pushImage(dispX, dispY, w, h, conv_buf);
        }

        // 检测退出 (LEFT+RIGHT同时按)
        if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == LOW) {
            Serial.println("[SMS] Exit combo detected");
            break;
        }
    }

    // 清理
    system_poweroff();
    if (bitmap.data) {
        heap_caps_free(bitmap.data);
        bitmap.data = NULL;
    }
    heap_caps_free(conv_buf);
    conv_buf = NULL;

    Serial.println("[SMS] Exited");
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
}
