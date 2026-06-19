#pragma once
/**
// ============================================================
// Web遥控界面
 * @brief 淘宝店: 耀坤智联 https://shop35432590.taobao.com/
 * @brief 提供本项目所需电子元器件套件，基础元件仅需9.9元 ；  
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 *
// 20260602 增加12色主题切换，主题视图为3x4网格
// 20260604 增加全屏切换按钮（⛶ / ⤬）
// 20260611 增加 IMU 滤波器切换控件（Madgwick / Mahony / 互补滤波）
// 20260611 增加所有“应用”按钮的蜂鸣提示音（Web Audio）
// 20260612 将“状态消息”移至遥控界面，并优化样式使其更醒目；优化浅色主题下虚拟摇杆中心点的可见性（使用主题变量）
// 20260612 增强设置页面“actionStatus”短暂消息的样式（更大更醒目）
// ============================================================
*/

static const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport"
        content="width=device-width,initial-scale=1,viewport-fit=cover,user-scalable=no">
  <title>耀坤无人机Web遥控</title>
  <style>
    :root{
      --gap: 8px;
      --card-pad: 10px;

      /* portrait fallback */
      --pad-size: min(30vh, 30vw, 320px);
      --center-width: min(360px, 96vw);
    }

    html, body { height: 100%; }

    body {
      font-family: sans-serif;
      margin: 0;
      padding: calc(6px + env(safe-area-inset-top))
               calc(6px + env(safe-area-inset-right))
               calc(6px + env(safe-area-inset-bottom))
               calc(6px + env(safe-area-inset-left));
      display: flex;
      flex-direction: column;
      gap: var(--gap);
      box-sizing: border-box;
      overflow: hidden;
      background: #fff;
    }

    * { box-sizing: border-box; }

    .card {
      border: 1px solid #ccc;
      border-radius: 10px;
      padding: var(--card-pad);
      background: #fff;
    }

    .row {
      display: flex;
      gap: 10px;
      align-items: center;
      flex-wrap: wrap;
    }

    .danger { font-weight: 700; }

    button {
      font-size: 16px;
      padding: 10px 14px;
    }

    input[type="number"] {
      width: 90px;
      font-size: 16px;
    }

    small { color: #555; }

    .kv {
      display: grid;
      grid-template-columns: 110px 1fr;
      gap: 6px 10px;
    }

    #modeBar {
      display: flex;
      gap: 6px;
      align-items: center;
      justify-content: center;
    }

    .modeBtn {
      font-size: 14px;
      padding: 6px 10px;
      border: 1px solid #888;
      border-radius: 8px;
      background: #f4f4f4;
    }

    .modeBtn.active {
      background: #222;
      color: #fff;
      border-color: #222;
    }

    #controlView,
    #settingsView,
    #motorTestView,
    #themeView {
      flex: 1 1 auto;
      width: 100%;
      min-height: 0;
    }

    #settingsView,
    #motorTestView,
    #themeView {
      display: none;
      overflow: auto;
    }

    body[data-mode="control"] #controlView { display: block; }
    body[data-mode="control"] #settingsView { display: none; }
    body[data-mode="control"] #motorTestView { display: none; }
    body[data-mode="control"] #themeView { display: none; }
    body[data-mode="settings"] #controlView { display: none; }
    body[data-mode="settings"] #settingsView { display: block; }
    body[data-mode="settings"] #motorTestView { display: none; }
    body[data-mode="settings"] #themeView { display: none; }
    body[data-mode="motor"] #controlView { display: none; }
    body[data-mode="motor"] #settingsView { display: none; }
    body[data-mode="motor"] #motorTestView { display: block; }
    body[data-mode="motor"] #themeView { display: none; }
    body[data-mode="theme"] #controlView { display: none; }
    body[data-mode="theme"] #settingsView { display: none; }
    body[data-mode="theme"] #motorTestView { display: none; }
    body[data-mode="theme"] #themeView { display: block; }

    #settingsLayout {
      height: 100%;
      display: flex;
      flex-direction: column;
      gap: var(--gap);
      align-items: center;
      justify-content: flex-start;
    }

    #motorTestLayout,
    #themeLayout {
      height: 100%;
      display: flex;
      flex-direction: column;
      gap: var(--gap);
      align-items: center;
      justify-content: flex-start;
    }

    #attCard {
      width: min(320px, 96vw);
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    #attCanvas {
      width: 100%;
      max-width: 280px;
      aspect-ratio: 1;
      display: block;
      margin: 0 auto;
      background: #f7f7f7;
      border-radius: 10px;
    }

    #tuneCard {
      width: min(420px, 96vw);
      display: flex;
      flex-direction: column;
      gap: 10px;
    }

    #motorTestCard,
    #themeCard {
      width: min(420px, 96vw);
      display: flex;
      flex-direction: column;
      gap: 10px;
    }

    .section { margin-top: 8px; }

    .section .row { margin-top: 6px; }

    .muted { color: #666; font-size: 12px; }

    #app {
      height: 100%;
      width: 100%;
      display: flex;
      flex-direction: column;
      gap: var(--gap);
      align-items: center;
      justify-content: flex-start;
    }

    .padCol {
      display: flex;
      align-items: center;
      justify-content: center;
      width: 100%;
    }

    .joy {
      width: var(--pad-size);
      height: var(--pad-size);
      border: 2px solid #444;
      border-radius: 12px;
      position: relative;
      touch-action: none;
      user-select: none;
      -webkit-user-select: none;
      -webkit-touch-callout: none;
      -webkit-tap-highlight-color: transparent;
      overflow: hidden;
    }

    .joyLabel {
      position: absolute;
      left: 10px;
      top: 10px;
      font-size: 14px;
      user-select: none;
      -webkit-user-select: none;
      -webkit-touch-callout: none;
      -webkit-tap-highlight-color: transparent;
    }

    /* 摇杆中心点样式：使用 CSS 变量，主题各自定义 */
    .stick {
      width: 60px;
      height: 60px;
      border-radius: 999px;
      background: var(--stick-bg, #555); /* 浅色主题默认使用深灰色 #555 */
      opacity: 0.8;
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%,-50%);
      user-select: none;
      -webkit-user-select: none;
      -webkit-touch-callout: none;
      -webkit-tap-highlight-color: transparent;
      box-shadow: 0 2px 5px rgba(0,0,0,0.2);
    }

    #centerCol {
      width: var(--center-width);
      display: flex;
      flex-direction: column;
      gap: var(--gap);
    }

    #telemetryCard { flex: 1; overflow: hidden; }

    @media (orientation:landscape) {
      :root{
        --pad-size: min(60vh, 32vw, 260px);
        --center-width: min(360px, 42vw, 380px);
        --card-pad: 6px;
      }

      #app {
        height: 100%;
        flex-direction: row;
        align-items: stretch;
        justify-content: center;
        gap: 6px;
      }

      .padCol {
        width: auto;
        flex: 0 0 auto;
      }

      #centerCol {
        height: 100%;
        max-height: 100%;
        min-height: 0;
        font-size: 14px;
      }

      #controlCard small { display: none; }

      #telemetryCard {
        overflow: auto;
        min-height: 0;
      }

      #telemetryCard .kv {
        grid-template-columns: 86px minmax(0, 1fr) 70px minmax(0, 1fr);
        gap: 4px 8px;
        font-size: 13px;
      }

      #telemetryCard .kv div {
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
      }

      #settingsLayout {
        flex-direction: row;
        align-items: stretch;
        justify-content: center;
      }

      #attCard {
        flex: 0 0 260px;
      }

      #attCanvas {
        max-width: 240px;
      }

      #tuneCard {
        flex: 1 1 360px;
        max-width: 520px;
        min-height: 0;
        overflow: auto;
      }

      .stick {
        width: 56px;
        height: 56px;
      }

      button {
        padding: 8px 12px;
      }
    }

    /* ========== 十二种主题CSS变量（不影响原有布局） ========== */
    /* 浅色（默认）已在根样式中定义，无需额外类 */
    /* 暗黑 */
    body.theme-dark {
      --bg-body: #121212;
      --card-bg: #1e1e1e;
      --card-border: #444;
      --text-primary: #e0e0e0;
      --text-secondary: #aaa;
      --btn-bg: #2c2c2c;
      --btn-border: #666;
      --btn-active-bg: #0a84ff;
      --btn-active-text: #fff;
      --joy-border: #aaa;
      --stick-bg: #aaa;
      --canvas-bg: #2a2a2a;
    }
    /* 海棠（浅红） */
    body.theme-crabapple {
      --bg-body: #fff5f0;
      --card-bg: #ffffff;
      --card-border: #e69b8c;
      --text-primary: #a63e2c;
      --text-secondary: #d67a63;
      --btn-bg: #ffe3db;
      --btn-border: #e69b8c;
      --btn-active-bg: #d45c44;
      --btn-active-text: #fff;
      --joy-border: #d45c44;
      --stick-bg: #d45c44;
      --canvas-bg: #fff0ea;
    }
    /* 朱红（深红） */
    body.theme-vermilion {
      --bg-body: #2f1100;
      --card-bg: #592c16;
      --card-border: #a15028;
      --text-primary: #fff0e8;
      --text-secondary: #f0bc9a;
      --btn-bg: #753d1f;
      --btn-border: #c46838;
      --btn-active-bg: #e68a4d;
      --btn-active-text: #fff;
      --joy-border: #e68a4d;
      --stick-bg: #e68a4d;
      --canvas-bg: #6b3820;
    }
    /* 藕荷（粉紫） */
    body.theme-lotusroot {
      --bg-body: #faf2f0;
      --card-bg: #ffffff;
      --card-border: #cfa5b0;
      --text-primary: #7a4c5c;
      --text-secondary: #b67f8e;
      --btn-bg: #f7e4e2;
      --btn-border: #cfa5b0;
      --btn-active-bg: #b16f82;
      --btn-active-text: #fff;
      --joy-border: #b16f82;
      --stick-bg: #b16f82;
      --canvas-bg: #fef0ee;
    }
    /* 湖蓝（浅蓝） */
    body.theme-lakeblue {
      --bg-body: #e6f3ff;
      --card-bg: #ffffff;
      --card-border: #5c9ed6;
      --text-primary: #0d3b66;
      --text-secondary: #3e6b92;
      --btn-bg: #cce7ff;
      --btn-border: #5c9ed6;
      --btn-active-bg: #1f7ab0;
      --btn-active-text: #fff;
      --joy-border: #1f7ab0;
      --stick-bg: #1f7ab0;
      --canvas-bg: #e0f0ff;
    }
    /* 晴蓝（中蓝） */
    body.theme-sunnyblue {
      --bg-body: #eef5ff;
      --card-bg: #ffffff;
      --card-border: #7cb3e6;
      --text-primary: #1a3e6f;
      --text-secondary: #4a6a9e;
      --btn-bg: #cfe3ff;
      --btn-border: #7cb3e6;
      --btn-active-bg: #2b6bb0;
      --btn-active-text: #fff;
      --joy-border: #2b6bb0;
      --stick-bg: #2b6bb0;
      --canvas-bg: #e0f0ff;
    }
    /* 藏青（深蓝） */
    body.theme-navyblue {
      --bg-body: #f0f4fa;
      --card-bg: #ffffff;
      --card-border: #3c5a8c;
      --text-primary: #1e2f4a;
      --text-secondary: #4a6a8c;
      --btn-bg: #dce6f2;
      --btn-border: #3c5a8c;
      --btn-active-bg: #2c4a7c;
      --btn-active-text: #fff;
      --joy-border: #2c4a7c;
      --stick-bg: #2c4a7c;
      --canvas-bg: #eef2f8;
    }
    /* 葱绿（浅绿） */
    body.theme-oniongreen {
      --bg-body: #eef7e6;
      --card-bg: #ffffff;
      --card-border: #6f9e4f;
      --text-primary: #2c4c1e;
      --text-secondary: #5b8c3a;
      --btn-bg: #d9f0c5;
      --btn-border: #6f9e4f;
      --btn-active-bg: #4a7a2e;
      --btn-active-text: #fff;
      --joy-border: #4a7a2e;
      --stick-bg: #4a7a2e;
      --canvas-bg: #e8f5e0;
    }
    /* 甘青（青绿） */
    body.theme-ganqing {
      --bg-body: #eaf4f0;
      --card-bg: #ffffff;
      --card-border: #669d9b;
      --text-primary: #1f4e4b;
      --text-secondary: #53807c;
      --btn-bg: #d4eee9;
      --btn-border: #669d9b;
      --btn-active-bg: #2f7874;
      --btn-active-text: #fff;
      --joy-border: #2f7874;
      --stick-bg: #2f7874;
      --canvas-bg: #e0f3ef;
    }
    /* 翡翠（深绿） */
    body.theme-jade {
      --bg-body: #edf7ef;
      --card-bg: #ffffff;
      --card-border: #4c8b6e;
      --text-primary: #1f4d3a;
      --text-secondary: #4b7b64;
      --btn-bg: #daf1e3;
      --btn-border: #4c8b6e;
      --btn-active-bg: #317a5c;
      --btn-active-text: #fff;
      --joy-border: #317a5c;
      --stick-bg: #317a5c;
      --canvas-bg: #eaf7f0;
    }
    /* 松花（黄绿） */
    body.theme-pineflower {
      --bg-body: #f4fce8;
      --card-bg: #ffffff;
      --card-border: #9bbf6e;
      --text-primary: #3b6e1e;
      --text-secondary: #6d9c44;
      --btn-bg: #e2f5ce;
      --btn-border: #9bbf6e;
      --btn-active-bg: #5b8c32;
      --btn-active-text: #fff;
      --joy-border: #5b8c32;
      --stick-bg: #5b8c32;
      --canvas-bg: #f0fce4;
    }

    /* 全局变量覆写（使主题生效） */
    body {
      background-color: var(--bg-body);
      color: var(--text-primary);
    }
    .card {
      background-color: var(--card-bg);
      border-color: var(--card-border);
    }
    button, .modeBtn {
      background-color: var(--btn-bg);
      border-color: var(--btn-border);
      color: var(--text-primary);
    }
    .modeBtn.active {
      background: var(--btn-active-bg);
      color: var(--btn-active-text);
      border-color: var(--btn-active-bg);
    }
    .joy {
      border-color: var(--joy-border);
    }
    #attCanvas, canvas {
      background-color: var(--canvas-bg);
    }
    small, .muted, .joyLabel {
      color: var(--text-secondary);
    }
    input, select, textarea {
      background-color: var(--card-bg);
      color: var(--text-primary);
      border-color: var(--card-border);
    }

    /* ========== 状态消息醒目样式（用于遥控界面） ========== */
    .statusLabel {
      font-size: 1.1rem;
      background-color: #ffeb3b;
      padding: 4px 12px;
      border-radius: 20px;
      color: #000;
      font-weight: bold;
      display: inline-block;
    }
    .statusMsgContent {
      font-size: 1.2rem;
      font-weight: bold;
      color: #d32f2f;
      background-color: #fff9c4;
      padding: 6px 12px;
      border-radius: 8px;
      display: inline-block;
      word-break: break-word;
      flex: 1;
    }
    /* 适配暗色主题下状态消息背景 */
    body.theme-dark .statusLabel {
      background-color: #f9a825;
      color: #000;
    }
    body.theme-dark .statusMsgContent {
      background-color: #3e2723;
      color: #ffb74d;
    }

    /* ========== 设置页面短暂消息（actionStatus）样式增强 ========== */
    #actionStatus {
      font-size: 1.2rem;
      font-weight: bold;
      background-color: #ffffb3;
      padding: 4px 12px;
      border-radius: 20px;
      display: inline-block;
      color: #b45f06;
    }
    body.theme-dark #actionStatus {
      background-color: #5d4037;
      color: #ffcc80;
    }

    /* 主题网格样式 */
    .theme-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
      margin-top: 12px;
    }
    .theme-item {
      text-align: center;
      padding: 12px 6px;
      border-radius: 12px;
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      border: 1px solid rgba(0,0,0,0.2);
      transition: transform 0.1s;
    }
    .theme-item:active { transform: scale(0.96); }
  </style>
