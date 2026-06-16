#pragma once
// NES控制器适配桩 - 为emucore.cpp提供必要的输入声明
// 不要包含Arduino.h，emucore.cpp是纯C风格代码

#include <stdint.h>

// 手柄按键结构体 (直接从controller.h复制)
typedef struct {
    union {
        struct {
            uint8_t JOY_A : 1;
            uint8_t JOY_B : 1;
            uint8_t JOY_SELECT : 1;
            uint8_t JOY_START : 1;
            uint8_t JOY_UP : 1;
            uint8_t JOY_DOWN : 1;
            uint8_t JOY_LEFT : 1;
            uint8_t JOY_RIGHT : 1;
        };
        uint8_t KEY_VALUE;
    };
} nes_pad_key_s;

// 两个手柄的全局状态
extern volatile nes_pad_key_s gamepad_p1;
extern volatile nes_pad_key_s gamepad_p2;

// 日志函数 - emucore.cpp不能直接用Serial，通过此桩函数输出
#ifdef __cplusplus
extern "C" {
#endif
void nes_log(const char *msg);
void nes_printf(const char *fmt, ...);
#ifdef __cplusplus
}
#endif
