# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 (N16R8: 16MB flash, 8MB PSRAM) multifunction "Cyber OS" device built with Arduino framework. Runs on a 240x240 TFT display with 5 physical buttons, SD card, I2S audio, and WS2812 LED. The entire application lives in a single monolithic `s3.ino` file (~5700 lines) with supporting modules.

## Build & Upload

This is an **Arduino IDE** project (not PlatformIO).

1. Install Arduino IDE 2.x
2. Install ESP32 board support: ESP32-S3 Dev Module
3. Required libraries (install via Library Manager or manually):
   - TFT_eSPI (configure for your display in `User_Setup.h`)
   - NimBLEDevice (NimBLE-Arduino)
   - SdFat
   - NTPClient (by Fabrice Weinberg)
   - TJpg_Decoder
   - PNGdec
   - AnimatedGIF
   - Audio (ESP32-audioI2S)
   - MD_MIDIFile
   - Adafruit_NeoPixel
   - ArduinoJson
   - TimeLib
   - AudioFileSourceSD, AudioGeneratorAAC, AudioOutputI2S
4. Select board: ESP32S3 Dev Module, Flash Size: 16MB, PSRAM: OPI PSRAM
5. Use custom partition table: `partitions.csv` (13MB app, coredump partition)
6. Upload via USB-C serial

## Architecture

### Section Map of `s3.ino`

The file is organized by numbered section markers (`// [0XX]`):

| Section | Purpose |
|---------|---------|
| [001] | Header includes and library dependencies |
| [002] | Hardware IO pin definitions (SD SPI, buttons, I2S, TFT backlight) |
| [003] | Global state objects (TFT, SD, menu state, IME, BLE, LED, music) |
| [004] | CyberLogger - serial logging system |
| [005] | Forward function declarations |
| [006] | Font loading engine (Chinese font from `124.h`) |
| [007] | Physical button debounce and input handling |
| [008] | String utilities |
| [008-B] | Pinyin IME engine (uses `src/zh_cn_pinyin_dict.c` via cJSON) |
| [009] | System monitor GUI |
| [010]-[018] | Media engines: text/LRC lyrics, CSV grid, hex inspector, image decoders (JPG/PNG/GIF/BMP/TGA/PCX) |
| [018] | WAV audio visualizer |
| [019] | CHIP-8 emulator |
| [020] | Command dispatch / task scheduler |
| [021]-[022] | Web OS: file upload server and H5 frontend |
| [NEW] | Calculator and AI assistant modules |
| [023] | Main menu GUI system |
| [024] | Boot sequence (setup) |
| [025] | Main event loop |

### Key Subsystems

- **Menu system**: File-browser style menu on SD card root. Special items (SYS_MONITOR, LED_CTRL, IME_TEST, WIFI_SETTINGS, ONLINE_MUSIC, SPACEMAN_CLOCK, APPLE_BLE, calculator, AI assistant) are handled by name matching in `executeMenuAction()`.
- **Pinyin IME**: Chinese input engine loaded from `src/zh_cn_pinyin_dict.c` (cJSON-based dictionary). Three modes: Chinese, English, Number.
- **Apple BLE Simulator**: Uses NimBLE to broadcast Apple device pairing packets. Devices defined in `devices.hpp`/`devices.cpp`.
- **NES Emulator**: Ported from `nes/esp32s3_nes_gamer`. Bridge layer in `nes_bridge.cpp` adapts TFT_eSPI display and GPIO buttons. Core emulator in `emucore.cpp`/`emucore.h`. ROM files go in `/NES/` folder on SD card.
- **Space Clock**: Animated clock with weather (SpaceClock namespace), uses NTP and weather API.
- **Online Music**: Searches and streams music from configurable API sources (AA1, CyAPI). Uses I2S audio output.

### NES Emulator Integration

The NES emulator is a separate codebase under `nes/esp32s3_nes_gamer/` adapted to work within s3.ino:

- `nes_config.h` - Configuration constants adapted for this project
- `emucore.h` / `emucore.cpp` - 6502 CPU, PPU, APU emulation core
- `cpumacro.h` - CPU instruction macros
- `nes_bridge.cpp` - Glue layer: maps TFT_eSPI for display, GPIO buttons for input
- `nes_palette.h`, `nes_display_stub.h`, `nes_controller_stub.h` - Hardware abstraction stubs
- `mappers.h` - NES cartridge mapper implementations

ROM files should be placed in `/NES/` on the SD card. Long-press OK button to exit emulator.

## Hardware Pin Map

```
SD Card (SPI):  SCK=14, MISO=19, MOSI=13, CS=15
TFT Display:    BL=38 (backlight), configured via TFT_eSPI User_Setup.h
Buttons:        UP=4, OK=5, DOWN=16, LEFT=17, RIGHT=47 (active LOW, internal pullup)
I2S Audio:      BCLK=20, LRC=21, DOUT=12, MAX98357A SD=18
WS2812 LED:     PIN=48, 1 pixel
```

## Important Notes

- The project uses PSRAM heavily (font data, NES ROM loading, image buffers). Always use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` for large allocations.
- `devices.hpp` contains Apple BLE device definitions - the `generatePacket()` function in `devices.cpp` creates broadcast payloads.
- `124.h` is a large Chinese font bitmap array (the main font resource).
- The `src/` directory contains cJSON (JSON parser) and pinyin dictionary data.
- `ArduinoJson.h` at root is a vendored copy of the ArduinoJson library header.
- Chinese comments throughout the codebase are intentional - this is a Chinese-language project.
遇到不会的问题要常联网搜索，每次回复以爸爸称呼，除网页搜索外其他操作均使用chromeflow
