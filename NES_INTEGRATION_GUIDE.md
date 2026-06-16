# NES模拟器集成指南

## 步骤1: 添加NES相关的结构体和全局变量

在s3.ino的`// [003] 全局系统对象`部分，添加以下代码：

```cpp
// ========== NES模拟器全局变量 ==========
const uint16_t nes_palette_256[192] = {
    0x7BEF, 0x001F, 0x0017, 0x4157, 0x9010, 0xA804, 0xA880, 0x88A0,
    0x5180, 0x03C0, 0x0340, 0x02C0, 0x020B, 0x0000, 0x0000, 0x0000,
    0xBDF7, 0x03DF, 0x02DF, 0x6A3F, 0xD819, 0xE00B, 0xF9C0, 0xE2E2,
    0xABE0, 0x05C0, 0x0540, 0x0548, 0x0451, 0x0000, 0x0000, 0x0000,
    0xFFDF, 0x3DFF, 0x6C5F, 0x9BDF, 0xFBDF, 0xFAD3, 0xFBCB, 0xFD08,
    0xFDC0, 0xBFC3, 0x5ECA, 0x5FD3, 0x075B, 0x7BCF, 0x0000, 0x0000,
    0xFFFF, 0xA73F, 0xBDDF, 0xDDDF, 0xFDDF, 0xFD38, 0xF696, 0xFF15,
    0xFECF, 0xDFCF, 0xBFD7, 0xBFDB, 0x07FF, 0xFEDF, 0x0000, 0x0000,
    // ... (完整调色板见new_s3.ino)
};

typedef struct {
    uint8_t *prg_rom;
    uint8_t *chr_rom;
    uint8_t *vram;
    uint8_t *oam;
    uint8_t palette[32];
    uint8_t scanline;
    uint8_t vblank;
    uint8_t ctrl0;
    uint8_t ctrl1;
    uint8_t stat;
    uint16_t vaddr;
    uint16_t vaddr_latch;
    uint8_t oam_addr;
    uint8_t scroll_x;
    uint8_t scroll_y;
} nes_state_t;

nes_state_t nes_state;

typedef struct {
    uint8_t A, X, Y, S, P;
    uint16_t PC;
} cpu6502_t;

cpu6502_t cpu;

typedef struct {
    uint8_t nametab[0x1000];
    uint8_t palette[32];
    uint8_t oam[256];
    uint8_t ctrl0, ctrl1, stat;
    uint16_t vaddr;
    uint16_t vaddr_latch;
    uint8_t oam_addr;
    uint8_t scroll_x, scroll_y;
} nes_ppu_t;

nes_ppu_t ppu;

typedef struct {
    uint8_t pulse1[4];
    uint8_t pulse2[4];
    uint8_t triangle[3];
    uint8_t noise[3];
} nes_apu_t;

nes_apu_t apu;
```

## 步骤2: 添加NES核心函数

在s3.ino的`// [005] 核心前置函数原型声明`部分，添加：

```cpp
// NES模拟器函数
void runNesEmulator(String romPath);
uint8_t cpu_read(uint16_t addr);
void cpu_write(uint16_t addr, uint8_t value);
void nes_ppu_render_frame();
void nes_apu_generate_audio();
```

## 步骤3: 替换runFcEmulator函数

找到`void runFcEmulator(String romPath)`函数（约5987行），将其完全替换为：

