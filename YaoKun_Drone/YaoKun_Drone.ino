/**
 * @file YaoKun_Drone.ino
 * @brief 耀坤飞控主程序 -主入口
 * @version 1.0 (20260613)
 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 *
 * 本文件包含：系统初始化、主循环、WiFi/Web服务器启动、姿态控制循环(controlStep)、
 * 电压监测与告警、校准辅助函数（状态管理）、电池电压读取等。
 * 其他功能（遥控指令、WebSocket命令、PID参数存储、故障保护、遥测发送等）已拆分至独立模块。
 * 
 * 编译前请确保以下文件存在且正确：
 *   - drone_types.h     (类型、枚举、跨模块声明)
 *   - config.h          (硬件引脚和系统常量)
 *   - drone_led.h       (LED 驱动头文件)
 *   - drone_buzzer.h    (蜂鸣器驱动头文件)
 *   - drone_imu.h       (IMU 驱动类声明)
 *   - drone_pid.h       (PID 类)
 *   - hardware.ino      (硬件实现)
 *   - storage.ino       (参数存储模块)
 *   - control.ino       (飞控控制算法)
 *   - failsafe.ino      (故障保护与解锁状态机)
 *   - telemetry.ino     (遥测数据发送模块)
 *   - rc.ino            (遥控指令存储与WebSocket 命令处理)  
 *   - drone_web_ui.h    (网页界面)
 */

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <math.h>
#include <string.h>
#include <esp32-hal-ledc.h>
#include "drone_types.h"
#include "config.h"

// ============================================================================
// 包含其他模块头文件
// ============================================================================
#include "drone_led.h"       // LED 驱动（红绿蓝）
#include "drone_buzzer.h"    // 蜂鸣器驱动
#include "drone_pid.h"       // PID 控制器类
#include "drone_imu.h"       // IMU 驱动类声明
#include "drone_web_ui.h"    // 静态网页

// ============================================================================
// RC 遥控指令存储（临界区保护）
// ============================================================================
bool g_websocket_connected = false;   // WebSocket 连接标志（控制蓝色LED）

// ============================================================================
// 全局变量（飞控状态、PID 对象、传感器数据等）
// ============================================================================
static AsyncWebServer g_server(80);            // HTTP 服务器
AsyncWebSocket g_ws("/ws");             // WebSocket 对象
static ImuDriver g_imu;                        // IMU 驱动（自动检测 MPU6050/6500）
static MadgwickImu g_ahrs_madgwick;            // Madgwick 姿态解算器
static MahonyImu g_ahrs_mahony;                // Mahony 姿态解算器
static uint32_t g_stationary_start_ms = 0;     // 静止开始时间（用于自动水平校准）

float g_roll_deg = 0.0f, g_pitch_deg = 0.0f, g_yaw_deg = 0.0f;  // 当前姿态（度）
static bool g_comp_init = false;       // 互补滤波是否已初始化
static float g_comp_roll_deg=0, g_comp_pitch_deg=0, g_comp_yaw_deg=0; // 互补滤波姿态
bool g_takeoff_sounded = false;  // 是否已播放起飞音效
// 电压告警相关
static uint8_t g_voltage_alert_level = 0;   // 0=正常,1=低,2=严重,3=危急
static uint32_t g_last_voltage_check_ms = 0;

static volatile uint8_t g_cal_request = CAL_REQ_NONE;       // 校准请求
static portMUX_TYPE g_calMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t g_cal_state = CAL_STATE_IDLE;               // 校准状态
static uint32_t g_cal_state_until_ms = 0;                  // 校准状态结束时间
char g_status_msg[96] = "";                         // 状态消息
void setStatusMsg(const char *msg);
void setStatusMsgFs(uint8_t reason);

// ============================================================================
// 辅助函数声明（实现见本文件末尾）
// ============================================================================
static void wifiStartAp();
static void webServerStart();
static void updateVoltageAlert();
static void applyVoltageLed();
static void controlStep(float dt);


// ============================================================================
// 电池电压读取
// ============================================================================
float readVbatt() {
  if (kVbattAdcPin < 0) return NAN;
#if defined(ARDUINO_ARCH_ESP32)
  const uint32_t mv = analogReadMilliVolts(kVbattAdcPin);
  return ((float)mv / 1000.0f) * kVbattDividerRatio;
#else
  return NAN;
#endif
}

