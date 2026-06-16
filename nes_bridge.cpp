// nes_bridge.cpp - NES模拟器桥接层
// 直接使用nes文件夹里的现成emucore代码，通过TFT_eSPI显示和GPIO按键适配s3.ino
#include <Arduino.h>
#include <stdarg.h>
#include <esp_heap_caps.h>
#define NES_USE_SERIAL  // 启用Serial输出
#include <TFT_eSPI.h>
#include <SD.h>
#include <Preferences.h>
#include <driver/i2s_std.h>
#include "nes_config.h"
#include "emucore.h"
#include "cpumacro.h"
#include "nes_palette.h"
#include "nes_display_stub.h"
#include "nes_controller_stub.h"

// ===================== 全局变量定义 =====================
uint8_t *SCREENMEMORY = nullptr;
void *TASK_VID_HANDLE = nullptr;

volatile nes_pad_key_s gamepad_p1;
volatile nes_pad_key_s gamepad_p2;

RUNNING_CFG cfg = {0};
// NESmachine, apu, mmc 在 emucore.cpp 中定义
extern nes_t *NESmachine;
extern apu_t apu;
extern mmc_t mmc;
extern void apu_process(void *buffer, int num_samples);

uint8_t *cachedRom = nullptr;
FILE_OBJ *romFiles = nullptr;

// 外部TFT对象 (s3.ino中定义)
extern TFT_eSPI tft;
extern int pngXpos;
extern int pngYpos;

// NES按键引脚 (与s3.ino一致)
#define BTN_UP     4
#define BTN_OK     5
#define BTN_DOWN   16
#define BTN_LEFT   17
#define BTN_RIGHT  47

// I2S 音频引脚 (与s3.ino一致)
#define NES_I2S_BCLK  20
#define NES_I2S_LRC   21
#define NES_I2S_DOUT  12
#define NES_I2S_SD    18

// 音频状态
static bool nes_i2s_initialized = false;
static uint8_t nes_volume = 50;  // 0-100, 从 Preferences 加载
static i2s_chan_handle_t nes_i2s_tx = nullptr;

// ===================== 输入设备实现 =====================
// pad0_readcount在emucore.cpp中定义

void input_init() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    gamepad_p1.KEY_VALUE = 0;
    gamepad_p2.KEY_VALUE = 0;
}

// input_clear 不再需要，input_refresh 每帧完整更新状态
void input_clear() {}

// OK键: 按住=A(跳跃)
// 组合键: 上+右=B, 左+右=Start, 上+下=Select, 上+左=退出
#define COMBO_HOLD_MS 250