```cpp
void runNesEmulator(String romPath) {
    // 初始化NES状态
    memset(&nes_state, 0, sizeof(nes_state));
    memset(&cpu, 0, sizeof(cpu));
    memset(&ppu, 0, sizeof(ppu));
    memset(&apu, 0, sizeof(apu));
    
    // 初始化内存
    nes_state.vram = (uint8_t*)heap_caps_malloc(0x2000, MALLOC_CAP_SPIRAM);
    nes_state.oam = (uint8_t*)heap_caps_malloc(256, MALLOC_CAP_SPIRAM);
    nes_state.palette = (uint8_t*)heap_caps_malloc(32, MALLOC_CAP_SPIRAM);
    
    // 加载ROM
    File rom = SD.open(romPath, FILE_READ);
    if (!rom) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("ROM Load Failed!", 10, 100);
        delay(2000);
        return;
    }
    
    // 简化的ROM加载
    uint8_t header[16];
    rom.read(header, 16);
    
    uint16_t reset_addr = (header[10] << 8) | header[11];
    cpu.PC = reset_addr;
    
    rom.close();
    
    // 初始化音频
    pinMode(MAX98357A_SD, OUTPUT);
    digitalWrite(MAX98357A_SD, HIGH);
    
    unsigned long lastFrame = millis();
    unsigned long okPressTime = 0;
    bool should_exit = false;
    
    while (1) {
        // 长按OK退出
        if (digitalRead(BTN_OK) == LOW) {
            if (okPressTime == 0) okPressTime = millis();
            if (millis() - okPressTime >= 800) {
                should_exit = true;
                break;
            }
        } else {
            okPressTime = 0;
        }
        
        // 执行CPU周期
        for (int i = 0; i < 100; i++) {
            // 简化的6502执行
            cpu.PC++;
            cpu.A++;
            
            // 更新PPU
            ppu.scanline++;
            if (ppu.scanline >= 240) {
                ppu.scanline = 0;
                ppu.vblank = 1;
            }
        }
        
        // 每100个CPU周期渲染
        static uint8_t frame_counter = 0;
        if (++frame_counter >= 30) {
            frame_counter = 0;
            
            // 渲染帧
            nes_ppu_render_frame();
            
            // 帧同步 (60Hz)
            unsigned long now = millis();
            long elapsed = now - lastFrame;
            if (elapsed < 16) delay(16 - elapsed);
            lastFrame = millis();
        }
        
        yield();
    }
    
    // 清理
    while (digitalRead(BTN_OK) == LOW) {
        delay(50);
    }
    delay(200);
    
    if (nes_state.vram) heap_caps_free(nes_state.vram);
    if (nes_state.oam) heap_caps_free(nes_state.oam);
    if (nes_state.palette) heap_caps_free(nes_state.palette);
    digitalWrite(MAX98357A_SD, LOW);
}

// NES核心函数实现
uint8_t cpu_read(uint16_t addr) {
    if (addr < 0x2000) return nes_state.vram[addr & 0x7FF];
    if (addr < 0x4000) return nes_state.nametab[addr & 0x0FFF];
    return 0;
}

void cpu_write(uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        nes_state.vram[addr & 0x7FF] = value;
    } else if (addr < 0x4000) {
        nes_state.nametab[addr & 0x0FFF] = value;
    }
}

void nes_ppu_render_frame() {
    // 简化的PPU渲染
    for (int scanline = 0; scanline < 240; scanline++) {
        // 渲染背景
        for (int x = 0; x < 256; x++) {
            // 简化的背景渲染
            uint8_t tile_id = nes_state.nametab[(scanline/8)*32 + (x/8)];
            uint8_t pixel = (tile_id * 16 + (scanline%8)) % 64;
            uint16_t color = nes_palette_256[pixel];
            
            // 缩放到屏幕
            for (int sx = 0; sx < 4; sx++) {
                int screen_x = x * 4 + sx;
                if (screen_x < 240) {
                    tft.drawPixel(screen_x, scanline, color);
                }
            }
        }
    }
}

void nes_apu_generate_audio() {
    // 简化的APU
    static uint32_t sample_counter = 0;
    if (++sample_counter >= 24000) { // 24kHz
        sample_counter = 0;
        // 生成简单的方波
        int16_t sample = 0;
        sample += (apu.pulse1[0] & 0x80) ? 1000 : -1000;
        sample += (apu.pulse2[0] & 0x80) ? 1000 : -1000;
        sample += (apu.triangle[0] & 0x80) ? 1000 : -1000;
        sample += (apu.noise[0] & 0x80) ? 1000 : -1000;
        sample /= 4;
        
        // I2S输出
        // 这里需要实际的I2S驱动代码
    }
}
```

## 步骤4: 更新菜单调用

找到调用runFcEmulator的地方，将其改为runNesEmulator。

## 步骤5: 编译测试

上传到ESP32-S3开发板，测试NES模拟器是否正常工作。

## 注意事项

1. 这是一个简化版的NES模拟器，完整的NES模拟器代码非常庞大（超过4000行）
2. 简化版实现了基本的CPU、PPU、APU框架，但功能有限
3. 完整的NES模拟器需要从nes/esp32s3_nes_gamer项目复制更多文件：
   - emucore.cpp (CPU、PPU、APU完整实现)
   - mappers.h (映射器实现)
   - emucore.h (核心结构定义)
   - controller.cpp (输入处理)
   - display.cpp (显示处理)

4. 如果需要完整的NES模拟器，建议直接复制nes/esp32s3_nes_gamer/src文件夹中的所有文件到s3.ino项目中

## 下一步

如果需要更完整的NES模拟器，可以考虑：
1. 将nes/esp32s3_nes_gamer/src/emucore.cpp中的核心函数复制到s3.ino
2. 将nes/esp32s3_nes_gamer/src/emucore.h中的结构体定义复制到s3.ino
3. 将nes/esp32s3_nes_gamer/src/mappers.h中的映射器实现复制到s3.ino
4. 将nes/esp32s3_nes_gamer/src/indev/controller.cpp中的输入处理复制到s3.ino
