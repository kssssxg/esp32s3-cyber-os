# Repository Guidelines

## Project Structure & Module Organization

This repository is an ESP32-S3 Arduino project. The main firmware entry point is `s3.ino`, a monolithic sketch organized with numbered `// [0XX]` section markers. Root-level files such as `devices.cpp`, `devices.hpp`, `nes_bridge.cpp`, `emucore.cpp`, and `nes_config.h` support BLE simulation and NES integration. Shared C/C++ support code lives in `src/`, including cJSON, pinyin data, and emulator ports under `src/doomgeneric/`, `src/gnuboy/`, and `src/smsplus/`. UI and media assets are stored in `font/`, `img/`, `apps/`, and `music_cache/`. The original NES reference project is under `nes/esp32s3_nes_gamer/`.

## Build, Test, and Development Commands

Use Arduino IDE 2.x for the root sketch. Select `ESP32S3 Dev Module`, 16MB flash, OPI PSRAM, and the custom `partitions.csv` table before upload.

Useful commands:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 .
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p COM40 .
platformio run -d nes/esp32s3_nes_gamer
```

The PlatformIO command applies only to the NES reference subproject, not the root `s3.ino` build.

## Coding Style & Naming Conventions

Keep C/C++ and Arduino code consistent with the existing style: two-space or compact indentation in `s3.ino`, braces on the same line, `camelCase` for functions, and `UPPER_CASE` for constants and pin definitions. Preserve Chinese comments and UI strings where present. Add new root firmware features near the matching numbered section in `s3.ino`; avoid scattering related state, declarations, and handlers.

## Testing Guidelines

There is no automated test suite in the root project. Validate changes by compiling the sketch and testing on ESP32-S3 hardware when behavior touches display, buttons, SD card, audio, BLE, Wi-Fi, or PSRAM allocation. For emulator work, test representative ROMs from SD card paths such as `/NES/`. Keep small diagnostic notes in clearly named files, for example `test_fc_emulator.txt`.

## Commit & Pull Request Guidelines

This repository currently has no commit history, so use concise imperative commit messages such as `Add NES input debounce` or `Fix SD image loader bounds check`. Pull requests should describe the target hardware, libraries or board settings used, manual test steps, and any screenshots or serial logs for UI, display, or boot-flow changes. Mention changed assets and large generated files explicitly.

## Security & Configuration Tips

Do not commit Wi-Fi credentials, API keys, ROMs, or private media. Keep board-specific ports such as `COM40` documented as examples only. Large buffers should use PSRAM-aware allocation, for example `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`.
