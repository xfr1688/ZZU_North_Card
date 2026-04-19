// 前端页面：HTML/CSS/JS 资源声明（实际数据存放在 frontend.cpp 的 Flash 中）
#ifndef FRONTEND_H
#define FRONTEND_H

#include <Arduino.h>

// 前端HTML资源，定义在 frontend.cpp 中，仅此一份避免 Flash 重复
extern const char index_html[] PROGMEM;

#endif
