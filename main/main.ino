#include <dummy.h>

/*
 * 项目名称：XFR1688 校园卡灯板（性能优化版）
 * 功能：物理按键、网页控制、色轮拾色、亮度滑块、OTA升级
 * 硬件：ESP8266 + 4颗共阳RGB灯珠(AO3400驱动) + 轻触开关
 * 作者：XFR1688 | 鸣谢：王超老师
 * 优化说明：
 *   - 主循环统一获取 millis()，减少重复调用
 *   - 添加 yield() 保障 WiFi 后台任务
 *   - 缩短低功耗唤醒检测阻塞时间
 *   - JSON 序列化文档复用
 *   - 色轮中心饱和度下限 0.25 避免过白
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>

// ========== 引脚定义 ==========
#define KEY_PIN 4
#define LED_R   5
#define LED_G   12
#define LED_B   13
#define LED_BUILTIN 2

// ========== 系统模式 ==========
#define MODE_OFF        0
#define MODE_ON         1
#define MODE_COLOR      2
#define MODE_FLASH      3
#define MODE_BREATH     4
#define MODE_RAINBOW    5
#define MODE_COLORWHEEL 6

// ========== 低功耗参数 ==========
#define IDLE_TIME_BEFORE_SLEEP  30000
#define DEBOUNCE_DELAY          20
#define SHORT_PRESS_TIME        500
#define LONG_PRESS_TIME         500
#define DIMMING_STEP_TIME       10

// ========== WiFi配置 ==========
const char* ap_ssid = "CampusLight";
const char* ap_password = "12345678";
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

// ========== 全局变量 ==========
int currentMode = MODE_OFF;
int brightness = 128;
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
float hue = 0;
int breathPhase = 0;

// 颜色库（后端索引0-6不变）
const int colorLib[7][3] = {
  {255, 0, 0},    // 0: 红
  {0, 255, 0},    // 1: 绿
  {0, 0, 255},    // 2: 蓝
  {255, 255, 0},  // 3: 黄
  {255, 0, 255},  // 4: 紫
  {0, 255, 255},  // 5: 青
  {255, 255, 255} // 6: 白
};
const int colorCount = 7;

/*
 * 将HSV颜色空间转换为RGB
 * @param {float} h - 色相，范围0~360
 * @param {float} s - 饱和度，范围0~1
 * @param {float} v - 明度，范围0~1
 * @param {int} &r - 输出红色分量
 * @param {int} &g - 输出绿色分量
 * @param {int} &b - 输出蓝色分量
 */
void hsvToRgb(float h, float s, float v, int &r, int &g, int &b) {
  h = fmod(h, 360);
  s = constrain(s, 0, 1);
  v = constrain(v, 0, 1);
  int hi = (int)(h / 60) % 6;
  float f = h / 60 - hi;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);
  switch (hi) {
    case 0: r = v * 255; g = t * 255; b = p * 255; break;
    case 1: r = q * 255; g = v * 255; b = p * 255; break;
    case 2: r = p * 255; g = v * 255; b = t * 255; break;
    case 3: r = p * 255; g = q * 255; b = v * 255; break;
    case 4: r = t * 255; g = p * 255; b = v * 255; break;
    default: r = v * 255; g = p * 255; b = q * 255; break;
  }
}

/*
 * 设置LED颜色与亮度
 * @param {int} r - 红色分量原始值（0~255）
 * @param {int} g - 绿色分量原始值（0~255）
 * @param {int} b - 蓝色分量原始值（0~255）
 * @param {int} bright - 整体亮度（0~255）
 */
void setLed(int r, int g, int b, int bright) {
  int scaledR = r * bright / 255;
  int scaledG = g * bright / 255;
  int scaledB = b * bright / 255;
  analogWrite(LED_R, scaledR);
  analogWrite(LED_G, scaledG);
  analogWrite(LED_B, scaledB);
}

void turnOff() { setLed(0, 0, 0, 0); }

/*
 * 控制板载LED闪烁一次（用于操作反馈）
 */