// ============================================================================
// 校准相关函数
// ============================================================================
void requestCalibration(uint8_t req) {
  portENTER_CRITICAL(&g_calMux);
  if (g_cal_request == CAL_REQ_NONE) g_cal_request = req;
  portEXIT_CRITICAL(&g_calMux);
}

static uint8_t takeCalibrationRequest() {
  uint8_t req = CAL_REQ_NONE;
  portENTER_CRITICAL(&g_calMux);
  req = g_cal_request;
  g_cal_request = CAL_REQ_NONE;
  portEXIT_CRITICAL(&g_calMux);
  return req;
}

static void setCalState(uint8_t state, uint32_t hold_ms) {
  g_cal_state = state;
  if (hold_ms > 0) g_cal_state_until_ms = millis() + hold_ms;
  else g_cal_state_until_ms = 0;
}

static void updateCalState(uint32_t now_ms) {
  if (g_cal_state_until_ms == 0) return;
  if ((int32_t)(now_ms - g_cal_state_until_ms) >= 0) {
    g_cal_state = CAL_STATE_IDLE;
    g_cal_state_until_ms = 0;
  }
}

void setStatusMsg(const char *msg) {
  if (msg == nullptr) g_status_msg[0] = '\0';
  else { strncpy(g_status_msg, msg, sizeof(g_status_msg)-1); g_status_msg[sizeof(g_status_msg)-1] = '\0'; }
}

void setStatusMsgFs(uint8_t reason) {
  if (reason == FS_NONE) setStatusMsg("DISARMED");
  else { char buf[64]; snprintf(buf, sizeof(buf), "FAILSAFE: %s", failSafeLabel(reason)); setStatusMsg(buf); }
}

// ============================================================================
// WiFi 与 Web 服务器
// ============================================================================
static void wifiStartAp() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kApSsid, kApPass);
  delay(100);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
}

static void webServerStart() {
  g_ws.onEvent(onWsEvent);
  g_server.addHandler(&g_ws);
  g_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", kIndexHtml);
  });
  g_server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });
  g_server.begin();
}

// ============================================================================
// 电压告警更新函数（根据电压等级控制蜂鸣器）
// ============================================================================
static void updateVoltageAlert() {
  float vbatt = readVbatt();
  if (!isFinitef(vbatt)) {
    if (g_voltage_alert_level != 0) {
      g_voltage_alert_level = 0;
      buzzerStopLoop();
    }
    return;
  }
  uint8_t new_level = 0;
  // 1S 电池阈值（满电4.2V）
  if (vbatt < 3.3f) new_level = 3;
  else if (vbatt < 3.5f) new_level = 2;
  else if (vbatt < 3.7f) new_level = 1;
  else new_level = 0;
  if (new_level != g_voltage_alert_level) {
    g_voltage_alert_level = new_level;
    buzzerStopLoop();
    if (new_level > 0) {
      switch (new_level) {
        case 1: buzzerPlay(BUZ_VOLT_ALERT_SLOW); break;
        case 2: buzzerPlay(BUZ_VOLT_ALERT_MED); break;
        case 3: buzzerPlay(BUZ_VOLT_ALERT_FAST); break;
      }
    }
  }
}

// ============================================================================
// 电压告警 LED 覆盖函数（覆盖原有红绿蓝LED显示）
// ============================================================================
static void applyVoltageLed() {
  if (g_voltage_alert_level == 0) return;
  uint32_t now_ms = millis();
  bool red_on = false, green_on = false, blue_on = false;
  switch (g_voltage_alert_level) {
    case 1:  // 蓝色慢闪
      blue_on = (now_ms % 1000) < 500;
      break;
    case 2:  // 绿色慢闪
      green_on = (now_ms % 1000) < 500;
      break;
    case 3:  // 红绿蓝快闪
      red_on = green_on = blue_on = (now_ms % 500) < 250;
      break;
  }
  if (red_on) ledcWrite(led1, kLedPwmMaxDuty);
  else ledcWrite(led1, 0);
  if (green_on) ledcWrite(led2, kLedPwmMaxDuty);
  else ledcWrite(led2, 0);
  digitalWrite(led3, blue_on ? HIGH : LOW);
}