</head>
<body data-mode="control">
  <div id="modeBar" class="card">
    <button id="modeControl" class="modeBtn active">遥控</button>
    <button id="modeSettings" class="modeBtn">设置</button>
    <button id="modeMotor" class="modeBtn">测试</button>
    <button id="modeTheme" class="modeBtn">主题</button>
    <!-- 全屏切换按钮 -->
    <button id="fullscreenBtn" class="modeBtn" title="全屏模式">⛶</button>
  </div>

  <!-- 遥控视图 -->
  <div id="controlView">
    <div id="app">
      <div class="padCol">
        <div class="joy" id="leftJoy">
          <div class="joyLabel">Throttle油门/Yaw偏航</div>
          <div class="stick" id="leftStick"></div>
        </div>
      </div>

      <div id="centerCol">
        <div class="card" id="controlCard">
          <div class="row">
            <label><input id="armSwitch" type="checkbox"> ARM解锁</label>
            <button id="disarmBtn" class="danger">紧急上锁</button>
          </div>
          <div class="row" style="margin-top:6px;">
            <div>Throttle油门: <b id="thv">0.00</b></div>
            <div>WebSocket: <span id="ws">-</span></div>
            <div>飞行状态: <b id="state">-</b></div>
          </div>
          <!-- 状态消息行：显示操作反馈（如“起飞重复次数已保存”、“滤波器切换中...”等） -->
          <div class="row" style="margin-top:8px;">
            <b class="statusLabel">状态消息</b>
            <span id="statusMsg" class="statusMsgContent">-</span>
          </div>
          <small>左摇杆上下：底部=0，顶部=1（保持）。紧急上锁会立即停止电机</small>
        </div>

        <div class="card" id="telemetryCard">
          <div class="kv">
            <div>姿态 (deg)</div><div id="att">-</div>
            <div>dt / 频率</div><div id="dtHz">-</div>
            <div>指令延迟</div><div id="cmdAge">-</div>
            <div>电机输出</div><div id="mot">-</div>
            <div>电池电压</div><div id="vbatt">-</div>
            <div>故障原因</div><div id="fs">-</div>
          </div>
        </div>
      </div>

      <div class="padCol">
        <div class="joy" id="rightJoy">
          <div class="joyLabel">Pitch俯仰 / Roll横滚</div>
          <div class="stick" id="rightStick"></div>
        </div>
      </div>
    </div>
  </div>

  <!-- 设置视图 -->
  <div id="settingsView">
    <div id="settingsLayout">
      <div class="card" id="attCard">
        <div class="row">
          <b>姿态仪表</b>
          <span class="muted" id="attNums">-</span>
        </div>
        <canvas id="attCanvas"></canvas>
        <div class="row">
          <button id="levelBtn">水平校准</button>
          <button id="gyroCalBtn">陀螺仪校准</button>
          <span class="muted" id="calState">校准状态: -</span>
        </div>
        <small class="muted">请将飞机放在水平静止平面上操作。</small>
        <div class="kv">
          <div>横滚</div><div id="attRoll">-</div>
          <div>俯仰</div><div id="attPitch">-</div>
          <div>偏航</div><div id="attYaw">-</div>
        </div>
      </div>

      <div class="card" id="tuneCard">
        <!-- 蜂鸣器设置区域 -->
        <div class="section">
          <b>蜂鸣器设置</b>
          <div class="row">
            <label><input type="checkbox" id="buzzerMuteCheckbox"> 全局静音</label>
          </div>
          <div class="row">
            <span>起飞音效重复次数:</span>
            <input type="range" id="takeoffRepeatSlider" min="1" max="10" step="1" value="3">
            <span id="takeoffRepeatValue">3</span>
            <button id="applyTakeoffRepeat">应用</button>
          </div>
          <small class="muted">静音开关将关闭所有蜂鸣器声音。起飞音效重复次数范围1~10，保存后下次起飞生效。</small>
        </div>

        <!-- IMU 滤波器切换控件 -->
        <div class="section">
          <b>姿态滤波器</b>
          <div class="row">
            <select id="imuFilterSelect">
              <option value="0">Madgwick (梯度下降)</option>
              <option value="1" selected>Mahony (比例积分)</option>
              <option value="2">互补滤波 (简单加权)</option>
            </select>
            <button id="applyFilterBtn">应用</button>
          </div>
          <small class="muted">仅可在上锁状态下切换，切换后自动保存到飞控，下次上电仍生效。</small>
        </div>

        <div class="row">
          <button id="syncBtn">获取参数</button>
          <button id="saveBtn">保存参数</button>
          <span class="muted">PID + 参数</span>
          <span id="actionStatus" class="muted">-</span>
        </div>

        <!-- 注意：状态消息已移至遥控界面，此处不再显示重复 -->

        <div class="section">
          <b>角度 PID (横滚/俯仰)</b>
          <div class="row">
            Kp <input id="aKp" type="number" step="0.001" inputmode="decimal">
            Ki <input id="aKi" type="number" step="0.001" inputmode="decimal">
            Kd <input id="aKd" type="number" step="0.001" inputmode="decimal">
            <button id="applyAngle">应用</button>
          </div>
        </div>

        <div class="section">
          <b>角速率 PID (横滚/俯仰)</b>
          <div class="row">
            Kp <input id="rKp" type="number" step="0.0001" inputmode="decimal">
            Ki <input id="rKi" type="number" step="0.0001" inputmode="decimal">
            Kd <input id="rKd" type="number" step="0.0001" inputmode="decimal">
            <button id="applyRate">应用</button>
          </div>
        </div>

        <div class="section">
          <b>偏航角速率 PID</b>
          <div class="row">
            Kp <input id="yKp" type="number" step="0.0001" inputmode="decimal">
            Ki <input id="yKi" type="number" step="0.0001" inputmode="decimal">
            Kd <input id="yKd" type="number" step="0.0001" inputmode="decimal">
            <button id="applyYaw">应用</button>
          </div>
        </div>

        <div class="section">
          <b>限制参数</b>
          <div class="row">
            最大角度 <input id="maxAngle" type="number" step="1" inputmode="decimal">
            最大偏航速率 <input id="maxYawRate" type="number" step="1" inputmode="decimal">
          </div>
          <div class="row">
            倾斜上锁角度 <input id="tiltDisarm" type="number" step="1" inputmode="decimal">
            指令超时 <input id="cmdTimeout" type="number" step="10" inputmode="decimal">
          </div>
          <div class="row">
            最小扭矩系数 <input id="torqueMin" type="number" step="0.01" min="0" max="1" inputmode="decimal">
            扭矩系数斜率 <input id="torqueSlope" type="number" step="0.1" min="0" max="4" inputmode="decimal">
          </div>
          <div class="row">
            遥测周期(ms)  <input id="telemMs" type="number" step="10" inputmode="decimal">
            <button id="applyLimits">应用限制</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- 电机测试视图 -->
  <div id="motorTestView">
    <div id="motorTestLayout">
      <div class="card" id="motorTestCard">
        <div class="row">
          <b>电机测试</b>
          <span class="muted">Props off</span>
        </div>
        <div class="row">
          油门 <input id="motorTestThrottle" type="number" step="0.01" min="0" max="0.25" inputmode="decimal">
          持续时间 <input id="motorTestMs" type="number" step="50" min="50" max="2000" inputmode="decimal"> ms
          <button id="motorTestStop" class="danger">停止</button>
        </div>
        <div class="row">
          <button id="motorTest0">M4 左前</button>
          <button id="motorTest1">M1 右前</button>
          <button id="motorTest2">M2 右后</button>
          <button id="motorTest3">M3 左后</button>
        </div>
        <small class="muted">X 配置：机头方向位于 M4 和 M1 之间。</small>
      </div>
    </div>
  </div>

  <!-- 主题视图 -->
  <div id="themeView">
    <div id="themeLayout">
      <div class="card" id="themeCard">
        <div class="row">
          <b>选择主题</b>
        </div>
        <div id="themeGrid" class="theme-grid"></div>
        <small class="muted">点击色块切换主题，主题会自动保存。</small>
      </div>
    </div>
  </div>