void ledBlink() {
  digitalWrite(LED_BUILTIN, LOW);
  ledOn = true;
  ledOffTime = millis() + 100;
}

/*
 * 更新板载LED闪烁状态，到达关闭时间后熄灭
 */
void updateLedBlink() {
  if (ledOn && millis() >= ledOffTime) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOn = false;
  }
}

/*
 * 切换系统模式，循环遍历所有可用模式
 */
void switchMode() {
  currentMode++;
  if (currentMode > MODE_COLORWHEEL) currentMode = MODE_OFF;
  if (currentMode == MODE_COLOR) {
    colorIndex++;
    if (colorIndex >= colorCount) colorIndex = 0;
  }
  lastActivityTime = millis();
  Serial.print("模式: ");
  Serial.println(currentMode);
}

/*
 * 更新呼吸与彩虹效果（由 loop 周期性调用）
 */
void updateEffect(unsigned long now) {
  switch (currentMode) {
    case MODE_BREATH:
      if (now - lastEffectUpdate >= 20) {
        lastEffectUpdate = now;
        breathPhase++;
        float breathBright = (sin(breathPhase * 0.005) + 1) / 2 * brightness;
        int colorIdx = (breathPhase / 200) % colorCount;
        setLed(colorLib[colorIdx][0], colorLib[colorIdx][1], colorLib[colorIdx][2], (int)breathBright);
      }
      break;
    case MODE_RAINBOW:
      if (now - lastEffectUpdate >= 20) {
        lastEffectUpdate = now;
        hue += 2;
        if (hue >= 360) hue -= 360;
        int r, g, b;
        hsvToRgb(hue, 1.0, brightness / 255.0, r, g, b);
        setLed(r, g, b, brightness);
      }
      break;
    default: break;
  }
}

/*
 * HTTP API: 返回当前系统状态 (JSON)
 */
