// 前端HTML资源定义：HTML/CSS/JS 存入 Flash，仅此一份
#include <Arduino.h>
#include "headers/frontend.h"

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>网安物联Card</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    :root {
      --bg-1: #07111f;
      --bg-2: #0d1b30;
      --panel: rgba(10, 22, 40, 0.72);
      --border: rgba(116, 202, 255, 0.18);
      --text-main: #eaf4ff;
      --text-sub: #8aa6c7;
      --accent: #55d6ff;
      --accent-2: #7c8cff;
      --shadow: 0 18px 45px rgba(0, 0, 0, 0.35);
      --ota-gray-1: #616974;
      --ota-gray-2: #858d98;
    }
    body {
      min-height: 100vh;
      padding: 18px;
      color: var(--text-main);
      font-family: "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
      background:
        radial-gradient(circle at top left, rgba(85, 214, 255, 0.12), transparent 34%),
        radial-gradient(circle at top right, rgba(124, 140, 255, 0.12), transparent 30%),
        linear-gradient(160deg, var(--bg-1), var(--bg-2));
    }
    .container {
      max-width: 540px;
      margin: 0 auto;
      padding: 12px 0 16px;
    }
    .header {
      position: relative;
      text-align: center;
      margin-bottom: 18px;
      padding: 22px 18px 18px;
      border: 1px solid var(--border);
      border-radius: 24px;
      background: linear-gradient(180deg, rgba(15, 30, 54, 0.94), rgba(9, 19, 34, 0.82));
      box-shadow: var(--shadow);
      backdrop-filter: blur(12px);
    }
    .header h1 {
      font-size: 1.9rem;
      letter-spacing: 0.04em;
      background: linear-gradient(135deg, #ffffff, #7ddfff 55%, #9faeff);
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .header p {
      margin-top: 8px;
      color: var(--text-sub);
      font-size: 0.92rem;
    }
    .ota-fab {
      position: absolute;
      left: 10px;
      top: 50%;
      transform: translateY(-50%);
      width: 38px;
      height: 38px;
      border: 1px solid rgba(255, 255, 255, 0.12);
      border-radius: 50%;
      background: linear-gradient(180deg, var(--ota-gray-2), var(--ota-gray-1));
      color: #ffffff;
      font-size: 1rem;
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: 0 10px 22px rgba(0, 0, 0, 0.22);
      cursor: pointer;
      transition: transform 0.18s ease, filter 0.18s ease;
    }
    .ota-fab:active {
      transform: translateY(-50%) scale(0.94);
      filter: brightness(0.95);
    }
    .status-card,
    .timer-card,
    .slider-section,
    .colorwheel-section,
    .footer {
      border: 1px solid var(--border);
      border-radius: 22px;
      background: var(--panel);
      backdrop-filter: blur(12px);
      box-shadow: var(--shadow);
    }
    .status-card {
      padding: 16px;
      margin-bottom: 16px;
      text-align: center;
    }
    .label {
      font-size: 0.78rem;
      color: var(--text-sub);
      letter-spacing: 0.08em;
    }
    .value {
      margin-top: 8px;
      font-size: 1.4rem;
      font-weight: 700;
      color: var(--text-main);
    }
    .bright-title {
      text-align: center;
      margin-top: 8px;
      color: var(--text-sub);
      font-size: 0.86rem;
    }
    .timer-card {
      display: none;
      padding: 10px 14px;
      margin-bottom: 16px;
      text-align: center;
    }
    .timer-card.show {
      display: block;
      animation: fadeInUp 0.22s ease;
    }
    .timer-number {
      font-size: 1.2rem;
      font-weight: 700;
      color: var(--accent);
      font-family: Consolas, Monaco, monospace;
      line-height: 1.1;
    }
    .timer-label {
      margin-top: 6px;
      font-size: 0.72rem;
      color: var(--text-sub);
    }
    .btn-group {
      display: grid;
      grid-template-columns: repeat(5, 1fr);
      gap: 8px;
      margin-bottom: 16px;
      padding: 2px 4px;
    }
    .btn {
      width: 100%;
      min-height: 44px;
      padding: 10px 4px;
      border: 1px solid rgba(117, 201, 255, 0.14);
      border-radius: 16px;
      background: linear-gradient(180deg, rgba(26, 42, 69, 0.95), rgba(15, 25, 43, 0.95));
      color: var(--text-main);
      font-size: 0.95rem;
      cursor: pointer;
      transition: transform 0.18s ease, background 0.18s ease, border-color 0.18s ease;
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.03);
    }
    .btn:active {
      transform: scale(0.97);
      background: linear-gradient(180deg, rgba(72, 161, 255, 0.28), rgba(23, 58, 110, 0.9));
      border-color: rgba(117, 201, 255, 0.34);
    }
    .color-grid {
      display: grid;
      grid-template-columns: repeat(7, 1fr);
      gap: 10px;
      margin-bottom: 16px;
    }
    .color-dot {
      width: 100%;
      aspect-ratio: 1;
      border-radius: 50%;
      border: 2px solid rgba(255, 255, 255, 0.18);
      box-shadow: 0 6px 18px rgba(0, 0, 0, 0.28);
      cursor: pointer;
      transition: transform 0.18s ease;
    }
    .color-dot:active { transform: scale(0.92); }
    .slider-section {
      padding: 16px;
      margin-bottom: 16px;
    }
    input[type=range] {
      width: 100%;
      height: 6px;
      border-radius: 999px;
      -webkit-appearance: none;
      background: linear-gradient(90deg, var(--accent), var(--accent-2));
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border: 2px solid rgba(255, 255, 255, 0.85);
      border-radius: 50%;
      background: #ffffff;
      box-shadow: 0 0 0 6px rgba(85, 214, 255, 0.14);
      cursor: pointer;
    }
    .slider-value {
      text-align: center;
      margin-top: 10px;
      font-size: 1rem;
      font-weight: 600;
      color: var(--text-main);
    }
    .colorwheel-section {
      padding: 18px 16px 14px;
      margin-bottom: 16px;
      text-align: center;
    }
    canvas {
      width: 100%;
      max-width: 300px;
      height: auto;
      margin: 0 auto;
      border-radius: 50%;
      background: #111820;
      box-shadow: 0 0 0 8px rgba(255, 255, 255, 0.03), 0 18px 30px rgba(0, 0, 0, 0.28);
      cursor: crosshair;
    }
    .wheel-tip {
      margin-top: 10px;
      font-size: 0.76rem;
      color: var(--text-sub);
    }
    .footer {
      padding: 16px;
      text-align: center;
      color: var(--text-sub);
      font-size: 0.78rem;
      line-height: 1.75;
    }
    @keyframes fadeInUp {
      from { opacity: 0; transform: translateY(8px); }
      to { opacity: 1; transform: translateY(0); }
    }
    /* OTA弹窗样式 */
    .modal-overlay {
      display: none;
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0, 0, 0, 0.72);
      backdrop-filter: blur(8px);
      z-index: 1000;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .modal-overlay.show {
      display: flex;
      animation: fadeIn 0.22s ease;
    }
    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }
    .modal-box {
      width: 100%;
      max-width: 400px;
      padding: 24px;
      border: 1px solid var(--border);
      border-radius: 24px;
      background: linear-gradient(180deg, rgba(15, 30, 54, 0.96), rgba(9, 19, 34, 0.88));
      box-shadow: var(--shadow);
      animation: fadeInUp 0.25s ease;
    }
    .modal-title {
      font-size: 1.3rem;
      font-weight: 700;
      text-align: center;
      margin-bottom: 8px;
      background: linear-gradient(135deg, #ffffff, #7ddfff 55%, #9faeff);
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .modal-desc {
      font-size: 0.86rem;
      color: var(--text-sub);
      text-align: center;
      margin-bottom: 20px;
      line-height: 1.6;
    }
    .file-input-wrapper {
      position: relative;
      margin-bottom: 16px;
    }
    .file-input {
      width: 100%;
      padding: 14px 16px;
      border: 2px dashed rgba(117, 201, 255, 0.3);
      border-radius: 16px;
      background: rgba(10, 22, 40, 0.5);
      color: var(--text-main);
      font-size: 0.9rem;
      cursor: pointer;
      transition: border-color 0.2s ease, background 0.2s ease;
    }
    .file-input:hover {
      border-color: rgba(117, 201, 255, 0.5);
      background: rgba(10, 22, 40, 0.7);
    }
    .file-name {
      font-size: 0.8rem;
      color: var(--accent);
      text-align: center;
      margin-top: 8px;
      min-height: 1.2em;
    }
    .modal-actions {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-top: 20px;
    }
    .modal-btn {
      padding: 12px 20px;
      border: 1px solid rgba(117, 201, 255, 0.14);
      border-radius: 14px;
      font-size: 0.95rem;
      cursor: pointer;
      transition: transform 0.18s ease, background 0.18s ease;
    }
    .modal-btn:active { transform: scale(0.97); }
    .modal-btn-secondary {
      background: linear-gradient(180deg, rgba(26, 42, 69, 0.8), rgba(15, 25, 43, 0.8));
      color: var(--text-sub);
    }
    .modal-btn-primary {
      background: linear-gradient(135deg, rgba(85, 214, 255, 0.25), rgba(124, 140, 255, 0.25));
      color: var(--text-main);
      border-color: rgba(117, 201, 255, 0.34);
    }
    .modal-btn-primary:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .upload-progress {
      display: none;
      margin-top: 16px;
      padding: 16px;
      border-radius: 14px;
      background: rgba(10, 22, 40, 0.6);
      text-align: center;
    }
    .upload-progress.show { display: block; }
    .progress-bar {
      width: 100%;
      height: 6px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.1);
      overflow: hidden;
      margin-top: 12px;
    }
    .progress-fill {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, var(--accent), var(--accent-2));
      border-radius: 999px;
      transition: width 0.3s ease;
    }
    .progress-text {
      font-size: 0.85rem;
      color: var(--accent);
      margin-top: 8px;
    }
    @media (max-width: 420px) {
      body { padding: 14px; }
      .color-grid { gap: 8px; }
      .header h1 { font-size: 1.6rem; }
      .ota-fab { left: 8px; width: 36px; height: 36px; }
      .modal-box { padding: 20px; }
      .modal-actions { grid-template-columns: 1fr; }
    }
  </style>
  <script>
    const modeNames = ['休眠', '常亮', '色谱环', '脉冲', '呼吸', '彩虹渐变', '色轮模式'];
    let countdown = 30;
    let timer = null;
    let countdownEl = null;
    let timerCardEl = null;
    let canvas = null, ctx = null;
    let picking = false;

    function updateTimerVisibility(show) {
      if (!timerCardEl) return;
      if (show) timerCardEl.classList.add('show');
      else timerCardEl.classList.remove('show');
    }

    function resetCountdownValue() {
      countdown = 30;
      if (countdownEl) countdownEl.innerText = countdown;
    }

    function stopCountdown() {
      if (timer) {
        clearInterval(timer);
        timer = null;
      }
      resetCountdownValue();
      updateTimerVisibility(false);
    }

    function interruptCountdownIfNeeded() {
      stopCountdown();
    }



    function startCountdown() {
      stopCountdown();
      updateTimerVisibility(true);
      timer = setInterval(() => {
        if (countdown > 0) {
          countdown--;
          if (countdownEl) countdownEl.innerText = countdown;
          if (countdown === 0) {
            clearInterval(timer);
            timer = null;
            alert('✨ 温馨提醒 ✨\n30秒未操作，设备已自动休眠。\n请按板侧按键唤醒。');
          }
        } else if (timer) {
          clearInterval(timer);
          timer = null;
        }
      }, 1000);
    }

    function manageCountdownByMode(mode) {
      if (mode === 0) startCountdown();
      else stopCountdown();
    }

    function sendMode(mode) {
      document.getElementById('modeDisplay').innerText = modeNames[mode];
      manageCountdownByMode(mode);
      fetch('/mode?m=' + mode + '&t=' + Date.now());
    }

    function sendColor(idx) {
      interruptCountdownIfNeeded();
      document.getElementById('modeDisplay').innerText = '色谱环';
      fetch('/color?c=' + idx + '&t=' + Date.now());
    }

    function setBrightness(val) {
      interruptCountdownIfNeeded();
      document.getElementById('brightVal').innerText = val + '%';
      let b = Math.round(val * 2.55);
      fetch('/bright?b=' + b + '&t=' + Date.now());
    }

    function fetchStatus() {
      fetch('/status?_=' + Date.now())
        .then(r => r.json())
        .then(data => {
          document.getElementById('modeDisplay').innerText = modeNames[data.currentMode];
          document.getElementById('brightDisplay').innerHTML = '\u5149\u80fd \u00b7 ' + Math.round(data.brightness / 2.55) + '%';
          document.getElementById('brightSlider').value = Math.round(data.brightness / 2.55);
          document.getElementById('brightVal').innerText = Math.round(data.brightness / 2.55) + '%';
          if (data.sleepCountdown) startCountdown();
          else if (data.currentMode === 0) stopCountdown();
          else updateTimerVisibility(false);
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
      return {r: Math.round((r1 + m) * 255), g: Math.round((g1 + m) * 255), b: Math.round((b1 + m) * 255)};
    }

    function drawColorWheel() {
      if (!canvas || !ctx) return;
      let w = canvas.width, h = canvas.height;
      let centerX = w / 2, centerY = h / 2, radius = w / 2 - 2;
      let imgData = ctx.createImageData(w, h);
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          let dx = x - centerX, dy = y - centerY;
          let dist = Math.sqrt(dx * dx + dy * dy);
          let idx = (y * w + x) * 4;
          if (dist <= radius) {
            let hue = (Math.atan2(dy, dx) + Math.PI) / (2 * Math.PI) * 360;
            let sat = 0.25 + (dist / radius) * 0.75;
            let val = 1.0;
            let rgb = hsvToRgb(hue, sat, val);
            imgData.data[idx] = rgb.r;
            imgData.data[idx + 1] = rgb.g;
            imgData.data[idx + 2] = rgb.b;
            imgData.data[idx + 3] = 255;
          } else {
            imgData.data[idx] = 0;
            imgData.data[idx + 1] = 0;
            imgData.data[idx + 2] = 0;
            imgData.data[idx + 3] = 0;
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
        if (r + g + b > 0) {
          interruptCountdownIfNeeded();
          fetch('/colorwheel?r=' + r + '&g=' + g + '&b=' + b + '&t=' + Date.now());
          document.getElementById('modeDisplay').innerText = '色轮模式';
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

    // OTA弹窗相关函数
    function showOTAModal() {
      document.getElementById('otaModal').classList.add('show');
      document.getElementById('otaFile').value = '';
      document.getElementById('fileName').textContent = '';
      document.getElementById('uploadProgress').classList.remove('show');
      document.getElementById('progressFill').style.width = '0%';
      document.getElementById('progressText').textContent = '';
      document.getElementById('uploadBtn').disabled = true;
    }

    function hideOTAModal() {
      document.getElementById('otaModal').classList.remove('show');
    }

    function onFileSelected(input) {
      const file = input.files[0];
      const fileNameEl = document.getElementById('fileName');
      const uploadBtn = document.getElementById('uploadBtn');
      if (file) {
        fileNameEl.textContent = '已选择: ' + file.name;
        uploadBtn.disabled = false;
      } else {
        fileNameEl.textContent = '';
        uploadBtn.disabled = true;
      }
    }

    function submitOTA() {
      const fileInput = document.getElementById('otaFile');
      const file = fileInput.files[0];
      if (!file) return;

      const progressDiv = document.getElementById('uploadProgress');
      const progressFill = document.getElementById('progressFill');
      const progressText = document.getElementById('progressText');
      const uploadBtn = document.getElementById('uploadBtn');
      const cancelBtn = document.getElementById('cancelBtn');

      progressDiv.classList.add('show');
      uploadBtn.disabled = true;
      cancelBtn.disabled = true;
      progressText.textContent = '正在准备升级...';

      // 先发送otastart请求，设置otaInProgress标志防止进入低功耗模式
      fetch('/otastart?t=' + Date.now())
        .then(() => {
          progressText.textContent = '正在上传固件...';
          doUpload();
        })
        .catch(() => {
          progressText.textContent = '准备升级失败，请重试';
          uploadBtn.disabled = false;
          cancelBtn.disabled = false;
        });

      function doUpload() {
      const xhr = new XMLHttpRequest();
      const formData = new FormData();
      formData.append('firmware', file);

      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          const percent = Math.round((e.loaded / e.total) * 100);
          progressFill.style.width = percent + '%';
          progressText.textContent = '上传进度: ' + percent + '%';
        }
      };

      xhr.onload = function() {
        if (xhr.status === 200) {
          progressFill.style.width = '100%';
          progressText.textContent = '上传成功! 设备正在重启...';
          setTimeout(function() {
            alert('固件升级成功！设备即将重启。');
            hideOTAModal();
            location.reload();
          }, 2000);
        } else {
          progressText.textContent = '上传失败: ' + xhr.statusText;
          uploadBtn.disabled = false;
          cancelBtn.disabled = false;
        }
      };

      xhr.onerror = function() {
        progressText.textContent = '上传失败，请检查网络连接';
        uploadBtn.disabled = false;
        cancelBtn.disabled = false;
      };

      xhr.open('POST', '/update');
      xhr.send(formData);
      }
    }

    // 点击遮罩层关闭弹窗
    document.addEventListener('click', function(e) {
      const modal = document.getElementById('otaModal');
      if (e.target === modal) {
        hideOTAModal();
      }
    });

    window.onload = function() {
      countdownEl = document.getElementById('countdownNum');
      timerCardEl = document.getElementById('timerCard');
      resetCountdownValue();
      fetchStatus();
      initColorWheel();
    };
  </script>
</head>
<body>
<div class="container">
  <div class="header">
    <button class="ota-fab" onclick="showOTAModal()" aria-label="固件升级">📡</button>
    <h1>网安物联Card</h1>
    <p>XFR1688 & UXU倒計時</p>
  </div>

  <div class="status-card">
    <div class="label">当前模式</div>
    <div class="value" id="modeDisplay">休眠</div>
  </div>

  <div class="timer-card" id="timerCard">
    <div class="timer-number" id="countdownNum">30</div>
    <div class="timer-label">秒后自动休眠</div>
  </div>

  <div class="btn-group">
    <button class="btn" onclick="sendMode(0)">◐ 休眠</button>
    <button class="btn" onclick="sendMode(1)">◉ 常亮</button>
    <button class="btn" onclick="sendMode(3)">◍ 脉冲</button>
    <button class="btn" onclick="sendMode(4)">♨ 呼吸</button>
    <button class="btn" onclick="sendMode(5)">🌈 彩虹</button>
  </div>

  <div class="color-grid">
    <div class="color-dot" style="background:#ff3b30" onclick="sendColor(0)"></div>
    <div class="color-dot" style="background:#007aff" onclick="sendColor(1)"></div>
    <div class="color-dot" style="background:#34c759" onclick="sendColor(2)"></div>
    <div class="color-dot" style="background:#af52de" onclick="sendColor(3)"></div>
    <div class="color-dot" style="background:#ffcc00" onclick="sendColor(4)"></div>
    <div class="color-dot" style="background:#5ac8fa" onclick="sendColor(5)"></div>
    <div class="color-dot" style="background:#ffffff" onclick="sendColor(6)"></div>
  </div>

  <div class="slider-section">
    <div class="bright-title">亮度调节</div>
    <input type="range" id="brightSlider" min="0" max="100" value="10" oninput="setBrightness(this.value)">
    <div class="slider-value"><span id="brightVal">10%</span></div>
  </div>

  <div class="colorwheel-section">
    <canvas id="colorWheel" width="300" height="300"></canvas>
    <p class="wheel-tip">点击或拖动色轮任意位置</p>
  </div>

  <div class="footer">
    <p>长按板侧按键 · 无级调光</p>
    <p>灯光关闭 30 秒后自动休眠，按键唤醒</p>
    <p>王超老师指导 · XFR1688 & UXU倒計時</p>
  </div>
</div>

<!-- OTA升级弹窗 -->
<div class="modal-overlay" id="otaModal">
  <div class="modal-box">
    <div class="modal-title">📡 固件升级</div>
    <div class="modal-desc">请选择要上传的固件文件(.bin格式)<br>升级过程中请勿断电或刷新页面</div>
    <div class="file-input-wrapper">
      <input type="file" class="file-input" id="otaFile" accept=".bin" onchange="onFileSelected(this)">
      <div class="file-name" id="fileName"></div>
    </div>
    <div class="upload-progress" id="uploadProgress">
      <div class="progress-bar">
        <div class="progress-fill" id="progressFill"></div>
      </div>
      <div class="progress-text" id="progressText"></div>
    </div>
    <div class="modal-actions">
      <button class="modal-btn modal-btn-secondary" id="cancelBtn" onclick="hideOTAModal()">取消</button>
      <button class="modal-btn modal-btn-primary" id="uploadBtn" onclick="submitOTA()" disabled>开始升级</button>
    </div>
  </div>
</div>

</body>
</html>
)rawliteral";
