// 核心状态变量定义与效果函数实现
#include <Arduino.h>
#include "headers/core.h"

// ========== 全局状态变量 ==========
int currentMode = MODE_OFF;
int brightness = 13;
int colorIndex = 0;
int customR = 255, customG = 255, customB = 255;
bool brightnessUp = true;
bool isPressing = false;
unsigned long pressStartTime = 0;
unsigned long lastDimmingTime = 0;
unsigned long lastActivityTime = 0;
bool lowPowerMode = false;
bool wakeUpRequest = false;
unsigned long ledOffTime = 0;
bool ledOn = false;
unsigned long lastEffectUpdate = 0;
int hue = 0;
int breathPhase = 0;
int breathColorIndex = 0;
unsigned int breathBrightRaw = 0;
bool isOffUpdated = false;
bool otaInProgress = false;
bool sleepCountdownActive = false;
unsigned long lastSleepBlinkTime = 0;
bool initialFadeDone = false;
bool initialFadeInProgress = false;
unsigned long fadeStartTime = 0;

// 呼吸灯亮度查表：256点预计算sin曲线
// 表中值为 (sin(i/256.0 * 2PI - PI/2) + 1) / 2 * 255
// 使得起始点(index 0)为最暗值
const uint8_t breathLUT[256] PROGMEM = {
  0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9,
  10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
  37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
  79, 82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124,
  127, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173,
  176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 239, 241, 243, 244, 246,
  247, 248, 249, 251, 252, 253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 253, 253, 252, 251, 249,
  248, 247, 246, 244, 243, 241, 239, 238, 236, 234, 232, 230, 228, 226, 224, 222,
  220, 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182,
  179, 176, 173, 170, 167, 164, 161, 158, 155, 152, 149, 146, 143, 140, 137, 134,
  131, 127, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 94, 91, 88, 85,
  82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42,
  40, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12,
  11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0
};

/**
 * HSV 转 RGB 颜色空间（整数优化版）
 * @param {int} h - 色相 [0, 359]
 * @param {int} s - 饱和度 [0, 255]
 * @param {int} v - 明度 [0, 255]
 * @param {int&} r - 输出红色分量
 * @param {int&} g - 输出绿色分量
 * @param {int&} b - 输出蓝色分量
 */
void hsvToRgb(int h, int s, int v, int &r, int &g, int &b) {
  if (s == 0) { r = g = b = v; return; }
  h = h % 360;
  if (h < 0) h += 360;
  int hi = (h / 60) % 6;
  int f = (h % 60) * 255 / 60;
  int p = (int)v * (255 - s) / 255;
  int q = (int)v * (255 - (int)s * f / 255) / 255;
  int t = (int)v * (255 - (int)s * (255 - f) / 255) / 255;
  switch (hi) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
}

/**
 * 设置LED颜色与亮度
 * @param {int} r - 红色 [0, 255]
 * @param {int} g - 绿色 [0, 255]
 * @param {int} b - 蓝色 [0, 255]
 * @param {int} bright - 亮度 [0, 255]
 */
void setLed(int r, int g, int b, int bright) {
  int scaledR = r * bright / 255;
  int scaledG = g * bright / 255;
  int scaledB = b * bright / 255;
  analogWrite(LED_R, scaledR);
  analogWrite(LED_G, scaledG);
  analogWrite(LED_B, scaledB);
}

/** 关闭所有LED输出 */
void turnOff() { setLed(0, 0, 0, 0); }

/** 切换到下一个模式，色谱模式下同时切换颜色 */
void switchMode() {
  currentMode++;
  if (currentMode > MODE_COLORWHEEL) currentMode = MODE_OFF;
  if (currentMode == MODE_COLOR) {
    colorIndex++;
    if (colorIndex >= colorCount) colorIndex = 0;
  }
  // 进入MODE_OFF时启动休眠倒计时，切换到其他模式时取消倒计时
  sleepCountdownActive = (currentMode == MODE_OFF);
  isOffUpdated = false;
  lastActivityTime = millis();
  // 重置板载LED闪烁时间戳，确保倒计时开始后能立即触发第一次闪烁
  lastSleepBlinkTime = lastActivityTime;
  Serial.print("模式: ");
  Serial.println(currentMode);
}

/**
 * 更新动态效果（呼吸、彩虹模式）
 * 呼吸灯使用预计算查表替代sin()运算，大幅减少浮点CPU消耗
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void updateEffect(unsigned long now) {
  switch (currentMode) {
    case MODE_BREATH:
      // 呼吸周期1.5s，共256步，每步间隔约5.86ms，取6ms
      if (now - lastEffectUpdate >= 6) {
        lastEffectUpdate = now;
        
        // 亮度查表索引递增(0-255循环)
        breathPhase++;
        if (breathPhase >= 256) {
          breathPhase = 0;
          // 完整呼吸周期结束后切换到下一个颜色
          breathColorIndex++;
          // 循环回到第一个颜色，排除白色（索引6）
          if (breathColorIndex >= 6) breathColorIndex = 0;
        }
        
        // 查表获取当前亮度(0-255)，映射到0-70范围
        int rawBright = (int)pgm_read_byte(&breathLUT[breathPhase]);
        int breathBright = rawBright * 70 / 255;
        
        setLed(colorLib[breathColorIndex][0], colorLib[breathColorIndex][1], colorLib[breathColorIndex][2], breathBright);
      }
      break;
    case MODE_RAINBOW:
      // 彩虹模式更新频率20ms，每次色相递增2度
      if (now - lastEffectUpdate >= 20) {
        lastEffectUpdate = now;
        hue += 2;
        if (hue >= 360) hue -= 360;
        int r, g, b;
        // 使用最大明度255进行HSV转换，获取纯色分量，再由setLed进行亮度缩放
        hsvToRgb(hue, 255, 255, r, g, b);
        setLed(r, g, b, brightness);
      }
      break;
    default: break;
  }
}

/**
 * 更新首次开机渐变效果（从0%平滑过渡到10%，3秒过渡）
 * 渐变期间亮度按线性插值递增，完成后恢复正常模式切换
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void updateFadeIn(unsigned long now) {
  if (!initialFadeInProgress) return;

  unsigned long elapsed = now - fadeStartTime;
  if (elapsed >= FADE_DURATION_MS) {
    // 渐变完成，设置目标亮度并标记完成
    brightness = FADE_TARGET_BRIGHT;
    initialFadeInProgress = false;
    initialFadeDone = true;
  } else {
  // 线性插值计算当前亮度
    brightness = (int)(elapsed * FADE_TARGET_BRIGHT / FADE_DURATION_MS);
  }
}

/**
 * 重置系统空闲计时器，中断休眠倒计时
 */
void resetSystemTimer() {
  lastActivityTime = millis();
  // 如果当前处于休眠倒计时期间，则中断倒计时并恢复正常状态
  if (sleepCountdownActive) {
    sleepCountdownActive = false;
    // 如果模式是 MODE_OFF (0)，为了让设备保持在线并可操作，可能需要切换到 MODE_ON 或保持现状但重置 5 分钟定时
    // 按照需求，“将设备状态从'休眠倒计时期间'转换回正常的活动状态”
    // 这里我们保持当前模式，但关闭 sleepCountdownActive 标志，由 loop 处理 5 分钟逻辑
  }
}
