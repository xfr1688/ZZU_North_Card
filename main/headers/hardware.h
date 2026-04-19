// 硬件控制函数声明：LED指示、按键处理、低功耗管理
#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include "config.h"
#include "core.h"

/** 板载LED短闪提示 */
void ledBlink();

/** 更新板载LED闪烁状态（非阻塞） */
void updateLedBlink();

/** 按键中断回调（ICACHE_RAM_ATTR） */
void ICACHE_RAM_ATTR keyISR();

/** 进入低功耗模式：关WiFi、降频、挂起等中断唤醒 */
void enterLowPower();

/** 退出低功耗模式：恢复WiFi、升频、重新初始化 */
void exitLowPower();

/**
 * 按键处理主逻辑：消抖、短按切换模式、长按无级调光
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void handleButton(unsigned long now);

#endif
