// Web服务器与路由处理函数声明
#ifndef BACKEND_H
#define BACKEND_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#ifdef ENABLE_OTA
#include <ESP8266HTTPUpdateServer.h>
#endif
#include "config.h"
#include "core.h"

// ========== 全局服务器对象 ==========
extern ESP8266WebServer server;
extern DNSServer dnsServer;

// ========== 路由处理函数 ==========

/** 返回当前设备状态的JSON数据 */
void handleStatus();

/** 切换LED工作模式 */
void handleMode();

/** 切换预设颜色 */
void handleColor();

/** 调节亮度 */
void handleBright();

/** 设置色轮自定义颜色 */
void handleColorWheel();

/** 返回WiFi连接状态 */
void handleWiFiStatus();

/** 返回前端页面 */
void handleRoot();

/** Captive portal重定向到AP主页 */
void handleRedirectRoot();
void handleGenerate204();
void handleHotspotDetect();
void handleNcsi();
void handleConnectTest();
void handleRedirectFallback();

// ========== 初始化函数 ==========

/** 初始化WiFi AP、Web路由、OTA（若启用） */
void initWiFi();

/** 配置OTA固件升级端点（需启用ENABLE_OTA） */
#ifdef ENABLE_OTA
void setupOTA();
#endif

#endif
