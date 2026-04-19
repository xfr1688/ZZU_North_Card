// 核心状态变量与效果函数声明
#ifndef CORE_H
#define CORE_H

#include <Arduino.h>
#include "config.h"

// ========== 全局状态变量 ==========
extern int currentMode;
extern int brightness;
extern int colorIndex;
extern int customR, customG, customB;
extern bool brightnessUp;
extern bool isPressing;
extern unsigned long pressStartTime;
extern unsigned long lastDimmingTime;
extern unsigned long lastActivityTime;
extern bool lowPowerMode;
extern bool wakeUpRequest;
extern unsigned long ledOffTime;
extern bool ledOn;
extern unsigned long lastEffectUpdate;
extern int hue;
extern int breathPhase;
extern int breathColorIndex;
extern unsigned int breathBrightRaw;
// 呼吸灯亮度查表：256点预计算sin曲线，替代运行时sin()运算
// 表中值为 (sin(x)+1)/2 * 255，结果为0-255的亮度值
extern const unsigned char breathLUT[256];
extern bool isOffUpdated;
extern bool otaInProgress;
extern bool sleepCountdownActive;
extern unsigned long lastSleepBlinkTime;
extern bool initialFadeDone;
extern bool initialFadeInProgress;
extern unsigned long fadeStartTime;

// ========== 核心函数 ==========

/**
 * HSV 转 RGB 颜色空间（整数优化版）
 * @param {int} h - 色相 [0, 359]
 * @param {int} s - 饱和度 [0, 255]
 * @param {int} v - 明度 [0, 255]
 * @param {int&} r - 输出红色分量
 * @param {int&} g - 输出绿色分量
 * @param {int&} b - 输出蓝色分量
 */
void hsvToRgb(int h, int s, int v, int &r, int &g, int &b);

/**
 * 设置LED颜色与亮度
 * @param {int} r - 红色 [0, 255]
 * @param {int} g - 绿色 [0, 255]
 * @param {int} b - 蓝色 [0, 255]
 * @param {int} bright - 亮度 [0, 255]
 */
void setLed(int r, int g, int b, int bright);

/** 关闭所有LED输出 */
void turnOff();

/** 切换到下一个模式，色谱模式下同时切换颜色 */
void switchMode();

/**
 * 更新动态效果（呼吸、彩虹模式）
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void updateEffect(unsigned long now);

/**
 * 更新首次开机渐变效果（从0%平滑过渡到10%，3秒过渡）
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void updateFadeIn(unsigned long now);

/**
 * 重置系统空闲计时器，中断休眠倒计时
 * 任何用户交互（Web/物理按键）都应触发此函数
 */
void resetSystemTimer();

#endif
