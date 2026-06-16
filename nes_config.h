#ifndef __NES_CONFIG_H
#define __NES_CONFIG_H
#include <stdint.h>

// ========================================================
// NES模拟器配置 - 适配s3.ino项目
// 直接使用nes文件夹里的现成代码，只做最小化适配
// ========================================================

// 屏幕分辨率 (与TFT_eSPI配置一致)
#define SCREEN_RES_HOR 240
#define SCREEN_RES_VER 240

// 使用TFT_eSPI库 (s3.ino已有的)
#define USE_TFT_ESPI

// 显示配置
#define TFT_IS_IPS true
#define TFT_ROTATION 0

// SD卡使用SPI模式 (s3.ino已有的)
#define USE_SPI_SD true

// 输入设备配置 (只用GPIO按键)
#define GPIO_PAD_ENABLED

// 音频配置
#define SOUND_ENABLED true

// 消息输出宏 (emucore.cpp中不能用Serial)
// 只有在明确启用时才使用Serial，避免emucore.cpp编译错误
#ifdef NES_USE_SERIAL
#define SHOW_MSG_SERIAL(x) Serial.print(x);
#define SHOW_MSG_BOTH(x) Serial.print(x);
#else
#define SHOW_MSG_SERIAL(x)
#define SHOW_MSG_BOTH(x)
#endif
#define STAY_HERE while (1){};

// UI颜色配置
#define UI_TEXT_COLOR 0xff9e
#define UI_TEXT_SELECTED_COLOR 0xffff
#define UI_STATUSBAR_CURSOR_X 10
#define UI_STATUSBAR_CURSOR_Y 228
#define UI_GAME_LIST_COLOR 0xc618
#define UI_GAME_LIST_SELECTED_COLOR 0xffff
#define UI_CONTROL_DEBOUNCE_MS1  200
#define UI_CONTROL_DEBOUNCE_MS2  400

// 文件系统配置
#define MAXFILES 255
#define MAXFILENAME_LENGTH 62
#define FILES_PER_PAGE 10
#define ROM_READING_BUFFER_SIZE 4096

// NES模拟器核心定义 (直接从config.h复制)
#define NES_FOLDER ("/NES/")

#define HOST_LITTLE_ENDIAN
#define ZERO_LENGTH 0
#define UNUSED_NES(x) ((x) = (x))

#define NTSC
#ifdef PAL
#define NES_REFRESH_RATE 50
#else
#define NES_REFRESH_RATE 60
#endif

#define NES_VISIBLE_HEIGHT 240
#define NES_SCREEN_WIDTH 256
#define NES_SCREEN_HEIGHT 240

#define DEFAULT_SAMPLERATE 24000
#define DEFAULT_FRAGSIZE 200
#define MAX_MEM_HANDLERS 32

#define NES_CLOCK_DIVIDER 12
#define NES_MASTER_CLOCK (236250000 / 11)
#define NES_SCANLINE_CYCLES (1364.0 / NES_CLOCK_DIVIDER)
#define NES_FIQ_PERIOD (NES_MASTER_CLOCK / NES_CLOCK_DIVIDER / 60)

#define NES_RAMSIZE 0x800
#define NES_SKIP_LIMIT (NES_REFRESH_RATE / 5)

#define NES6502_NUMBANKS 16
#define NES6502_BANKSHIFT 12
#define NES6502_BANKSIZE (0x10000 / NES6502_NUMBANKS)
#define NES6502_BANKMASK (NES6502_BANKSIZE - 1)

// 枚举和结构体 (从config.h中提取)
typedef enum {
    VID_NONE = 0x0000,
    VID_DRAW_NES_FRAME = 0x0001
} vid_notify_command_t;

typedef enum {
    CONTROLLER_NONE = 0,
    CONTROLLER_GPIO_PAD
} controller_enum;

typedef struct {
    uint16_t file_size;
    char file_name[MAXFILENAME_LENGTH];
} FILE_OBJ;

typedef struct {
    uint8_t currMode;
    uint8_t nesEnterFlag : 1;
    uint8_t sdOK : 1;
    uint8_t romFilenameCached : 1;
    uint8_t nesPower : 1;
    uint8_t mute : 1;
    uint8_t loadedRomFileNames;
    uint8_t menuCursor;
    uint8_t menuPage;
    uint32_t romMaxSize;
} RUNNING_CFG;

#endif