void input_refresh() {
    uint8_t val = 0;
    unsigned long now = millis();

    bool up = (digitalRead(BTN_UP) == LOW);
    bool down = (digitalRead(BTN_DOWN) == LOW);
    bool left = (digitalRead(BTN_LEFT) == LOW);
    bool right = (digitalRead(BTN_RIGHT) == LOW);

    // ---- 组合键检测 (Windows风格) ----
    // 先按住一个键超过COMBO_HOLD_MS，再按对向键→触发Start/Select
    // 触发时屏蔽两个方向键，只输出Start/Select
    static unsigned long leftDownTime = 0, rightDownTime = 0;
    static unsigned long upDownTime = 0, downDownTime = 0;
    static bool leftWasDown = false, rightWasDown = false;
    static bool upWasDown = false, downWasDown = false;
    static bool startTriggered = false, selectTriggered = false, bTriggered = false;

    // 记录各键首次按下的时间
    if (left && !leftWasDown) leftDownTime = now;
    if (right && !rightWasDown) rightDownTime = now;
    if (up && !upWasDown) upDownTime = now;
    if (down && !downWasDown) downDownTime = now;
    leftWasDown = left;
    rightWasDown = right;
    upWasDown = up;
    downWasDown = down;

    // START: 左+右组合
    if (left && right) {
        // 哪个键先按住超过阈值?
        if (!startTriggered && (now - leftDownTime > COMBO_HOLD_MS || now - rightDownTime > COMBO_HOLD_MS)) {
            startTriggered = true;
        }
        if (startTriggered) {
            val |= 0x08;  // START
            left = false; right = false;  // 屏蔽方向键
        }
    } else {
        startTriggered = false;
    }

    // SELECT: 上+下组合
    if (up && down) {
        if (!selectTriggered && (now - upDownTime > COMBO_HOLD_MS || now - downDownTime > COMBO_HOLD_MS)) {
            selectTriggered = true;
        }
        if (selectTriggered) {
            val |= 0x04;  // SELECT
            up = false; down = false;  // 屏蔽方向键
        }
    } else {
        selectTriggered = false;
    }

    // B键: 上+右组合
    if (up && right) {
        if (!bTriggered && (now - upDownTime > COMBO_HOLD_MS || now - rightDownTime > COMBO_HOLD_MS)) {
            bTriggered = true;
        }
        if (bTriggered) {
            val |= 0x02;  // B
            up = false; right = false;  // 屏蔽方向键
        }
    } else {
        bTriggered = false;
    }

    // 方向键映射 (仅在非组合模式下)
    if (up)    val |= 0x10;
    if (down)  val |= 0x20;
    if (left)  val |= 0x40;
    if (right) val |= 0x80;

    // OK键: 按住=A
    if (digitalRead(BTN_OK) == LOW) {
        val |= 0x01;  // A
    }

    gamepad_p1.KEY_VALUE = val;

    // 调试输出
    static unsigned long lastDbg = 0;
    static uint8_t lastVal = 0;
    if (val != lastVal && now - lastDbg > 100) {
        lastDbg = now;
        lastVal = val;
        Serial.printf("[PAD] 0x%02X %s%s%s%s%s%s%s%s\n", val,
            (val & 0x01) ? "A " : "",
            (val & 0x02) ? "B " : "",
            (val & 0x04) ? "SEL " : "",
            (val & 0x08) ? "STA " : "",
            (val & 0x10) ? "U " : "",
            (val & 0x20) ? "D " : "",
            (val & 0x40) ? "L " : "",
            (val & 0x80) ? "R " : "");
    }
}

// emucore.cpp中已定义input_strobe和get_pad0/get_pad1
// 每帧重置shift register计数器
extern "C" void input_reset_readcount(void);

// 这里只需要定义gamepad_p1/gamepad_p2全局变量(在文件顶部)

// ===================== 显示实现 =====================
// 将NES的8bit索引色帧缓冲推送到TFT_eSPI
// 支持自定义拉伸: 等比(居中) / 强制(拉满)

static uint16_t nes_lineBuf[240];  // 最大宽度
static uint8_t nes_stretch = 0;
static uint16_t nes_outW = 240;
static uint16_t nes_outH = 140;

void nes_refresh_display() {
    if (!SCREENMEMORY) return;

    uint8_t stretch = nes_stretch;
    uint16_t outW = nes_outW;
    uint16_t outH = nes_outH;

    if (outW > 240) outW = 240;
    if (outH > 240) outH = 240;
    if (outW < 120) outW = 120;
    if (outH < 80) outH = 80;

    // 等比模式: 按NES原始256:240比例计算
    if (stretch == 0) {
        // 以宽度为基准算高度, 或以高度为基准算宽度, 取较小者
        int hByW = outW * 240 / 256;
        int wByH = outH * 256 / 240;
        if (hByW <= outH) {
            outH = hByW;
        } else {
            outW = wByH;
        }
    }
    // 强制模式: 直接使用用户设定的宽高

    int outX = (240 - outW) / 2;
    int outY = (240 - outH) / 2;

    tft.startWrite();
    tft.setAddrWindow(outX, outY, outW, outH);

    for (int oy = 0; oy < outH; oy++) {
        int sy = (oy * NES_SCREEN_HEIGHT) / outH;
        if (sy >= NES_SCREEN_HEIGHT) sy = NES_SCREEN_HEIGHT - 1;
        uint8_t *src = SCREENMEMORY + sy * NES_SCREEN_WIDTH;

        for (int ox = 0; ox < outW; ox++) {
            int sx = (ox * NES_SCREEN_WIDTH) / outW;
            if (sx >= NES_SCREEN_WIDTH) sx = NES_SCREEN_WIDTH - 1;
            nes_lineBuf[ox] = nes_palette_256[src[sx] & 0x3F];
        }
        tft.pushColors(nes_lineBuf, outW);
    }
    tft.endWrite();
}