<script>
(() => {
  // ========== 全屏功能 ==========
  const fullscreenBtn = document.getElementById('fullscreenBtn');
  function updateFullscreenIcon() {
    const isFullscreen = !!document.fullscreenElement;
    if (fullscreenBtn) {
      fullscreenBtn.textContent = isFullscreen ? '⤬' : '⛶';
      fullscreenBtn.title = isFullscreen ? '退出全屏' : '全屏模式';
    }
  }
  function toggleFullscreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(err => {
        console.warn(`全屏请求失败: ${err.message}`);
      });
    } else {
      document.exitFullscreen();
    }
  }
  if (fullscreenBtn) {
    fullscreenBtn.addEventListener('click', toggleFullscreen);
    document.addEventListener('fullscreenchange', updateFullscreenIcon);
    updateFullscreenIcon();
  }

  // ========== 公共提示音函数（Web Audio） ==========
  let audioCtx = null;  // 延迟初始化，仅在首次点击时创建

  function playBeep() {
    try {
      if (!audioCtx) {
        audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      }
      if (audioCtx.state === 'suspended') {
        audioCtx.resume().then(() => {
          beepNow();
        }).catch(e => console.log("AudioContext resume failed", e));
      } else {
        beepNow();
      }
    } catch (e) {
      console.log("Web Audio not supported", e);
    }
  }

  function beepNow() {
    if (!audioCtx) return;
    const now = audioCtx.currentTime;
    const oscillator = audioCtx.createOscillator();
    const gainNode = audioCtx.createGain();
    oscillator.connect(gainNode);
    gainNode.connect(audioCtx.destination);
    oscillator.frequency.value = 1200;   // 1200Hz
    gainNode.gain.value = 0.3;           // 音量 30%
    oscillator.start();
    gainNode.gain.exponentialRampToValueAtTime(0.00001, now + 0.1);
    oscillator.stop(now + 0.1);
  }

  // ========== DOM 元素绑定 ==========
  const wsLabel = document.getElementById('ws');
  const stateLabel = document.getElementById('state');
  const attLabel = document.getElementById('att');
  const dtHzLabel = document.getElementById('dtHz');
  const cmdAgeLabel = document.getElementById('cmdAge');
  const motLabel = document.getElementById('mot');
  const vbattLabel = document.getElementById('vbatt');
  const fsLabel = document.getElementById('fs');

  const attNums = document.getElementById('attNums');
  const attRollLabel = document.getElementById('attRoll');
  const attPitchLabel = document.getElementById('attPitch');
  const attYawLabel = document.getElementById('attYaw');
  const attCanvas = document.getElementById('attCanvas');
  const attCtx = attCanvas ? attCanvas.getContext('2d') : null;
  const levelBtn = document.getElementById('levelBtn');
  const gyroCalBtn = document.getElementById('gyroCalBtn');
  const calState = document.getElementById('calState');
  const actionStatus = document.getElementById('actionStatus'); // 设置页面短暂消息
  const statusMsg = document.getElementById('statusMsg');       // 遥控界面状态消息

  const modeControl = document.getElementById('modeControl');
  const modeSettings = document.getElementById('modeSettings');
  const modeMotor = document.getElementById('modeMotor');
  const modeTheme = document.getElementById('modeTheme');

  const armSwitch = document.getElementById('armSwitch');
  const disarmBtn = document.getElementById('disarmBtn');
  const thv = document.getElementById('thv');

  const syncBtn = document.getElementById('syncBtn');
  const saveBtn = document.getElementById('saveBtn');

  const aKp = document.getElementById('aKp');
  const aKi = document.getElementById('aKi');
  const aKd = document.getElementById('aKd');
  const rKp = document.getElementById('rKp');
  const rKi = document.getElementById('rKi');
  const rKd = document.getElementById('rKd');
  const yKp = document.getElementById('yKp');
  const yKi = document.getElementById('yKi');
  const yKd = document.getElementById('yKd');

  const applyAngle = document.getElementById('applyAngle');
  const applyRate = document.getElementById('applyRate');
  const applyYaw = document.getElementById('applyYaw');

  const maxAngle = document.getElementById('maxAngle');
  const maxYawRate = document.getElementById('maxYawRate');
  const tiltDisarm = document.getElementById('tiltDisarm');
  const cmdTimeout = document.getElementById('cmdTimeout');
  const telemMs = document.getElementById('telemMs');
  const torqueMin = document.getElementById('torqueMin');
  const torqueSlope = document.getElementById('torqueSlope');
  const applyLimits = document.getElementById('applyLimits');

  const leftJoy = document.getElementById('leftJoy');
  const leftStick = document.getElementById('leftStick');
  const rightJoy = document.getElementById('rightJoy');
  const rightStick = document.getElementById('rightStick');

  const motorTestThrottle = document.getElementById('motorTestThrottle');
  const motorTestMs = document.getElementById('motorTestMs');
  const motorTestStop = document.getElementById('motorTestStop');
  const motorTest0 = document.getElementById('motorTest0');
  const motorTest1 = document.getElementById('motorTest1');
  const motorTest2 = document.getElementById('motorTest2');
  const motorTest3 = document.getElementById('motorTest3');

  // 蜂鸣器相关控件
  const buzzerMuteCheckbox = document.getElementById('buzzerMuteCheckbox');
  const takeoffRepeatSlider = document.getElementById('takeoffRepeatSlider');
  const takeoffRepeatValue = document.getElementById('takeoffRepeatValue');
  const applyTakeoffRepeat = document.getElementById('applyTakeoffRepeat');

  // IMU 滤波器控件
  const imuFilterSelect = document.getElementById('imuFilterSelect');
  const applyFilterBtn = document.getElementById('applyFilterBtn');

  // 主题网格容器
  const themeGrid = document.getElementById('themeGrid');

  let ws = null;

  let roll = 0;
  let pitch = 0;
  let yaw = 0;
  let thr = 0;
  let armReq = 0;

  let lastAtt = { roll: 0, pitch: 0, yaw: 0 };
  let statusTimer = null;
  let actionTimer = null;  // 用于设置页面短暂消息
  const refreshJoysticks = [];

  const fsMap = {
    0: 'NONE无故障',
    1: '指令超时',
    2: 'WebSocket断开',
    3: 'IMU失效',
    4: '倾斜过大',
    5: '循环超时',
    6: '紧急停止',
    7: '手动上锁',
    8: '电机故障'
  };

  function clamp(v, vmin, vmax) {
    return Math.max(vmin, Math.min(vmax, v));
  }

  // 设置遥控界面状态消息（醒目样式）
  function setStatus(text, holdMs = 2000) {
    if (!statusMsg) return;
    statusMsg.textContent = text;
    if (statusTimer) {
      clearTimeout(statusTimer);
      statusTimer = null;
    }
    if (holdMs > 0) {
      statusTimer = setTimeout(() => {
        statusMsg.textContent = '-';
        statusTimer = null;
      }, holdMs);
    }
  }

  // 设置设置页面短暂消息（原有 actionStatus，增强样式）
  function setActionStatus(text, holdMs = 2000) {
    if (!actionStatus) return;
    actionStatus.textContent = text;
    if (actionTimer) {
      clearTimeout(actionTimer);
      actionTimer = null;
    }
    if (holdMs > 0) {
      actionTimer = setTimeout(() => {
        actionStatus.textContent = '-';
        actionTimer = null;
      }, holdMs);
    }
  }

  function joyMaxR(areaEl) {
    return Math.min(areaEl.clientWidth, areaEl.clientHeight) * 0.35;
  }

  function setStickNorm(areaEl, stickEl, nx, ny) {
    const mr = joyMaxR(areaEl);
    stickEl.style.transform =
      `translate(calc(-50% + ${nx * mr}px), calc(-50% + ${ny * mr}px))`;
  }

  function nyToThrottle(ny) {
    return clamp((1 - ny) / 2, 0, 1);
  }

  function throttleToNy(t) {
    const tt = clamp(t, 0, 1);
    return 1 - 2 * tt;
  }

  function updateThrottleUi() {
    if (thv) thv.textContent = thr.toFixed(2);
  }

  function setMode(mode) {
    document.body.dataset.mode = mode;
    if (modeControl) modeControl.classList.toggle('active', mode === 'control');
    if (modeSettings) modeSettings.classList.toggle('active', mode === 'settings');
    if (modeMotor) modeMotor.classList.toggle('active', mode === 'motor');
    if (modeTheme) modeTheme.classList.toggle('active', mode === 'theme');
    if (mode !== 'motor') {
      sendMotorTestStop();
    }
    if (mode === 'settings' || mode === 'motor' || mode === 'theme') {
      armReq = 0;
      if (armSwitch) armSwitch.checked = false;
      thr = 0;
      yaw = 0;
      roll = 0;
      pitch = 0;
      updateThrottleUi();
      if (leftJoy && leftStick) {
        setStickNorm(leftJoy, leftStick, 0, throttleToNy(thr));
      }
      if (rightJoy && rightStick) {
        setStickNorm(rightJoy, rightStick, 0, 0);
      }
      requestAnimationFrame(() => {
        drawAttitude(lastAtt.roll, lastAtt.pitch, lastAtt.yaw);
      });
    }
  }

  function resizeAttCanvas() {
    if (!attCanvas || !attCtx) return;
    const rect = attCanvas.getBoundingClientRect();
    if (rect.width < 10 || rect.height < 10) return;
    const dpr = window.devicePixelRatio || 1;
    const w = Math.max(1, Math.round(rect.width * dpr));
    const h = Math.max(1, Math.round(rect.height * dpr));
    if (attCanvas.width !== w || attCanvas.height !== h) {
      attCanvas.width = w;
      attCanvas.height = h;
    }
  }

  function drawAttitude(rollDeg, pitchDeg, yawDeg) {
    lastAtt = { roll: rollDeg, pitch: pitchDeg, yaw: yawDeg };
    if (!attCanvas || !attCtx) return;
    const rect = attCanvas.getBoundingClientRect();
    if (rect.width < 10 || rect.height < 10) return;
    resizeAttCanvas();

    const w = attCanvas.width;
    const h = attCanvas.height;
    const dpr = window.devicePixelRatio || 1;
    const ctx = attCtx;
    ctx.clearRect(0, 0, w, h);

    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.45;
    const rollRad = rollDeg * Math.PI / 180;
    const pitch = clamp(pitchDeg, -45, 45);
    const offset = (pitch / 45) * radius;

    ctx.save();
    ctx.translate(cx, cy);
    ctx.beginPath();
    ctx.arc(0, 0, radius, 0, Math.PI * 2);
    ctx.clip();
    ctx.rotate(-rollRad);
    ctx.fillStyle = '#cfe8ff';
    ctx.fillRect(-radius * 2, -radius * 2 + offset, radius * 4, radius * 2);
    ctx.fillStyle = '#d2a679';
    ctx.fillRect(-radius * 2, offset, radius * 4, radius * 2);
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 2 * dpr;
    ctx.beginPath();
    ctx.moveTo(-radius * 2, offset);
    ctx.lineTo(radius * 2, offset);
    ctx.stroke();
    ctx.restore();

    ctx.strokeStyle = '#333';
    ctx.lineWidth = 2 * dpr;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(cx - 10 * dpr, cy);
    ctx.lineTo(cx + 10 * dpr, cy);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(cx, cy - 10 * dpr);
    ctx.lineTo(cx, cy + 10 * dpr);
    ctx.stroke();

    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(-yawDeg * Math.PI / 180);
    ctx.fillStyle = '#333';
    ctx.beginPath();
    ctx.moveTo(0, -radius);
    ctx.lineTo(-6 * dpr, -radius + 12 * dpr);
    ctx.lineTo(6 * dpr, -radius + 12 * dpr);
    ctx.closePath();
    ctx.fill();
    ctx.restore();
  }

  // 主题定义
  const themes = [
    { name: '浅色', id: 'light', bg: '#ffffff', text: '#000000' },
    { name: '暗黑', id: 'dark', bg: '#121212', text: '#e0e0e0' },
    { name: '海棠', id: 'crabapple', bg: '#e69b8c', text: '#a63e2c' },
    { name: '朱红', id: 'vermilion', bg: '#a15028', text: '#fff0e8' },
    { name: '藕荷', id: 'lotusroot', bg: '#cfa5b0', text: '#7a4c5c' },
    { name: '湖蓝', id: 'lakeblue', bg: '#5c9ed6', text: '#0d3b66' },
    { name: '晴蓝', id: 'sunnyblue', bg: '#7cb3e6', text: '#1a3e6f' },
    { name: '藏青', id: 'navyblue', bg: '#3c5a8c', text: '#1e2f4a' },
    { name: '葱绿', id: 'oniongreen', bg: '#6f9e4f', text: '#2c4c1e' },
    { name: '甘青', id: 'ganqing', bg: '#669d9b', text: '#1f4e4b' },
    { name: '翡翠', id: 'jade', bg: '#4c8b6e', text: '#1f4d3a' },
    { name: '松花', id: 'pineflower', bg: '#9bbf6e', text: '#3b6e1e' }
  ];

  function buildThemeGrid() {
    if (!themeGrid) return;
    themeGrid.innerHTML = '';
    themes.forEach(theme => {
      const div = document.createElement('div');
      div.className = 'theme-item';
      div.textContent = theme.name;
      div.style.backgroundColor = theme.bg;
      div.style.color = theme.text;
      div.addEventListener('click', () => {
        applyTheme(theme.id);
      });
      themeGrid.appendChild(div);
    });
  }

  function applyTheme(themeId) {
    const themeClasses = themes.map(t => t.id).filter(id => id !== 'light');
    document.body.classList.remove(...themeClasses.map(id => `theme-${id}`));
    if (themeId !== 'light') {
      document.body.classList.add(`theme-${themeId}`);
    }
    localStorage.setItem('drone_theme', themeId);
  }

  function connectWs() {
    ws = new WebSocket(`ws://${location.host}/ws`);
    ws.onopen = () => {
      wsLabel.textContent = '已连接';
      if (ws && ws.readyState === 1) ws.send('GET');
    };
    ws.onclose = () => { wsLabel.textContent = '已断开'; setTimeout(connectWs, 500); };
    ws.onerror = () => { wsLabel.textContent = 'ERR错误'; };
    ws.onmessage = (ev) => {
      const s = String(ev.data || '');
      try {
        const obj = JSON.parse(s);
        if (obj.type === 'tel') {
          const rollDeg = Number(obj.roll);
          const pitchDeg = Number(obj.pitch);
          const yawDeg = Number(obj.yaw);
          const rollTxt = Number.isFinite(rollDeg) ? rollDeg.toFixed(1) : '-';
          const pitchTxt = Number.isFinite(pitchDeg) ? pitchDeg.toFixed(1) : '-';
          const yawTxt = Number.isFinite(yawDeg) ? yawDeg.toFixed(1) : '-';

          stateLabel.textContent = obj.armed ? '已解锁' : '已上锁';
          attLabel.textContent = `${rollTxt}, ${pitchTxt}, ${yawTxt}`;
          if (attNums) attNums.textContent = `R ${rollTxt} P ${pitchTxt} Y ${yawTxt}`;
          if (attRollLabel) attRollLabel.textContent = rollTxt;
          if (attPitchLabel) attPitchLabel.textContent = pitchTxt;
          if (attYawLabel) attYawLabel.textContent = yawTxt;
          if (calState && obj.cal) calState.textContent = `校准状态:  ${obj.cal}`;

          dtHzLabel.textContent = `${obj.dt_ms.toFixed(2)} 毫秒 / ${obj.loop_hz.toFixed(0)} Hz`;
          cmdAgeLabel.textContent = `${obj.cmd_age} 毫秒`;
          motLabel.textContent = `${obj.m0.toFixed(2)} ${obj.m1.toFixed(2)} ${obj.m2.toFixed(2)} ${obj.m3.toFixed(2)}`;
          vbattLabel.textContent = (Number.isFinite(obj.vbatt) ? `${obj.vbatt.toFixed(2)} V` : '-');
          if (vbattLabel && Number.isFinite(obj.vbatt)) {
              const v = obj.vbatt;
              if (v < 3.3) {
                  vbattLabel.style.color = 'red';
              } else if (v < 3.5) {
                  vbattLabel.style.color = 'orange';
              } else if (v < 3.7) {
                  vbattLabel.style.color = 'goldenrod';
              } else {
                  vbattLabel.style.color = '';
              }
          } else if (vbattLabel) {
              vbattLabel.style.color = '';
          }
          fsLabel.textContent = fsMap[obj.fs] || String(obj.fs);
          if (statusMsg) statusMsg.textContent = obj.msg ? String(obj.msg) : '-';

          if (Number.isFinite(rollDeg) && Number.isFinite(pitchDeg) && Number.isFinite(yawDeg)) {
            drawAttitude(rollDeg, pitchDeg, yawDeg);
          }
        } else if (obj.type === 'ack') {
          const op = String(obj.op || '');
          const tag = obj.tag ? String(obj.tag) : '';
          const ok = obj.ok ? true : false;
          const msg = obj.msg ? String(obj.msg) : '';
          let text = op || 'ACK';
          if (tag) text += ` ${tag}`;
          text += ok ? ' 成功' : ' 失败';
          if (msg) text += ` (${msg})`;
          setStatus(text, ok ? 2000 : 3000);
          setActionStatus(text, ok ? 2000 : 3000); // 同时显示在设置页面
        } else if (obj.type === 'cfg') {
          // 更新 PID 和限制参数
          if (aKp) aKp.value = obj.angle.kp;
          if (aKi) aKi.value = obj.angle.ki;
          if (aKd) aKd.value = obj.angle.kd;
          if (rKp) rKp.value = obj.rate.kp;
          if (rKi) rKi.value = obj.rate.ki;
          if (rKd) rKd.value = obj.rate.kd;
          if (yKp) yKp.value = obj.yaw.kp;
          if (yKi) yKi.value = obj.yaw.ki;
          if (yKd) yKd.value = obj.yaw.kd;
          if (obj.limits) {
            if (maxAngle) maxAngle.value = obj.limits.max_angle;
            if (maxYawRate) maxYawRate.value = obj.limits.max_yaw_rate;
            if (tiltDisarm) tiltDisarm.value = obj.limits.tilt_disarm;
            if (cmdTimeout) cmdTimeout.value = obj.limits.cmd_timeout;
            if (telemMs) telemMs.value = obj.limits.telem_ms;
            if (torqueMin) torqueMin.value = obj.limits.torque_min;
            if (torqueSlope) torqueSlope.value = obj.limits.torque_slope;
          }
          // 更新蜂鸣器设置
          if (buzzerMuteCheckbox && obj.buzzer_mute !== undefined) {
            buzzerMuteCheckbox.checked = obj.buzzer_mute === 1;
          }
          if (takeoffRepeatSlider && takeoffRepeatValue && obj.takeoff_repeat !== undefined) {
            takeoffRepeatSlider.value = obj.takeoff_repeat;
            takeoffRepeatValue.textContent = obj.takeoff_repeat;
          }
          // 更新 IMU 滤波器下拉框
          if (imuFilterSelect && obj.imu_filter !== undefined) {
            imuFilterSelect.value = obj.imu_filter;
          }
          setStatus('配置已更新', 2000);
          setActionStatus('配置已更新', 2000);
        }
      } catch (e) {
        // ignore
      }
    };
  }

  // 视图切换事件绑定
  if (modeControl) modeControl.addEventListener('click', () => setMode('control'));
  if (modeSettings) modeSettings.addEventListener('click', () => setMode('settings'));
  if (modeMotor) modeMotor.addEventListener('click', () => setMode('motor'));
  if (modeTheme) modeTheme.addEventListener('click', () => setMode('theme'));

  if (armSwitch) {
    armSwitch.addEventListener('change', () => {
      armReq = armSwitch.checked ? 1 : 0;
    });
  }

  if (disarmBtn) {
    disarmBtn.addEventListener('click', () => {
      armReq = 0;
      if (armSwitch) armSwitch.checked = false;

      thr = 0;
      yaw = 0;
      roll = 0;
      pitch = 0;

      updateThrottleUi();
      if (leftJoy && leftStick) {
        setStickNorm(leftJoy, leftStick, 0, throttleToNy(thr));
      }
      if (rightJoy && rightStick) {
        setStickNorm(rightJoy, rightStick, 0, 0);
      }

      if (ws && ws.readyState === 1) {
        ws.send('K');
      }
    });
  }

  function sendWs(text, statusText) {
    if (!ws || ws.readyState !== 1) {
      setStatus('WebSocket未连接', 2000);
      setActionStatus('WebSocket未连接', 2000);
      return false;
    }
    if (statusText) {
      setStatus(statusText, 1500);
      setActionStatus(statusText, 1500);
    }
    ws.send(text);
    return true;
  }

  function sendMotorTestStop() {
    if (!ws || ws.readyState !== 1) return;
    ws.send('MTEST,STOP');
  }

  function sendPid(tag, kp, ki, kd) {
    if (!ws || ws.readyState !== 1) return;
    ws.send(`PID,${tag},${kp},${ki},${kd}`);
  }

  // ========== 所有“应用”按钮添加蜂鸣提示 ==========
  if (applyAngle) {
    applyAngle.addEventListener('click', () => {
      playBeep();
      sendPid('ANGLE', Number(aKp.value), Number(aKi.value), Number(aKd.value));
    });
  }
  if (applyRate) {
    applyRate.addEventListener('click', () => {
      playBeep();
      sendPid('RATE', Number(rKp.value), Number(rKi.value), Number(rKd.value));
    });
  }
  if (applyYaw) {
    applyYaw.addEventListener('click', () => {
      playBeep();
      sendPid('YAW', Number(yKp.value), Number(yKi.value), Number(yKd.value));
    });
  }
  if (applyLimits) {
    applyLimits.addEventListener('click', () => {
      playBeep();
      const maxAngleVal = readNumberInput(maxAngle);
      const maxYawVal = readNumberInput(maxYawRate);
      const tiltVal = readNumberInput(tiltDisarm);
      const cmdVal = readNumberInput(cmdTimeout);
      const telemVal = readNumberInput(telemMs);
      const torqueMinVal = readNumberInput(torqueMin);
      const torqueSlopeVal = readNumberInput(torqueSlope);

      if (maxAngleVal !== null) sendCfg('MAX_ANGLE', maxAngleVal);
      if (maxYawVal !== null) sendCfg('MAX_YAW_RATE', maxYawVal);
      if (tiltVal !== null) sendCfg('TILT_DISARM', tiltVal);
      if (cmdVal !== null) sendCfg('CMD_TIMEOUT', Math.round(cmdVal));
      if (telemVal !== null) sendCfg('TELEM_MS', Math.round(telemVal));
      if (torqueMinVal !== null) sendCfg('TORQUE_MIN', torqueMinVal);
      if (torqueSlopeVal !== null) sendCfg('TORQUE_SLOPE', torqueSlopeVal);
    });
  }
  if (applyTakeoffRepeat) {
    applyTakeoffRepeat.addEventListener('click', () => {
      playBeep();
      const val = parseInt(takeoffRepeatSlider.value, 10);
      if (!isNaN(val) && val >= 1 && val <= 10) {
        sendWs(`TAKEOFF_REPEAT,${val}`, `起飞重复次数设为 ${val}`);
      } else {
        setStatus('无效数值', 2000);
        setActionStatus('无效数值', 2000);
      }
    });
  }
  if (applyFilterBtn) {
    applyFilterBtn.addEventListener('click', () => {
      playBeep();
      const filterValue = imuFilterSelect ? imuFilterSelect.value : '1';
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(`CFG,IMU_FILTER,${filterValue}`);
        setStatus(`正在切换滤波器模式到 ${imuFilterSelect.options[imuFilterSelect.selectedIndex].text}...`, 1500);
        setActionStatus(`正在切换滤波器模式到 ${imuFilterSelect.options[imuFilterSelect.selectedIndex].text}...`, 1500);
      } else {
        setStatus('WebSocket未连接，无法切换', 2000);
        setActionStatus('WebSocket未连接，无法切换', 2000);
      }
    });
  }
  // ================================================

  if (syncBtn) {
    syncBtn.addEventListener('click', () => {
      sendWs('GET', '已发送获取参数');
    });
  }
  if (saveBtn) {
    saveBtn.addEventListener('click', () => {
      sendWs('SAVE', '已发送保存参数');
    });
  }

  if (levelBtn) {
    levelBtn.addEventListener('click', () => {
      sendWs('CAL,LEVEL', '已发送水平校准');
    });
  }
  if (gyroCalBtn) {
    gyroCalBtn.addEventListener('click', () => {
      sendWs('CAL,GYRO', '已发送陀螺仪校准');
    });
  }

  function sendCfg(key, value) {
    if (!ws || ws.readyState !== 1) return;
    ws.send(`CFG,${key},${value}`);
  }

  function readNumberInput(input) {
    if (!input) return null;
    const s = String(input.value || '').trim();
    if (s === '') return null;
    const v = Number(s);
    return Number.isFinite(v) ? v : null;
  }

  function sendMotorTest(idx) {
    const thrVal = readNumberInput(motorTestThrottle);
    const msVal = readNumberInput(motorTestMs);
    const thr = thrVal !== null ? thrVal : 0.12;
    const ms = msVal !== null ? Math.round(msVal) : 600;
    const thrClamped = clamp(thr, 0, 0.25);
    const msClamped = clamp(ms, 50, 2000);
    sendWs(`MTEST,${idx},${thrClamped.toFixed(3)},${msClamped}`, `MTEST M${idx} sent`);
  }

  if (motorTestStop) {
    motorTestStop.addEventListener('click', () => {
      sendMotorTestStop();
      setStatus('电机测试已停止', 1500);
      setActionStatus('电机测试已停止', 1500);
    });
  }
  if (motorTest0) {
    motorTest0.addEventListener('click', () => sendMotorTest(0));
  }
  if (motorTest1) {
    motorTest1.addEventListener('click', () => sendMotorTest(1));
  }
  if (motorTest2) {
    motorTest2.addEventListener('click', () => sendMotorTest(2));
  }
  if (motorTest3) {
    motorTest3.addEventListener('click', () => sendMotorTest(3));
  }

  function makeJoystick(areaEl, stickEl, cfg) {
    const maxR = () => joyMaxR(areaEl);
    let activeId = null;
    let origin = null;
    let curNx = cfg.initialNx || 0;
    let curNy = cfg.initialNy || 0;

    function setStick(nx, ny) {
      curNx = nx;
      curNy = ny;
      setStickNorm(areaEl, stickEl, nx, ny);
    }

    requestAnimationFrame(() => {
      setStick(curNx, curNy);
    });

    const refresh = () => setStick(curNx, curNy);
    refreshJoysticks.push(refresh);

    areaEl.addEventListener('pointerdown', (e) => {
      activeId = e.pointerId;
      areaEl.setPointerCapture(activeId);

      const rect = areaEl.getBoundingClientRect();
      origin = { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2 };

      const mr = maxR();
      const nx = clamp((e.clientX - origin.x) / mr, -1, 1);
      const ny = clamp((e.clientY - origin.y) / mr, -1, 1);
      setStick(nx, ny);
      cfg.onMove(nx, ny);
    });

    areaEl.addEventListener('pointermove', (e) => {
      if (activeId === null || e.pointerId !== activeId || !origin) return;
      const mr = maxR();
      const nx = clamp((e.clientX - origin.x) / mr, -1, 1);
      const ny = clamp((e.clientY - origin.y) / mr, -1, 1);
      setStick(nx, ny);
      cfg.onMove(nx, ny);
    });

    function release(e) {
      if (activeId === null || e.pointerId !== activeId) return;
      activeId = null;
      origin = null;

      const nx = cfg.centerX ? 0 : curNx;
      const ny = cfg.centerY ? 0 : curNy;
      setStick(nx, ny);

      if (cfg.onRelease) {
        cfg.onRelease(nx, ny);
      }
    }

    areaEl.addEventListener('pointerup', release);
    areaEl.addEventListener('pointercancel', release);
  }

  // 左摇杆：X=偏航（回中），Y=油门（不回中，底部0顶部1）
  makeJoystick(leftJoy, leftStick, {
    centerX: true,
    centerY: false,
    initialNx: 0,
    initialNy: 1,
    onMove: (nx, ny) => {
      yaw = clamp(-nx, -1, 1);
      thr = nyToThrottle(ny);
      updateThrottleUi();
    },
    onRelease: (nx, ny) => {
      yaw = 0;
      thr = nyToThrottle(ny);
      updateThrottleUi();
    }
  });

  // 右摇杆：X=横滚，Y=俯仰（均回中）
  makeJoystick(rightJoy, rightStick, {
    centerX: true,
    centerY: true,
    initialNx: 0,
    initialNy: 0,
    onMove: (nx, ny) => {
      roll = clamp(nx, -1, 1);
      pitch = clamp(ny, -1, 1);
    },
    onRelease: () => {
      roll = 0;
      pitch = 0;
    }
  });

  setInterval(() => {
    if (!ws || ws.readyState !== 1) return;
    ws.send(`C,${thr.toFixed(3)},${roll.toFixed(3)},${pitch.toFixed(3)},${yaw.toFixed(3)},${armReq}`);
  }, 20);

  window.addEventListener('resize', () => {
    resizeAttCanvas();
    drawAttitude(lastAtt.roll, lastAtt.pitch, lastAtt.yaw);
    refreshJoysticks.forEach((fn) => fn());
  });

  // 蜂鸣器静音开关（不播放提示音，仅发送命令）
  if (buzzerMuteCheckbox) {
    buzzerMuteCheckbox.addEventListener('change', () => {
      const mute = buzzerMuteCheckbox.checked ? 1 : 0;
      sendWs(`BUZZER_MUTE,${mute}`, '设置静音');
    });
  }

  // 起飞音效次数滑动条显示实时值
  if (takeoffRepeatSlider && takeoffRepeatValue) {
    takeoffRepeatSlider.addEventListener('input', () => {
      const val = parseInt(takeoffRepeatSlider.value, 10);
      takeoffRepeatValue.textContent = val;
    });
  }

  // 初始化主题网格和已保存的主题
  buildThemeGrid();
  const savedTheme = localStorage.getItem('drone_theme');
  if (savedTheme && savedTheme !== 'light') {
    applyTheme(savedTheme);
  } else {
    applyTheme('light');
  }

  thr = 0;
  updateThrottleUi();
  if (motorTestThrottle) motorTestThrottle.value = 0.12;
  if (motorTestMs) motorTestMs.value = 600;
  setMode('control');
  connectWs();
})();
</script>
</body>
</html>
)HTML";
