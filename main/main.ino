/*
 * 项目名称：XFR1688 & UXU倒計時 校园卡灯板
 * 主程序入口：硬件初始化、主循环调度
 * 作者：XFR1688 & UXU倒計時 | 鸣谢：王超老师
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#ifdef ENABLE_OTA
#include <ESP8266HTTPUpdateServer.h>
#endif
#include "headers/config.h"
#include "headers/core.h"
#include "headers/hardware.h"
#include "headers/backend.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  turnOff();
  pinMode(KEY_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(100);
  initWiFi();
  lastActivityTime = millis();
  Serial.println("系统就绪，优化完毕，OTA可用");
}

void loop() {
  unsigned long now = millis();

  // 非低功耗模式下处理DNS劫持和Web请求
  if (!lowPowerMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  }

  // 更新板载LED闪烁状态
  updateLedBlink();

  // 按键处理：消抖、短按切换、长按调光、低功耗唤醒
  handleButton(now);

  // 低功耗模式下仅处理按键和中断
  if (lowPowerMode) {
    yield();
    return;
  }

  // 空闲超时进入低功耗（OTA升级期间禁止进入低功耗）
  if (!lowPowerMode && !otaInProgress) {
    unsigned long currentNow = millis();
    // 情况1：主动点击休眠模式，进入 30 秒倒计时
    if (currentMode == MODE_OFF && sleepCountdownActive) {
      if (currentNow - lastActivityTime >= SLEEP_COUNTDOWN_MS) {
        sleepCountdownActive = false;
        enterLowPower();
        return;
      }
    }
    // 情况2：正常活动状态下，5 分钟无操作自动休眠
    else if (!sleepCountdownActive) {
      if (currentNow - lastActivityTime >= IDLE_SLEEP_TIMEOUT_MS) {
        enterLowPower();
        return;
      }
    }
  }



  // 非休眠模式下更新效果
  if (currentMode != MODE_OFF) {
    updateEffect(now);
  }

  // 更新首次开机渐变（非阻塞，按线性插值递增亮度）
  updateFadeIn(now);

  // 根据当前模式输出LED
  switch (currentMode) {
    case MODE_OFF:
      if (sleepCountdownActive) {
        // 休眠倒计时期间：RGB灯关闭，板载LED每秒慢闪提示设备仍在线
        if (!isOffUpdated) { turnOff(); isOffUpdated = true; }
        if (now - lastSleepBlinkTime >= 1000) {
          lastSleepBlinkTime = now;
          ledBlink();
        }
      } else if (!isOffUpdated) { turnOff(); isOffUpdated = true; }
      break;
    case MODE_ON:
      setLed(255, 255, 255, brightness);
      break;
    case MODE_COLOR:
      setLed(colorLib[colorIndex][0], colorLib[colorIndex][1], colorLib[colorIndex][2], brightness);
      break;
    case MODE_FLASH: {
      int idx = (now / 500) % colorCount;
      setLed(colorLib[idx][0], colorLib[idx][1], colorLib[idx][2], brightness);
      break;
    }
    case MODE_COLORWHEEL:
      setLed(customR, customG, customB, brightness);
      break;
    default: break;
  }

  yield();
}