// ===================== NES 音频 (直接写入 I2S, 大DMA缓冲) =====================
void nes_i2s_init() {
    if (nes_i2s_initialized) return;

    // 从 flash 读取音量和拉伸参数
    Preferences prefs;
    prefs.begin("fc-config", true);
    nes_volume = prefs.getUChar("volume", 50);
    nes_stretch = prefs.getUChar("stretch", 0);
    nes_outW = prefs.getUShort("nesW", 240);
    nes_outH = prefs.getUShort("nesH", 140);
    prefs.end();
    Serial.printf("[NES] Volume: %d%%, Stretch: %s, Size: %dx%d\n",
        nes_volume, nes_stretch ? "forced" : "fit", nes_outW, nes_outH);

    // 启用功放
    pinMode(NES_I2S_SD, OUTPUT);
    digitalWrite(NES_I2S_SD, HIGH);

    // I2S 配置: 大DMA缓冲区吸收帧率波动
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 16;
    chan_cfg.dma_frame_num = 400;
    i2s_new_channel(&chan_cfg, &nes_i2s_tx, NULL);

    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.sample_rate_hz = 24000;
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    std_cfg.gpio_cfg.bclk = (gpio_num_t)NES_I2S_BCLK;
    std_cfg.gpio_cfg.ws = (gpio_num_t)NES_I2S_LRC;
    std_cfg.gpio_cfg.dout = (gpio_num_t)NES_I2S_DOUT;
    std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;

    i2s_channel_init_std_mode(nes_i2s_tx, &std_cfg);
    i2s_channel_enable(nes_i2s_tx);

    nes_i2s_initialized = true;
    Serial.println("[NES] I2S audio initialized");
}

static int16_t *nes_monoBuf = nullptr;
static int16_t *nes_stereoBuf = nullptr;

void nes_audio_frame() {
    if (!nes_i2s_initialized || !nes_i2s_tx || !NESmachine || !NESmachine->apu) return;

    // 根据实际帧时间计算采样数 (解决慢帧率导致的音频欠采样)
    static unsigned long lastAudioTime = 0;
    unsigned long now = millis();
    int num_samples;
    if (lastAudioTime == 0) {
        num_samples = 24000 / NES_REFRESH_RATE;  // 首帧: 400
    } else {
        unsigned long elapsed_ms = now - lastAudioTime;
        num_samples = (int)(24000UL * elapsed_ms / 1000);
        if (num_samples < 100) num_samples = 100;
        if (num_samples > 500) num_samples = 500;  // 最大500 (缓冲区限制)
    }
    lastAudioTime = now;

    if (!nes_monoBuf) {
        nes_monoBuf = (int16_t *)heap_caps_malloc(500 * sizeof(int16_t), MALLOC_CAP_DMA);
        if (!nes_monoBuf) return;
    }
    if (!nes_stereoBuf) {
        nes_stereoBuf = (int16_t *)heap_caps_malloc(500 * 2 * sizeof(int16_t), MALLOC_CAP_DMA);
        if (!nes_stereoBuf) return;
    }

    // APU 生成样本
    apu_process(nes_monoBuf, num_samples);

    // 转立体声并应用音量
    int32_t gain = (int32_t)(nes_volume * 0.35f);
    for (int i = 0; i < num_samples; i++) {
        int16_t s = (int16_t)((nes_monoBuf[i] * gain) / 100);
        nes_stereoBuf[i * 2] = s;
        nes_stereoBuf[i * 2 + 1] = s;
    }

    // 直接写入 I2S (重试直到全部写入)
    size_t totalBytes = num_samples * 2 * sizeof(int16_t);
    size_t written = 0;
    while (written < totalBytes) {
        size_t w = 0;
        esp_err_t err = i2s_channel_write(nes_i2s_tx, (uint8_t *)nes_stereoBuf + written, totalBytes - written, &w, pdMS_TO_TICKS(100));
        if (err != ESP_OK) break;
        written += w;
    }
}