void handleStatus() {
  static StaticJsonDocument<200> doc; // 复用文档避免重复分配
  doc.clear();
  doc["currentMode"] = currentMode;
  doc["brightness"] = brightness;
  doc["colorIndex"] = colorIndex;
  doc["lowPowerMode"] = lowPowerMode;
  doc["customR"] = customR;
  doc["customG"] = customG;
  doc["customB"] = customB;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

/*
 * HTTP API: 设置工作模式
 * @param {String} m - 模式编号 (0~6)
 */
void handleMode() {
  String m = server.arg("m");
  if (m == "0") currentMode = MODE_OFF;
  else if (m == "1") currentMode = MODE_ON;
  else if (m == "2") currentMode = MODE_COLOR;
  else if (m == "3") currentMode = MODE_FLASH;
  else if (m == "4") currentMode = MODE_BREATH;
  else if (m == "5") currentMode = MODE_RAINBOW;
  else if (m == "6") currentMode = MODE_COLORWHEEL;
  lastActivityTime = millis();
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/*
 * HTTP API: 设置固定颜色
 * @param {String} c - 颜色库索引 (0~6)
 */
void handleColor() {
  int c = server.arg("c").toInt();
  if (c >= 0 && c < colorCount) {
    colorIndex = c;
    currentMode = MODE_COLOR;
  }
  lastActivityTime = millis();
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/*
 * HTTP API: 设置全局亮度
 * @param {String} b - 亮度值 (0~255)
 */
void handleBright() {
  int b = server.arg("b").toInt();
  if (b >= 0 && b <= 255) {
    brightness = b;
    Serial.print("亮度更新: ");
    Serial.println(brightness);
  }
  lastActivityTime = millis();
  ledBlink();
  server.send(200, "text/plain", "OK");
}

/*
 * HTTP API: 色轮选色（修正通道映射）
 * @param {String} r - 红色分量 (0~255)
 * @param {String} g - 绿色分量 (0~255)
 * @param {String} b - 蓝色分量 (0~255)
 */
void handleColorWheel() {
  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();
  if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
    // 交换G和B通道，修正蓝绿、黄紫对调问题
    customR = r;
    customG = b;
    customB = g;
    currentMode = MODE_COLORWHEEL;
    Serial.printf("色轮颜色: R=%d, G=%d, B=%d (交换后)\n", customR, customG, customB);
  }
  lastActivityTime = millis();
  ledBlink();
  server.send(200, "text/plain", "OK");
}

// ========== 网页界面 ==========
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>XFR1688 光控矩阵</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    body{background:linear-gradient(135deg,#0a0f1e,#0d1425);font-family:sans-serif;min-height:100vh;padding:20px;color:#e0e0e0}
    .container{max-width:500px;margin:0 auto}
    .header{text-align:center;margin-bottom:20px;padding:15px 0;border-bottom:1px solid rgba(0,255,255,0.2)}
    .header h1{font-size:1.8rem;background:linear-gradient(135deg,#00d2ff,#3a7bd5);-webkit-background-clip:text;background-clip:text;color:transparent}
    .status-card{background:rgba(10,20,40,0.6);backdrop-filter:blur(10px);border-radius:20px;padding:12px;margin-bottom:20px;border:1px solid rgba(0,255,255,0.15);text-align:center}
    .timer-card{background:rgba(0,0,0,0.4);border-radius:20px;padding:12px;margin-bottom:20px;text-align:center;border:1px solid rgba(0,255,255,0.2)}
    .timer-number{font-size:2rem;font-weight:bold;color:#00d2ff;font-family:monospace}
    .timer-label{font-size:0.7rem;color:#6c86a3}
    .btn-group{display:flex;flex-wrap:wrap;gap:10px;margin-bottom:20px;justify-content:center}
    .btn{background:rgba(20,30,50,0.8);border:1px solid rgba(0,255,255,0.2);border-radius:30px;padding:8px 16px;color:#e0e0e0;cursor:pointer;transition:0.2s;font-size:0.9rem}
    .btn:active{transform:scale(0.96);background:#00d2ff;color:#0a0f1e}
    .slider-section{background:rgba(10,20,40,0.4);border-radius:20px;padding:15px;margin-bottom:20px}
    input[type=range]{width:100%;height:4px;-webkit-appearance:none;background:linear-gradient(90deg,#00d2ff,#3a7bd5)}
    input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;background:#00d2ff;border-radius:50%;cursor:pointer}
    .colorwheel-section{text-align:center;margin-bottom:20px}
    canvas{border-radius:50%;box-shadow:0 0 20px rgba(0,210,255,0.3);cursor:crosshair;width:100%;max-width:300px;height:auto;background:#111}
    .footer{text-align:center;margin-top:20px;padding-top:15px;font-size:0.7rem;color:#4a627a}
    .ota-btn{background:#ff4757;margin-top:10px}
  </style>
  <script>
    const modeNames = ['休眠态', '恒光域', '色谱环', '脉冲流', '呼吸模式', '彩虹渐变', '色轮模式'];
    let countdown = 30;
    let timer = null;
    let countdownEl = null;
    let canvas = null, ctx = null;
    let picking = false;

    function stopCountdown() {
      if (timer) { clearInterval(timer); timer = null; }
      countdown = 30;
      if (countdownEl) countdownEl.innerText = countdown;
    }
    function startCountdown() {
      if (timer) stopCountdown();
      timer = setInterval(() => {
        if (countdown > 0) {
          countdown--;
          if (countdownEl) countdownEl.innerText = countdown;
          if (countdown === 0) {
            clearInterval(timer); timer = null;
            alert('✨ 温馨提醒 ✨\n30秒未操作，设备已自动休眠。\n请按板侧按键唤醒。');
          }
        } else if (timer) clearInterval(timer);
      }, 1000);
    }
    function manageCountdownByMode(mode) {
      if (mode === 0) { if (!timer) startCountdown(); }
      else { if (timer) stopCountdown(); }
    }
    function sendMode(mode) {
      document.getElementById('modeDisplay').innerText = modeNames[mode];
      manageCountdownByMode(mode);
      fetch('/mode?m=' + mode + '&t=' + Date.now());
    }
    function sendColor(idx) {
      document.getElementById('modeDisplay').innerText = '色谱环';
      manageCountdownByMode(2);
      fetch('/color?c=' + idx + '&t=' + Date.now());
    }
    function setBrightness(val) {
      document.getElementById('brightVal').innerText = val + '%';
      let b = Math.round(val * 2.55);
      fetch('/bright?b=' + b + '&t=' + Date.now());
      const modeIdx = modeNames.indexOf(document.getElementById('modeDisplay').innerText);
      manageCountdownByMode(modeIdx);
    }
    function fetchStatus() {
      fetch('/status?_=' + Date.now())
        .then(r => r.json())
        .then(data => {
          document.getElementById('modeDisplay').innerText = modeNames[data.currentMode];
          document.getElementById('brightDisplay').innerHTML = '光能 · ' + Math.round(data.brightness / 2.55) + '%';
          document.getElementById('brightSlider').value = Math.round(data.brightness / 2.55);
          document.getElementById('brightVal').innerText = Math.round(data.brightness / 2.55) + '%';
          manageCountdownByMode(data.currentMode);
        });
    }
    function hsvToRgb(h, s, v) {
      let c = v * s;
      let hp = h / 60.0;
      let x = c * (1 - Math.abs((hp % 2) - 1));
      let r1, g1, b1;
      if (hp < 1) { r1 = c; g1 = x; b1 = 0; }
      else if (hp < 2) { r1 = x; g1 = c; b1 = 0; }
      else if (hp < 3) { r1 = 0; g1 = c; b1 = x; }
      else if (hp < 4) { r1 = 0; g1 = x; b1 = c; }
      else if (hp < 5) { r1 = x; g1 = 0; b1 = c; }
      else { r1 = c; g1 = 0; b1 = x; }
      let m = v - c;
      return {r: Math.round((r1 + m)*255), g: Math.round((g1 + m)*255), b: Math.round((b1 + m)*255)};
    }
    function drawColorWheel() {
      if (!canvas || !ctx) return;
      let w = canvas.width, h = canvas.height;
      let centerX = w/2, centerY = h/2, radius = w/2 - 2;
      let imgData = ctx.createImageData(w, h);
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          let dx = x - centerX, dy = y - centerY;
          let dist = Math.sqrt(dx*dx + dy*dy);
          if (dist <= radius) {
            let hue = (Math.atan2(dy, dx) + Math.PI) / (2*Math.PI) * 360;
            // 饱和度调整：中心最小饱和度0.25，避免过白；边缘仍为1.0
            let sat = 0.25 + (dist / radius) * 0.75;
            let val = 1.0;
            let rgb = hsvToRgb(hue, sat, val);
            let idx = (y * w + x) * 4;
            imgData.data[idx] = rgb.r;
            imgData.data[idx+1] = rgb.g;
            imgData.data[idx+2] = rgb.b;
            imgData.data[idx+3] = 255;
          } else {
            let idx = (y * w + x) * 4;
            imgData.data[idx] = 0;
            imgData.data[idx+1] = 0;
            imgData.data[idx+2] = 0;
            imgData.data[idx+3] = 0;
          }
        }
      }
      ctx.putImageData(imgData, 0, 0);
    }
    function pickColor(e) {
      let rect = canvas.getBoundingClientRect();
      let scaleX = canvas.width / rect.width;
      let scaleY = canvas.height / rect.height;
      let clientX, clientY;
      if (e.touches) {
        clientX = e.touches[0].clientX;
        clientY = e.touches[0].clientY;
        e.preventDefault();
      } else {
        clientX = e.clientX;
        clientY = e.clientY;
      }
      let canvasX = (clientX - rect.left) * scaleX;
      let canvasY = (clientY - rect.top) * scaleY;
      if (canvasX >= 0 && canvasX < canvas.width && canvasY >= 0 && canvasY < canvas.height) {
        let pixel = ctx.getImageData(canvasX, canvasY, 1, 1).data;
        let r = pixel[0], g = pixel[1], b = pixel[2];
        if (r+g+b > 0) {
          fetch('/colorwheel?r=' + r + '&g=' + g + '&b=' + b + '&t=' + Date.now());
          document.getElementById('modeDisplay').innerText = '色轮模式';
          manageCountdownByMode(6);
        }
      }
    }
    function initColorWheel() {
      canvas = document.getElementById('colorWheel');
      ctx = canvas.getContext('2d');
      canvas.width = 300;
      canvas.height = 300;
      drawColorWheel();
      canvas.addEventListener('mousedown', (e) => { picking = true; pickColor(e); });
      window.addEventListener('mousemove', (e) => { if (picking) pickColor(e); });
      window.addEventListener('mouseup', () => { picking = false; });
      canvas.addEventListener('touchstart', (e) => { picking = true; pickColor(e); });
      canvas.addEventListener('touchmove', (e) => { pickColor(e); });
      canvas.addEventListener('touchend', () => { picking = false; });
    }
    window.onload = function() {
      countdownEl = document.getElementById('countdownNum');
      fetchStatus();
      initColorWheel();
    };
  </script>
</head>
<body>
<div class="container">
  <div class="header"><h1>✦ 光控矩阵 ✦</h1><p>XFR1688 · 色轮拾色版</p></div>
  <div class="status-card">
    <div class="label">当前模式</div>
    <div class="value" id="modeDisplay">休眠态</div>
  </div>
  <div class="timer-card">
    <div class="timer-number" id="countdownNum">30</div>
    <div class="timer-label">秒后自动休眠（仅休眠态有效）</div>
  </div>
  <div class="btn-group">
    <button class="btn" onclick="sendMode(0)">◐ 休眠</button>
    <button class="btn" onclick="sendMode(1)">◉ 恒光</button>
    <button class="btn" onclick="sendMode(2)">◈ 色谱</button>
    <button class="btn" onclick="sendMode(3)">◍ 脉冲</button>
    <button class="btn" onclick="sendMode(4)">♨ 呼吸</button>
    <button class="btn" onclick="sendMode(5)">🌈 彩虹</button>
    <button class="btn" onclick="sendMode(6)">🎨 色轮</button>
  </div>
  <div class="color-grid" style="display:flex; flex-wrap:wrap; gap:10px; justify-content:center; margin-bottom:20px">
    <div style="width:40px;height:40px;border-radius:50%;background:#ff3b30" onclick="sendColor(0)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#007aff" onclick="sendColor(1)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#34c759" onclick="sendColor(2)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#af52de" onclick="sendColor(3)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#ffcc00" onclick="sendColor(4)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#5ac8fa" onclick="sendColor(5)"></div>
    <div style="width:40px;height:40px;border-radius:50%;background:#ffffff" onclick="sendColor(6)"></div>
  </div>
  <div class="slider-section">
    <input type="range" id="brightSlider" min="0" max="100" value="50" oninput="setBrightness(this.value)">
    <div style="text-align:center;margin-top:8px"><span id="brightVal">50%</span></div>
  </div>
  <div class="colorwheel-section">
    <canvas id="colorWheel" width="300" height="300" style="width:100%; max-width:300px; height:auto; border-radius:50%"></canvas>
    <p style="font-size:0.7rem; margin-top:5px">✨ 点击或拖动色轮任意位置 ✨</p>
  </div>
  <div class="footer">
    <p>长按板侧按键 · 无级调光</p>
    <p>灯光关闭30秒后自动休眠，按键唤醒</p>
    <p>✧ 王超老师指导 · XFR1688 ✧</p>
    <button class="btn ota-btn" onclick="window.open('/update','_blank')">📡 固件升级 (OTA)</button>
  </div>
</div>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// ========== OTA 设置 ==========
void setupOTA() {
  httpUpdater.setup(&server, "/update");
}

// ========== WiFi初始化 ==========
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
  Serial.print("AP: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/color", handleColor);
  server.on("/bright", handleBright);
  server.on("/status", handleStatus);
  server.on("/colorwheel", handleColorWheel);
  setupOTA();
  server.begin();
}

// ========== 低功耗 ==========
void ICACHE_RAM_ATTR keyISR() { wakeUpRequest = true; }

/*
 * 进入低功耗模式，关闭WiFi，等待按键唤醒
 */
void enterLowPower() {
  Serial.println("进入低功耗");
  lowPowerMode = true;
  WiFi.forceSleepBegin();
  WiFi.mode(WIFI_OFF);
  attachInterrupt(digitalPinToInterrupt(KEY_PIN), keyISR, FALLING);
  system_update_cpu_freq(80);
  while (!wakeUpRequest) {
    delay(1); // 缩短阻塞时间降低功耗
  }
  exitLowPower();
}

/*
 * 退出低功耗模式，恢复WiFi和CPU频率
 */
void exitLowPower() {
  Serial.println("退出低功耗");
  detachInterrupt(digitalPinToInterrupt(KEY_PIN));
  system_update_cpu_freq(160);
  WiFi.forceSleepWake();
  delay(100);
  initWiFi();
  wakeUpRequest = false;
  lowPowerMode = false;
  lastActivityTime = millis();
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}

// ========== setup ==========
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  pinMode(KEY_PIN, INPUT_PULLUP);
  turnOff();
  Serial.begin(115200);
  delay(100);
  initWiFi();
  lastActivityTime = millis();
  Serial.println("系统就绪，色轮颜色已修正，OTA可用，性能优化已应用");
}

// ========== loop ==========
void loop() {
  unsigned long now = millis(); // 统一获取时间基准

  if (!lowPowerMode) {
    server.handleClient();
  }
  updateLedBlink();

  int key = digitalRead(KEY_PIN);
  
  // 低功耗模式下仅检测按键唤醒
  if (lowPowerMode) {
    if (key == LOW) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(KEY_PIN) == LOW) {
        while (digitalRead(KEY_PIN) == LOW) {
          delay(1); // 缩短阻塞
        }
        if (lowPowerMode) exitLowPower();
      }
    }
    yield();
    return;
  }
  
  // 按键状态机（使用 now 变量）
  if (key == LOW && !isPressing) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(KEY_PIN) == HIGH) {
      // 抖动误触发，忽略
    } else {
      isPressing = true;
      pressStartTime = now;
      lastActivityTime = now;
    }
  }
  
  if (key == HIGH && isPressing) {
    isPressing = false;
    unsigned long dur = now - pressStartTime;
    if (dur < SHORT_PRESS_TIME) {
      switchMode();
    } else {
      Serial.print("长按结束，亮度: ");
      Serial.println(brightness);
    }
    lastActivityTime = now;
  }
  
  if (isPressing && (now - pressStartTime) >= LONG_PRESS_TIME) {
    if (currentMode != MODE_OFF && (now - lastDimmingTime) >= DIMMING_STEP_TIME) {
      lastDimmingTime = now;
      if (brightnessUp) {
        brightness++;
        if (brightness >= 255) {
          brightness = 255;
          brightnessUp = false;
        }
      } else {
        brightness--;
        if (brightness <= 0) {
          brightness = 0;
          brightnessUp = true;
        }
      }
    }
  }
  
  // 自动休眠检测
  if (currentMode == MODE_OFF && !lowPowerMode) {
    if (now - lastActivityTime > IDLE_TIME_BEFORE_SLEEP) {
      enterLowPower();
      return;
    }
  } else if (currentMode != MODE_OFF) {
    lastActivityTime = now;
  }
  
  // 更新动态效果
  updateEffect(now);
  
  // 根据当前模式刷新LED输出
  switch (currentMode) {
    case MODE_OFF:
      turnOff();
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
    default:
      break;
  }
  
  yield(); // 确保 WiFi 后台任务执行
}