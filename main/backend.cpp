// Web服务器与路由处理函数实现
#include <Arduino.h>
#include "headers/config.h"
#ifdef ENABLE_OTA
#include <ESP8266HTTPUpdateServer.h>
#endif
#include "headers/backend.h"
#include "headers/frontend.h"
#include "headers/hardware.h"
#include "headers/core.h"

// ========== 全局服务器对象 ==========
ESP8266WebServer server(80);
DNSServer dnsServer;
#ifdef ENABLE_OTA
ESP8266HTTPUpdateServer httpUpdater;
#endif

/** 返回当前设备状态的JSON数据（手动拼接，避免ArduinoJson库占用Flash） */
void handleStatus() {
  char json[180];
  snprintf(json, sizeof(json),
    "{\"currentMode\":%d,\"brightness\":%d,\"colorIndex\":%d,\"lowPowerMode\":%s,\"customR\":%d,\"customG\":%d,\"customB\":%d,\"sleepCountdown\":%s}",
    currentMode, brightness, colorIndex,
    lowPowerMode ? "true" : "false",
    customR, customG, customB,
    sleepCountdownActive ? "true" : "false");
  server.send(200, "application/json", json);
}

/** 切换LED工作模式 */
void handleMode() {
  String m = server.arg("m");
  if (m == "0") {
    currentMode = MODE_OFF;
    sleepCountdownActive = true;
    lastActivityTime = millis();
  } else {
    resetSystemTimer();
    if (m == "1") currentMode = MODE_ON;
    else if (m == "2") currentMode = MODE_COLOR;
    else if (m == "3") currentMode = MODE_FLASH;
    else if (m == "4") currentMode = MODE_BREATH;
    else if (m == "5") currentMode = MODE_RAINBOW;
    else if (m == "6") currentMode = MODE_COLORWHEEL;
  }
  isOffUpdated = false;
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/** 切换预设颜色 */
void handleColor() {
  int c = server.arg("c").toInt();
  resetSystemTimer();
  if (c >= 0 && c < colorCount) {
    colorIndex = c;
    currentMode = MODE_COLOR;
    isOffUpdated = false;
  }
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/** 调节亮度 */
void handleBright() {
  int b = server.arg("b").toInt();
  resetSystemTimer();
  if (b >= 0 && b <= 255) {
    brightness = b;
    Serial.print("亮度更新: ");
    Serial.println(brightness);
  }
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/** 设置色轮自定义颜色（G/B交换对应硬件映射） */
void handleColorWheel() {
  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();
  resetSystemTimer();
  if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
    customR = r;
    customG = b; // 保持原有交换逻辑，对应硬件引脚映射
    customB = g;
    currentMode = MODE_COLORWHEEL;
    isOffUpdated = false;
    Serial.printf("色轮颜色: R=%d, G=%d, B=%d\n", customR, customG, customB);
  }
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/** 返回WiFi连接状态 */
void handleWiFiStatus() {
  char json[80];
  snprintf(json, sizeof(json),
    "{\"connected\":%s,\"ssid\":\"%s\",\"ip\":\"%s\"}",
    WiFi.isConnected() ? "true" : "false",
    WiFi.softAPSSID().c_str(),
    WiFi.softAPIP().toString().c_str());
  server.send(200, "application/json", json);
}

/** Captive portal重定向到AP主页 */
void handleRedirectRoot() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "Redirecting");
}

void handleGenerate204() { handleRedirectRoot(); }
void handleHotspotDetect() { handleRedirectRoot(); }
void handleNcsi() { handleRedirectRoot(); }
void handleConnectTest() { handleRedirectRoot(); }
void handleRedirectFallback() { handleRedirectRoot(); }

/** 返回前端页面 */
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

/** OTA升级开始处理：设置标志防止进入低功耗模式 */
#ifdef ENABLE_OTA
void handleOTAStart() {
  otaInProgress = true;
  Serial.println("OTA升级开始，禁止进入低功耗模式");
  server.send(200, "text/plain", "OK");
}
#endif

/** 配置OTA固件升级端点（需启用ENABLE_OTA） */
#ifdef ENABLE_OTA
void setupOTA() {
  httpUpdater.setup(&server, "/update", "", "");
}
#endif

/** 初始化WiFi AP、DNS劫持、Web路由、OTA（若启用） */
void initWiFi() {
  WiFi.mode(WIFI_AP);
  delay(100);
  String mac = WiFi.macAddress();
  String suffix = "0000";
  if (mac.length() >= 5 && mac != "00:00:00:00:00:00") {
    suffix = mac.substring(mac.length()-5, mac.length()-1);
    suffix.replace(":", "");
  }
  String ssid = String(ap_ssid) + "_" + suffix;
  WiFi.softAP(ssid.c_str(), ap_password);

  // DNS劫持：将所有域名解析到设备IP，触发Captive Portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Serial.print("AP: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // 业务路由
  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/color", handleColor);
  server.on("/bright", handleBright);
  server.on("/status", handleStatus);
  server.on("/colorwheel", handleColorWheel);
  server.on("/wifistatus", handleWiFiStatus);
#ifdef ENABLE_OTA
  server.on("/otastart", handleOTAStart);
#endif

  // Captive Portal检测路由：各平台自动检测URL均重定向到主页
  server.on("/generate_204", handleGenerate204);         // Android
  server.on("/hotspot-detect.html", handleHotspotDetect); // iOS
  server.on("/connecttest.txt", handleNcsi);             // Windows
  server.on("/ncsi.txt", handleConnectTest);             // Windows NCSI

  // 兜底：未匹配的路径重定向到主页，确保DNS劫持后的HTTP请求也能跳转
  server.onNotFound(handleRedirectRoot);

#ifdef ENABLE_OTA
  setupOTA();
#endif
  server.begin();
}