void nes_i2s_deinit() {
    if (!nes_i2s_initialized) return;
    if (nes_i2s_tx) {
        i2s_channel_disable(nes_i2s_tx);
        i2s_del_channel(nes_i2s_tx);
        nes_i2s_tx = nullptr;
    }
    digitalWrite(NES_I2S_SD, LOW);
    nes_i2s_initialized = false;
    Serial.println("[NES] I2S audio deinitialized");
}

// ===================== NES模拟器入口 =====================
void runNesEmulator(String romPath) {
    Serial.println("[NES] Starting emulator for: " + romPath);

    // 1. 分配帧缓冲 (PSRAM)
    if (!SCREENMEMORY) {
        SCREENMEMORY = (uint8_t *)heap_caps_malloc(NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    }
    if (!SCREENMEMORY) {
        Serial.println("[NES] Failed to allocate SCREENMEMORY");
        goto cleanup;
    }
    memset(SCREENMEMORY, 0, NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT);

    // 2. 设置最大ROM大小
    {
        uint32_t psram = ESP.getPsramSize();
        if (psram == 0) {
            Serial.println("[NES] No PSRAM!");
            goto cleanup;
        }
        cfg.romMaxSize = (psram <= 0x200000) ? 0x120000 : 0x200000;
    }

    // 3. 分配ROM缓存 (PSRAM)
    if (!cachedRom) {
        cachedRom = (uint8_t *)heap_caps_malloc(cfg.romMaxSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (!cachedRom) {
        Serial.println("[NES] Failed to allocate ROM cache");
        goto cleanup;
    }

    // 4. 创建NES机器
    if (!NESmachine) {
        NESmachine = nes_create();
    }
    if (!NESmachine) {
        Serial.println("[NES] Failed to create NES machine");
        goto cleanup;
    }
    if (!NESmachine->rominfo) {
        NESmachine->rominfo = (rominfo_t *)heap_caps_malloc(sizeof(rominfo_t), MALLOC_CAP_DMA);
    }
    if (!NESmachine->rominfo) {
        Serial.println("[NES] Failed to allocate rominfo");
        goto cleanup;
    }

    // 5. 初始化输入
    input_init();

    // 6. 加载ROM文件到PSRAM
    Serial.println("[NES] Loading ROM...");
    {
        File fp = SD.open(romPath);
        if (!fp) {
            Serial.println("[NES] Cannot open ROM file");
            goto cleanup;
        }
        uint32_t fileSize = fp.size();
        if (fileSize > cfg.romMaxSize || fileSize < 16) {
            Serial.println("[NES] ROM file too large or too small");
            fp.close();
            goto cleanup;
        }
        uint32_t offset = 0;
        while (offset < fileSize) {
            size_t toRead = min((uint32_t)ROM_READING_BUFFER_SIZE, fileSize - offset);
            size_t bytesRead = fp.read(cachedRom + offset, toRead);
            if (bytesRead == 0) break;
            offset += bytesRead;
        }
        fp.close();
        Serial.printf("[NES] ROM loaded: %u bytes\n", offset);
    }

    // 7. 解析ROM头
    {
        uint8_t *romdata = cachedRom;
        memset(NESmachine->rominfo, 0, sizeof(rominfo_t));
        if (rom_getheader(&romdata, NESmachine->rominfo)) {
            Serial.println("[NES] Invalid NES ROM header");
            goto cleanup;
        }

        // 8. 检查Mapper支持
        if (!mmc_peek(NESmachine->rominfo->mapper_number)) {
            Serial.printf("[NES] Unsupported mapper: %d\n", NESmachine->rominfo->mapper_number);
            goto cleanup;
        }

        // 9. 分配SRAM
        if (rom_allocsram(&(NESmachine->rominfo))) {
            Serial.println("[NES] Failed to allocate SRAM");
            goto cleanup;
        }

        // 10. 加载Trainer和ROM数据
        rom_loadtrainer(&romdata, NESmachine->rominfo);
        if (rom_loadrom(&romdata, NESmachine->rominfo)) {
            Serial.println("[NES] Failed to load ROM data");
            goto cleanup;
        }
    }

    // ROM信息
    Serial.printf("[NES] Mapper: %d, PRG: %dKB, CHR: %d\n",
        NESmachine->rominfo->mapper_number,
        NESmachine->rominfo->rom_banks * 16,
        NESmachine->rominfo->vrom_banks);

    // 11. 配置MMC (Mapper)
    if (NESmachine->rominfo->sram) {
        NESmachine->cpu->mem_page[6] = NESmachine->rominfo->sram;
        NESmachine->cpu->mem_page[7] = NESmachine->rominfo->sram + 0x1000;
    }
    NESmachine->mmc = mmc_create(NESmachine->rominfo);
    if (!NESmachine->mmc) {
        Serial.println("[NES] Failed to create MMC");
        goto cleanup;
    }
    if (NESmachine->rominfo->vram) {
        NESmachine->ppu->vram_present = true;
    }

    // 12. 音频初始化 (I2S + 从flash读取音量)
    nes_i2s_init();

    // 13. 构建地址处理器并重置NES
    build_address_handlers(NESmachine);
    nes_setcontext(NESmachine);
    nes_reset();

    // 14. 准备显示
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(0);

    Serial.println("[NES] Emulator running!");
    cfg.nesPower = 1;

    // 15. 主循环 (直接使用nes的nes_renderframe)
    {
    unsigned long lastFrame = millis();
    unsigned long allDirPressTime = 0;
    bool allDirWasPressed = false;
    int exitReleaseCount = 0;  // 连续未按帧数 (防抖)

    while (cfg.nesPower == 1) {
        // 上+左同时按住500ms退出游戏 (带防抖: 连续3帧未按才重置)
        bool bUp = (digitalRead(BTN_UP) == LOW);
        bool bLeft = (digitalRead(BTN_LEFT) == LOW);
        bool exitCombo = bUp && bLeft;

        if (exitCombo) {
            exitReleaseCount = 0;
            if (!allDirWasPressed) {
                allDirWasPressed = true;
                allDirPressTime = millis();
            } else if (millis() - allDirPressTime > 500) {
                cfg.nesPower = 0;
                break;
            }
            // 退出进度条 (屏幕顶部绿色)
            unsigned long held = millis() - allDirPressTime;
            int barW = (int)(held * 240 / 500);
            if (barW > 240) barW = 240;
            tft.fillRect(0, 0, barW, 3, TFT_GREEN);
        } else {
            if (allDirWasPressed) {
                exitReleaseCount++;
                if (exitReleaseCount >= 3) {
                    allDirWasPressed = false;
                    exitReleaseCount = 0;
                    tft.fillRect(0, 0, 240, 3, TFT_BLACK);
                }
            }
        }

        // 刷新输入
        input_clear();
        input_refresh();

        // 退出组合键激活时，屏蔽上+左方向输入，防止游戏吃到
        if (exitCombo) {
            gamepad_p1.KEY_VALUE &= ~(0x10 | 0x40);  // 清除 UP 和 LEFT 位
        }

        // 每帧重置shift register计数器，确保手柄读取从bit0开始
        input_reset_readcount();

        // 运行一帧 (带超时检测)
        unsigned long frameStart = millis();
        nes_renderframe(true);
        unsigned long frameTime = millis() - frameStart;
        if (frameTime > 100) {
            Serial.printf("[NES] Frame took %lu ms! Possible freeze.\n", frameTime);
        }

        // 帧调试: 前几帧和按键时打印
        {
            extern volatile int get_pad0_calls;
            extern volatile uint8_t last_ram_0000;
            static int frameCount = 0;
            static uint8_t lastPrintedRaw = 0xFF;
            uint8_t pv = gamepad_p1.KEY_VALUE;
            frameCount++;
            if (frameCount <= 10 || pv != lastPrintedRaw || pv != 0) {
                Serial.printf("[F%d] raw=%02X ctrl=%02X rds=%d\n",
                    frameCount, pv, last_ram_0000, get_pad0_calls);
                lastPrintedRaw = pv;
            }
            get_pad0_calls = 0;
        }

        // 刷新显示
        nes_refresh_display();

        // 左上角实时帧率
        {
            static int fpsCount = 0;
            static unsigned long fpsTimer = millis();
            static char fpsStr[8] = "0";
            fpsCount++;
            unsigned long now = millis();
            if (now - fpsTimer >= 1000) {
                snprintf(fpsStr, sizeof(fpsStr), "%d", fpsCount);
                fpsCount = 0;
                fpsTimer = now;
            }
            tft.fillRect(0, 5, 30, 12, TFT_BLACK);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.setTextSize(1);
            tft.drawString(fpsStr, 2, 5);
        }

        // 输出音频 (与渲染同上下文，APU状态一致)
        nes_audio_frame();

        // 帧同步
        unsigned long now = millis();
        long elapsed = now - lastFrame;
        if (elapsed < (1000 / NES_REFRESH_RATE)) {
            delay(1000 / NES_REFRESH_RATE - elapsed);
        }
        lastFrame = millis();

        yield();
    }
    } // end main loop block

    // 16. 清理
    Serial.println("[NES] Emulator stopped");

    // 等待退出组合键释放
    while ((digitalRead(BTN_UP) == LOW) && (digitalRead(BTN_LEFT) == LOW)) delay(50);
    delay(200);

cleanup:
    // 关闭I2S音频
    nes_i2s_deinit();

    // 检查堆完整性，损坏则跳过free避免crash
    bool heapOk = heap_caps_check_integrity_all(true);
    if (!heapOk) {
        Serial.println("[NES] WARNING: Heap corrupted, skipping free to avoid crash");
    }

    // rominfo->rom/vrom 指向 cachedRom 内部, 不能让 rom_free 释放
    if (heapOk && NESmachine && NESmachine->rominfo) {
        NESmachine->rominfo->rom = NULL;
        NESmachine->rominfo->vrom = NULL;
    }

    // 释放NES机器 (内部会释放rominfo/sram/vram/cpu/ppu/apu/mmc)
    if (heapOk && NESmachine) {
        nes_destroy(&NESmachine);
        NESmachine = NULL;
    }

    // 释放帧缓冲
    if (heapOk && SCREENMEMORY) {
        free(SCREENMEMORY);
        SCREENMEMORY = NULL;
    }

    // 释放ROM缓存
    if (heapOk && cachedRom) {
        free(cachedRom);
        cachedRom = NULL;
    }

    // 释放音频缓冲区
    extern int16_t *nes_monoBuf;
    extern int16_t *nes_stereoBuf;
    if (heapOk && nes_monoBuf) { free(nes_monoBuf); nes_monoBuf = NULL; }
    if (heapOk && nes_stereoBuf) { free(nes_stereoBuf); nes_stereoBuf = NULL; }

    Serial.printf("[NES] Freed. Free heap: %dKB, PSRAM: %dKB\n",
        ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);

    tft.fillScreen(TFT_BLACK);
}

// ===================== emucore.cpp需要的外部函数 =====================
// 音频信息 (直接从nes原项目的sound.cpp复制)
void osd_getsoundinfo(sndinfo_t *info) {
    info->sample_rate = 24000; // DEFAULT_SAMPLERATE
    info->bps = 16;
}

// 日志桥接 - emucore.cpp通过此函数输出Serial日志
extern "C" void nes_log(const char *msg) {
    Serial.print(msg);
}
extern "C" void nes_printf(const char *fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
}

// 手柄状态桥接函数 (C链接，供emucore.cpp调用)
extern "C" uint8_t nes_bridge_get_pad0_state(void) {
    return gamepad_p1.KEY_VALUE;
}

extern "C" uint8_t nes_bridge_get_pad1_state(void) {
    return gamepad_p2.KEY_VALUE;
}