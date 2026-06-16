#pragma once
// NES显示适配桩 - 为emucore.cpp提供必要的显示声明
// 不要包含Arduino.h，emucore.cpp是纯C风格代码

#include <stdint.h>

// SCREENMEMORY: NES帧缓冲 (256x240 像素, 8bit索引色)
extern uint8_t *SCREENMEMORY;

// 刷屏任务句柄 (s3.ino不使用FreeRTOS刷屏任务，保留声明即可)
extern void *TASK_VID_HANDLE;
