// 硬件控制函数实现：LED指示、按键处理、低功耗管理
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "headers/hardware.h"
#include "headers/backend.h"

/** 板载LED短闪提示 */
void ledBlink() {
  digitalWrite(LED_BUILTIN, LOW);
  ledOn = true;
  ledOffTime = millis() + 100;
}

/** 更新板载LED闪烁状态（非阻塞） */
void updateLedBlink() {
  if (ledOn && millis() >= ledOffTime) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOn = false;
  }
}

/** 按键中断回调，设置唤醒标志 */
void ICACHE_RAM_ATTR keyISR() { wakeUpRequest = true; }

/** 退出低功耗模式：恢复WiFi、升频、重新初始化 */
void exitLowPower() {
  Serial.println("退出低功耗");
  detachInterrupt(digitalPinToInterrupt(KEY_PIN));
  system_update_cpu_freq(160);
  WiFi.forceSleepWake();
  delay(100);
  initWiFi();
  wakeUpRequest = false;
  lowPowerMode = false;
  // 唤醒后重置活动时间，防止立即再次进入休眠，并进入 5 分钟正常空闲计时
  resetSystemTimer();


  // 唤醒后自动进入第一个灯光模式
  currentMode = MODE_ON;
  isOffUpdated = false;
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, LOW); delay(50);
    digitalWrite(LED_BUILTIN, HIGH); delay(50);
  }
}

/** 进入低功耗模式：关WiFi、降频、挂起等中断唤醒 */
void enterLowPower() {
  Serial.println("进入低功耗");
  lowPowerMode = true;
  turnOff();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  // 确保先清除可能的旧标志
  wakeUpRequest = false;
  attachInterrupt(digitalPinToInterrupt(KEY_PIN), keyISR, FALLING);
  system_update_cpu_freq(80);
  // 在循环中等待中断触发 wakeUpRequest = true
  while (!wakeUpRequest) { 
    // 使用 light sleep 节省功耗 (在 delay 时 ESP8266 会尝试自动进入 modem sleep/light sleep)
    delay(100); 
  }
  exitLowPower();
}

/**
 * 按键处理主逻辑：消抖、短按切换模式、长按无级调光
 * @param {unsigned long} now - 当前毫秒时间戳
 */
void handleButton(unsigned long now) {
  int key = digitalRead(KEY_PIN);

  // 低功耗模式下的按键检测与唤醒
  if (lowPowerMode) {
    if (key == LOW) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(KEY_PIN) == LOW) {
        while (digitalRead(KEY_PIN) == LOW) { delay(10); }
        if (lowPowerMode) exitLowPower();
      }
    }
    return;
  }

  // 按键按下：消抖后记录起始时间
  if (key == LOW && !isPressing) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(KEY_PIN) == LOW) {
      isPressing = true;
      pressStartTime = now;
      resetSystemTimer();
    }
  }

  // 按键释放：短按切换模式，长按结束打印亮度
  if (key == HIGH && isPressing) {
    isPressing = false;
    resetSystemTimer();
    unsigned long dur = now - pressStartTime;
    if (dur < SHORT_PRESS_TIME) {


      if (initialFadeInProgress) {
        // 渐变进行中再次短按：立即完成渐变
        brightness = FADE_TARGET_BRIGHT;
        initialFadeInProgress = false;
        initialFadeDone = true;
      } else if (!initialFadeDone && currentMode == MODE_OFF) {
        // 首次短按且当前为关闭模式：启动亮度渐变而非切换模式
        currentMode = MODE_ON;
        brightness = 0;
        initialFadeInProgress = true;
        fadeStartTime = now;
        isOffUpdated = false;
        sleepCountdownActive = false;
        lastActivityTime = now;
      } else {
        // 正常模式切换
        if (!initialFadeDone) initialFadeDone = true;
        switchMode();
      }
    } else {
      // 长按结束：若正在渐变则取消渐变
      if (initialFadeInProgress) {
        initialFadeInProgress = false;
        initialFadeDone = true;
      }
      Serial.print("长按结束，亮度: "); Serial.println(brightness);
    }
    lastActivityTime = now;
  }

  // 长按持续调光：渐变期间取消渐变并交由调光接管
  if (isPressing && (now - pressStartTime) >= LONG_PRESS_TIME) {
    if (initialFadeInProgress) {
      initialFadeInProgress = false;
      initialFadeDone = true;
    }
    if (currentMode != MODE_OFF && (now - lastDimmingTime) >= DIMMING_STEP_TIME) {
      lastDimmingTime = now;
      if (brightnessUp) {
        brightness++;
        if (brightness >= 255) { brightness = 255; brightnessUp = false; }
      } else {
        brightness--;
        if (brightness <= 0) { brightness = 0; brightnessUp = true; }
      }
    }
  }
}