// ============================================================================
// 核心控制循环（姿态解算、PID、混控）
// ============================================================================
static void controlStep(float dt) {
  applyTuningsIfDirty();
  RuntimeConfig cfg;
  FilterMode imu_mode = kFilterMode;
  portENTER_CRITICAL(&g_cfgMux);
  cfg = g_config;
  imu_mode = g_filter_mode;
  portEXIT_CRITICAL(&g_cfgMux);
  RcCommand rc = getRcCommand();
  const uint32_t now_ms = millis();
  updateCalState(now_ms);
  const uint32_t cmd_age_ms = (rc.last_ms == 0) ? 0xFFFFFFFFUL : (now_ms - rc.last_ms);

  // 处理紧急停止请求
  bool kill = false; uint8_t kill_reason = FS_KILL;
  portENTER_CRITICAL(&g_killMux);
  if (g_kill_pending) { kill = true; kill_reason = g_kill_reason; g_kill_pending = false; }
  portEXIT_CRITICAL(&g_killMux);
  if (kill) {
    buzzerPlay(BUZ_KILL);   //蜂鸣器报警：紧急停止 (KILL)	收到 kill 请求，BUZ_KILL（专用）
    disarmNow(kill_reason);
    g_led_mode = LED_FAILSAFE;
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 电机测试模式优先
  MotorTestState mt;
  if (getMotorTestState(&mt)) {
    if (mt.until_ms != 0 && (int32_t)(now_ms - mt.until_ms) >= 0) { stopMotorTest(); motorsWriteAll(0,0,0,0); }
    else {
      float m0=0,m1=0,m2=0,m3=0;
      switch(mt.index) {
        case 0: m0=mt.throttle; break;
        case 1: m1=mt.throttle; break;
        case 2: m2=mt.throttle; break;
        case 3: m3=mt.throttle; break;
      }
      motorsWriteAll(m0,m1,m2,m3);
      g_led_mode = LED_DISARMED;
      char msg[48]; snprintf(msg, sizeof(msg), "MOTOR TEST: %s", motorIndexLabel(mt.index));
      setStatusMsg(msg);
      sendTelemetry(dt, rc, cmd_age_ms, cfg);
    }
    return;
  }

  // 时间有效性检查
  if (dt <= 0.0f || dt > kDtMaxSec) {
    disarmNow(FS_DT_LIMIT);  
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);    //故障报警① dt 超限 
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 读取 IMU 数据
  ImuSample s;
  if (!g_imu.read(&s)) {
    disarmNow(FS_IMU_FAIL);
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);   //故障报警② IMU 读取失败
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  const float gx_rads = s.gx_dps * (float)M_PI / 180.0f;
  const float gy_rads = s.gy_dps * (float)M_PI / 180.0f;
  const float gz_rads = s.gz_dps * (float)M_PI / 180.0f;

  // 计算加速度计信任度（用于自适应滤波器）
  float acc_trust = 0.0f;
  const float acc_norm = sqrtf(s.ax_g*s.ax_g + s.ay_g*s.ay_g + s.az_g*s.az_g);
  if (isFinitef(acc_norm)) {
    const float dev = fabsf(acc_norm - 1.0f);
    if (kAccTrustLo > kAccTrustHi) acc_trust = (kAccTrustLo - dev) / (kAccTrustLo - kAccTrustHi);
    else acc_trust = (dev <= kAccTrustHi) ? 1.0f : 0.0f;
    acc_trust = clampf(acc_trust, 0.0f, 1.0f);
  }

  // 滤波器模式切换（保持姿态连续）
  static FilterMode active_mode = kFilterMode;
  if (imu_mode != active_mode) {
    float r0=0,p0=0,y0=0;
    if (active_mode == FILTER_MADGWICK) g_ahrs_madgwick.getEulerDeg(&r0,&p0,&y0);
    else if (active_mode == FILTER_MAHONY) g_ahrs_mahony.getEulerDeg(&r0,&p0,&y0);
    else { r0=g_comp_roll_deg; p0=g_comp_pitch_deg; y0=g_comp_yaw_deg; }
    g_ahrs_madgwick.setEulerDeg(r0,p0,y0);
    g_ahrs_mahony.reset(); g_ahrs_mahony.setEulerDeg(r0,p0,y0);
    g_comp_init=true; g_comp_roll_deg=r0; g_comp_pitch_deg=p0; g_comp_yaw_deg=y0;
    active_mode = imu_mode;
  }

  // 姿态解算
  float roll_raw=0, pitch_raw=0, yaw_raw=0;
  if (active_mode == FILTER_MADGWICK) {
    float beta_use = kMadgwickBetaMin + acc_trust * (kMadgwickBetaMax - kMadgwickBetaMin);
    beta_use = clampf(beta_use, kMadgwickBetaMin, kMadgwickBetaMax);
    g_ahrs_madgwick.setBeta(beta_use);
    g_ahrs_madgwick.update(gx_rads, gy_rads, gz_rads, s.ax_g, s.ay_g, s.az_g, dt);
    g_ahrs_madgwick.getEulerDeg(&roll_raw, &pitch_raw, &yaw_raw);
  } else if (active_mode == FILTER_MAHONY) {
    g_ahrs_mahony.update(gx_rads, gy_rads, gz_rads, s.ax_g, s.ay_g, s.az_g, dt, acc_trust);
    g_ahrs_mahony.getEulerDeg(&roll_raw, &pitch_raw, &yaw_raw);
  } else {
    float acc_roll=0, acc_pitch=0;
    accelToRollPitchDeg(s, &acc_roll, &acc_pitch);
    if (!g_comp_init || !isFinitef(g_comp_roll_deg)) {
      g_comp_roll_deg=acc_roll; g_comp_pitch_deg=acc_pitch; g_comp_yaw_deg=0; g_comp_init=true;
    } else {
      g_comp_roll_deg += s.gx_dps * dt;
      g_comp_pitch_deg += s.gy_dps * dt;
      g_comp_yaw_deg += s.gz_dps * dt;
      const float alpha_use = clampf(1.0f - (1.0f - kCompAlpha) * acc_trust, kCompAlpha, 1.0f);
      g_comp_roll_deg = alpha_use * g_comp_roll_deg + (1.0f - alpha_use) * acc_roll;
      g_comp_pitch_deg = alpha_use * g_comp_pitch_deg + (1.0f - alpha_use) * acc_pitch;
    }
    roll_raw = g_comp_roll_deg; pitch_raw = g_comp_pitch_deg; yaw_raw = g_comp_yaw_deg;
  }

  if (!isFinitef(roll_raw) || !isFinitef(pitch_raw) || !isFinitef(yaw_raw)) {
    disarmNow(FS_IMU_FAIL);
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);   //故障报警③	姿态解算无效	roll_raw/pitch_raw/yaw_raw 非有限值
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 静态状态检测（用于自动水平校准）
  const bool stationary_now = isImuStationary(s);
  if (!g_armed && stationary_now) { if (g_stationary_start_ms == 0) g_stationary_start_ms = now_ms; } else g_stationary_start_ms = 0;
  float use_roll = roll_raw, use_pitch = pitch_raw;
  if (!g_armed && g_stationary_start_ms != 0 && (now_ms - g_stationary_start_ms) >= kRelevelHoldMs) {
    float acc_roll=0, acc_pitch=0; accelToRollPitchDeg(s, &acc_roll, &acc_pitch);
    if (isFinitef(acc_roll) && isFinitef(acc_pitch)) {
      use_roll = acc_roll; use_pitch = acc_pitch;
      g_ahrs_madgwick.setEulerDeg(use_roll, use_pitch, yaw_raw);
      g_ahrs_mahony.reset(); g_ahrs_mahony.setEulerDeg(use_roll, use_pitch, yaw_raw);
      g_comp_init=true; g_comp_roll_deg=use_roll; g_comp_pitch_deg=use_pitch; g_comp_yaw_deg=yaw_raw;
    }
  }

  // 处理校准请求
  const uint8_t cal_req = takeCalibrationRequest();
  if (cal_req != CAL_REQ_NONE) {
    if (g_armed) {
      setCalState(CAL_STATE_FAIL, 1500);
    }
    else if (cal_req == CAL_REQ_LEVEL) {
      g_level_roll_offset_deg = use_roll;
      g_level_pitch_offset_deg = use_pitch;
      saveTunings();
      setCalState(CAL_STATE_LEVEL, 1500);
      buzzerPlay(BUZ_TRIPLE_BEEP);
    }
    else if (cal_req == CAL_REQ_GYRO) {
      motorsWriteAll(0, 0, 0, 0);
      g_imu.calibrateGyro(500, 2);
      setCalState(CAL_STATE_GYRO, 1500);
      buzzerPlay(BUZ_DOUBLE_BEEP);
    }
  }

  // 应用水平校准偏置
  g_roll_deg = use_roll - g_level_roll_offset_deg;
  g_pitch_deg = use_pitch - g_level_pitch_offset_deg;
  g_yaw_deg = yaw_raw - g_level_yaw_offset_deg;

  // 调试输出（每秒 100 次姿态）
  if (kEnableSerialAttDebug) {
    static uint32_t last_serial_ms=0;
    if ((now_ms - last_serial_ms) >= 10) { last_serial_ms=now_ms; Serial.printf("ATT,%.2f,%.2f,%.2f\n", g_roll_deg, g_pitch_deg, g_yaw_deg); }
  }

  // 倾斜保护
  if (fabsf(g_roll_deg) > cfg.tilt_disarm_deg || fabsf(g_pitch_deg) > cfg.tilt_disarm_deg) {
    disarmNow(FS_TILT_LIMIT);
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);   //故障报警④ 倾斜保护：横滚/俯仰超过 tilt_disarm_deg
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 遥控超时保护
  if (rc.last_ms != 0 && cmd_age_ms > cfg.cmd_timeout_ms) {
    disarmNow(FS_CMD_TIMEOUT);
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);   //故障报警⑤ 遥控超时：cmd_age_ms > cfg.cmd_timeout_ms 成立
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 更新解锁状态
  updateArmingState(rc, now_ms);
  if (!g_armed) { g_led_mode = (rc.arm_request && !g_arm_inhibit) ? LED_ARMING : LED_DISARMED; sendTelemetry(dt, rc, cmd_age_ms, cfg); return; }
  g_led_mode = LED_ARMED;

  // 油门极低时强制停转
  const float throttle = clampf(rc.throttle, 0.0f, 1.0f);
  if (throttle <= kArmThrottleMax) {
    motorsWriteAll(0.0f, 0.0f, 0.0f, 0.0f);
    resetAllPid();
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // 起飞音效触发（首次推油门）
  if (!g_takeoff_sounded && throttle > kArmThrottleMax) {
    g_takeoff_sounded = true;
    buzzerPlay(BUZ_TAKEOFF);
  }

  // 计算控制量
  const float roll_sp_deg = rc.roll * cfg.max_angle_deg;
  const float pitch_sp_deg = rc.pitch * cfg.max_angle_deg;
  const float yaw_rate_sp_dps = rc.yaw * cfg.max_yaw_rate_dps;

  // 角度环（外环）输出期望角速度
  const float roll_rate_sp_dps = g_roll_angle_pid.update(roll_sp_deg - g_roll_deg, dt);
  const float pitch_rate_sp_dps = g_pitch_angle_pid.update(pitch_sp_deg - g_pitch_deg, dt);

  // 扭矩缩放（低油门时保持操控性）
  const float torque_scale = clampf(cfg.torque_scale_min + throttle * cfg.torque_scale_slope, 0.0f, 1.0f);

  // 角速度环（内环）输出归一化扭矩
  float roll_torque = g_roll_rate_pid.update(roll_rate_sp_dps - s.gx_dps, dt) * torque_scale;
  float pitch_torque = g_pitch_rate_pid.update(pitch_rate_sp_dps - s.gy_dps, dt) * torque_scale;
  float yaw_torque = g_yaw_rate_pid.update(yaw_rate_sp_dps - s.gz_dps, dt) * torque_scale;

  if (!isFinitef(roll_torque) || !isFinitef(pitch_torque) || !isFinitef(yaw_torque)) {
    disarmNow(FS_IMU_FAIL);
    g_led_mode = LED_FAILSAFE;
    buzzerPlay(BUZ_FAST_ALARM);   //故障报警⑥ PID 计算异常：roll_torque/pitch_torque/yaw_torque 非有限值
    sendTelemetry(dt, rc, cmd_age_ms, cfg);
    return;
  }

  // X 型混控（油门 + 横滚 + 俯仰 + 偏航）
  float m0 = throttle + roll_torque + pitch_torque - yaw_torque;   // 左前
  float m1 = throttle - roll_torque + pitch_torque + yaw_torque;   // 右前
  float m2 = throttle - roll_torque - pitch_torque - yaw_torque;   // 右后
  float m3 = throttle + roll_torque - pitch_torque + yaw_torque;   // 左后

  // 限幅并输出
  m0 = clampf(m0,0,1); m1 = clampf(m1,0,1); m2 = clampf(m2,0,1); m3 = clampf(m3,0,1);
  motorsWriteAll(m0,m1,m2,m3);

  // 发送遥测
  sendTelemetry(dt, rc, cmd_age_ms, cfg);
}

// ============================================================================
// 系统初始化与主循环
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Boot...");

  // LED 初始化（红绿蓝）
  g_led_mode = LED_BOOT;
  ledInit();
  pinMode(led3, OUTPUT);
  digitalWrite(led3, LOW);

  // 蜂鸣器初始化并播放上电提示音
  buzzerInit();
  buzzerPlay(BUZ_POWER_ON);

  // 电机初始化（硬件层）
  const bool motor_ok = motorInit();
  motorsWriteAll(0,0,0,0);
  if (!motor_ok) {
    Serial.println("LEDC motor init FAILED");
    g_motor_fault = true;
    disarmNow(FS_MOTOR_FAIL);
    g_led_mode = LED_FAILSAFE;
    setStatusMsg("FAILSAFE: MOTOR_FAIL (LEDC init)");
  }

  // IMU 自动检测与初始化
  if (!g_imu.begin()) {
    Serial.println("IMU init FAILED (no sensor detected)");
    while (true) delay(1000);
  }
  Serial.println("Calibrating gyro... keep still");
  g_imu.calibrateGyro(500, 2);
  Serial.println("Gyro calibration done");

  // 姿态滤波器初始化
  g_ahrs_madgwick.begin(kMadgwickBeta);
  g_ahrs_mahony.begin(kMahonyKp, kMahonyKi);
  g_ahrs_mahony.setIntegralLimitRads(kMahonyIntegralLimitRads);
  g_ahrs_mahony.setSpinRateLimitDps(kMahonySpinLimitDps);

  // 加载并应用 PID 参数
  loadTunings();
  applyTunings(g_tunings);
  resetAllPid();

  // 配置电池电压 ADC
  if (kVbattAdcPin >= 0) {
#if defined(ARDUINO_ARCH_ESP32)
    analogReadResolution(12);
    analogSetPinAttenuation(kVbattAdcPin, ADC_11db);
#endif
  }

  // 初始状态为上锁（无故障）
  disarmNow(FS_NONE);

  // 启动 WiFi 热点和 Web 服务器
  wifiStartAp();
  webServerStart();

  // 系统就绪提示音
  buzzerPlay(BUZ_READY);
  g_led_mode = LED_DISARMED;
  Serial.println("Ready. Open http://192.168.9.1/");
}

void loop() {
  static uint32_t last_us = 0, next_us = 0;

  // 更新 LED 状态（红绿灯）
  ledUpdate();

  // 更新蜂鸣器状态机
  buzzerUpdate();

  const uint32_t now_us = micros();
  if (last_us == 0) { last_us = now_us; next_us = now_us + kLoopPeriodUs; return; }
  if ((int32_t)(now_us - next_us) >= 0) {
    while ((int32_t)(now_us - next_us) >= 0) next_us += kLoopPeriodUs;
    const float dt = (now_us - last_us) * 1e-6f;
    last_us = now_us;

    // 核心控制循环
    controlStep(dt);

    // 电压告警检查（每200ms）
    static uint32_t last_voltage_check_ms = 0;
    uint32_t now_ms = millis();
    if (now_ms - last_voltage_check_ms >= 200) {
      last_voltage_check_ms = now_ms;
      updateVoltageAlert();
    }

    // 蓝色 LED 状态更新（慢闪/常亮/快闪）
    static uint32_t last_blue_update = 0;
    if (now_ms - last_blue_update >= 50) {
      last_blue_update = now_ms;
      RcCommand rc = getRcCommand();
      bool flying = g_armed && (rc.throttle > 0.05f);
      bool led_on = false;
      if (flying) {
        led_on = (now_ms % 200) < 100;          // 快闪（周期200ms）
      } else if (g_websocket_connected) {
        led_on = true;                          // 常亮（网页连接）
      } else {
        led_on = (now_ms % 1000) < 500;         // 慢闪（待机）
      }
      digitalWrite(led3, led_on ? HIGH : LOW);
    }

    // 电压告警 LED 覆盖（优先级高于飞控状态）
    applyVoltageLed();

  } else {
    delayMicroseconds(50);
  }
}