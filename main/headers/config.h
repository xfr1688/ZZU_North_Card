// 项目全局配置：引脚定义、模式常量、WiFi参数、颜色库
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========== 引脚定义 ==========
#define KEY_PIN 4
#define LED_R 5
#define LED_G 12
#define LED_B 13
#define LED_BUILTIN 2

// ========== 系统模式 ==========
#define MODE_OFF 0
#define MODE_ON 1
#define MODE_COLOR 2
#define MODE_FLASH 3
#define MODE_BREATH 4
#define MODE_RAINBOW 5
#define MODE_COLORWHEEL 6

// ========== 低功耗参数 ==========
#define SLEEP_COUNTDOWN_MS 30000
#define IDLE_SLEEP_TIMEOUT_MS 300000
#define DEBOUNCE_DELAY 20
#define SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 500
#define DIMMING_STEP_TIME 10

// ========== 首次渐变参数 ==========
#define FADE_DURATION_MS 3000
#define FADE_TARGET_BRIGHT 13

// ========== WiFi配置 ==========
#define ENABLE_OTA
const char* const ap_ssid = "CampusLight";
const char* const ap_password = "12345678";
constexpr uint8_t DNS_PORT = 53;

// ========== 颜色库 ==========
const int colorLib[7][3] = {
  {255, 0, 0},   // 0: 红
  {0, 255, 0},   // 1: 绿
  {0, 0, 255},   // 2: 蓝
  {255, 255, 0}, // 3: 黄
  {255, 0, 255}, // 4: 紫
  {0, 255, 255}, // 5: 青
  {255, 255, 255}// 6: 白
};
const int colorCount = 7;

#endif
