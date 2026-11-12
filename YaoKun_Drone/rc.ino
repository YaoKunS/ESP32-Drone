/**
 * @file rc.ino
 * @brief 遥控指令存储与 WebSocket 命令处理
 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 * 本文件包含：
 *   - 遥控指令的线程安全存储 (getRcCommand/setRcCommand)
 *   - WebSocket 消息处理 (PID调参、配置修改、校准、电机测试、遥控指令等)
 */

#include "drone_types.h"
#include "config.h"
#include <ESPAsyncWebServer.h>

// ============================================================================
// 遥控指令存储（临界区保护）
// ============================================================================
RcCommand g_rc = {0.0f, 0.0f, 0.0f, 0.0f, false, 0};
portMUX_TYPE g_rcMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief 获取当前遥控指令（线程安全）
 */
RcCommand getRcCommand() {
  RcCommand out;
  portENTER_CRITICAL(&g_rcMux);
  out = g_rc;
  portEXIT_CRITICAL(&g_rcMux);
  return out;
}

/**
 * @brief 设置遥控指令（线程安全）
 * @param throttle     油门 0~1
 * @param roll         横滚 -1~1
 * @param pitch        俯仰 -1~1
 * @param yaw          偏航 -1~1
 * @param arm_request  解锁请求 true/false
 */
void setRcCommand(float throttle, float roll, float pitch, float yaw, bool arm_request) {
  portENTER_CRITICAL(&g_rcMux);
  g_rc.throttle = clampf(throttle, 0.0f, 1.0f);
  g_rc.roll = clampf(roll, -1.0f, 1.0f);
  g_rc.pitch = clampf(pitch, -1.0f, 1.0f);
  g_rc.yaw = clampf(yaw, -1.0f, 1.0f);
  g_rc.arm_request = arm_request;
  g_rc.last_ms = millis();
  portEXIT_CRITICAL(&g_rcMux);
}

// ============================================================================
// WebSocket 命令处理
// ============================================================================

// 外部变量声明（定义在其他模块）
extern AsyncWebSocket g_ws;
extern bool g_websocket_connected;
extern bool g_armed;
extern bool g_motor_fault;
extern volatile bool g_tunings_dirty;
extern Tunings g_tunings;
extern RuntimeConfig g_config;
extern FilterMode g_filter_mode;
extern portMUX_TYPE g_cfgMux;

// 外部函数声明
extern void requestKill(uint8_t reason);
extern void saveTunings();
extern void stopMotorTest();
extern bool startMotorTest(uint8_t index, float throttle, uint32_t duration_ms);
extern void setBuzzerMute(bool mute);
extern void setTakeoffRepeat(uint8_t times);
extern void requestCalibration(uint8_t req);
extern void setStatusMsg(const char* msg);
extern bool isBuzzerMuted();
extern uint8_t getTakeoffRepeat();

/**
 * @brief 发送飞控配置参数给客户端
 */
static void sendCfgToClient(AsyncWebSocketClient *client) {
  if (client == nullptr) return;
  Tunings t;
  RuntimeConfig cfg;
  FilterMode imu = kFilterMode;
  portENTER_CRITICAL(&g_cfgMux);
  t = g_tunings;
  cfg = g_config;
  imu = g_filter_mode;
  portEXIT_CRITICAL(&g_cfgMux);
  char buf[520];
  snprintf(buf, sizeof(buf),
    "{\"type\":\"cfg\",\"imu_filter\":%u,\"angle\":{\"kp\":%.6f,\"ki\":%.6f,\"kd\":%.6f},"
    "\"rate\":{\"kp\":%.6f,\"ki\":%.6f,\"kd\":%.6f},"
    "\"yaw\":{\"kp\":%.6f,\"ki\":%.6f,\"kd\":%.6f},"
    "\"limits\":{\"max_angle\":%.2f,\"max_yaw_rate\":%.2f,\"tilt_disarm\":%.2f,"
    "\"cmd_timeout\":%lu,\"telem_ms\":%lu,"
    "\"torque_min\":%.3f,\"torque_slope\":%.3f},"
    "\"buzzer_mute\":%d,\"takeoff_repeat\":%d}",
    (unsigned int)imu,
    t.angle.kp, t.angle.ki, t.angle.kd,
    t.rate.kp, t.rate.ki, t.rate.kd,
    t.yaw.kp, t.yaw.ki, t.yaw.kd,
    cfg.max_angle_deg, cfg.max_yaw_rate_dps, cfg.tilt_disarm_deg,
    (unsigned long)cfg.cmd_timeout_ms, (unsigned long)cfg.telem_period_ms,
    cfg.torque_scale_min, cfg.torque_scale_slope,
    (int)isBuzzerMuted(), (int)getTakeoffRepeat());
  client->text(buf);
}

/**
 * @brief 发送应答消息（成功/失败）
 */
static void sendAck(AsyncWebSocketClient *client, const char *op, const char *tag, bool ok, const char *msg) {
  if (client == nullptr || op == nullptr) return;
  const bool has_tag = (tag != nullptr && tag[0] != '\0');
  const bool has_msg = (msg != nullptr && msg[0] != '\0');
  char buf[160];
  if (has_tag && has_msg) {
    snprintf(buf, sizeof(buf), "{\"type\":\"ack\",\"op\":\"%s\",\"tag\":\"%s\",\"ok\":%d,\"msg\":\"%s\"}", op, tag, ok ? 1 : 0, msg);
  } else if (has_tag) {
    snprintf(buf, sizeof(buf), "{\"type\":\"ack\",\"op\":\"%s\",\"tag\":\"%s\",\"ok\":%d}", op, tag, ok ? 1 : 0);
  } else if (has_msg) {
    snprintf(buf, sizeof(buf), "{\"type\":\"ack\",\"op\":\"%s\",\"ok\":%d,\"msg\":\"%s\"}", op, ok ? 1 : 0, msg);
  } else {
    snprintf(buf, sizeof(buf), "{\"type\":\"ack\",\"op\":\"%s\",\"ok\":%d}", op, ok ? 1 : 0);
  }
  client->text(buf);
}

/**
 * @brief 处理 PID 参数设置命令
 */
static void handlePidMsg(char *saveptr) {
  const char *tag = strtok_r(nullptr, ",", &saveptr);
  const char *kp_s = strtok_r(nullptr, ",", &saveptr);
  const char *ki_s = strtok_r(nullptr, ",", &saveptr);
  const char *kd_s = strtok_r(nullptr, ",", &saveptr);
  if (tag == nullptr || kp_s == nullptr || ki_s == nullptr || kd_s == nullptr) return;
  const float kp = atof(kp_s), ki = atof(ki_s), kd = atof(kd_s);
  if (!isFinitef(kp) || !isFinitef(ki) || !isFinitef(kd)) return;
  portENTER_CRITICAL(&g_cfgMux);
  if (strcmp(tag, "ANGLE") == 0) {
    g_tunings.angle.kp = kp; g_tunings.angle.ki = ki; g_tunings.angle.kd = kd;
    g_tunings_dirty = true;
  } else if (strcmp(tag, "RATE") == 0) {
    g_tunings.rate.kp = kp; g_tunings.rate.ki = ki; g_tunings.rate.kd = kd;
    g_tunings_dirty = true;
  } else if (strcmp(tag, "YAW") == 0) {
    g_tunings.yaw.kp = kp; g_tunings.yaw.ki = ki; g_tunings.yaw.kd = kd;
    g_tunings_dirty = true;
  }
  portEXIT_CRITICAL(&g_cfgMux);
}

/**
 * @brief 处理配置修改命令
 */
static void handleCfgMsg(char *saveptr) {
  const char *key = strtok_r(nullptr, ",", &saveptr);
  const char *val_s = strtok_r(nullptr, ",", &saveptr);
  if (key == nullptr || val_s == nullptr) return;
  const float vf = atof(val_s);
  const uint32_t vu = atoi(val_s);
  portENTER_CRITICAL(&g_cfgMux);
  if (strcmp(key, "MAX_ANGLE") == 0) {
    if (isFinitef(vf)) g_config.max_angle_deg = clampf(vf, kMinMaxAngleDeg, kMaxMaxAngleDeg);
  } else if (strcmp(key, "MAX_YAW_RATE") == 0) {
    if (isFinitef(vf)) g_config.max_yaw_rate_dps = clampf(vf, kMinMaxYawRateDps, kMaxMaxYawRateDps);
  } else if (strcmp(key, "TILT_DISARM") == 0) {
    if (isFinitef(vf)) g_config.tilt_disarm_deg = clampf(vf, kMinTiltDisarmDeg, kMaxTiltDisarmDeg);
  } else if (strcmp(key, "CMD_TIMEOUT") == 0) {
    g_config.cmd_timeout_ms = clampu32(vu, kMinCmdTimeoutMs, kMaxCmdTimeoutMs);
  } else if (strcmp(key, "TELEM_MS") == 0) {
    g_config.telem_period_ms = clampu32(vu, kMinTelemPeriodMs, kMaxTelemPeriodMs);
  } else if (strcmp(key, "TORQUE_MIN") == 0) {
    if (isFinitef(vf)) g_config.torque_scale_min = clampf(vf, kMinTorqueScaleMin, kMaxTorqueScaleMin);
  } else if (strcmp(key, "TORQUE_SLOPE") == 0) {
    if (isFinitef(vf)) g_config.torque_scale_slope = clampf(vf, kMinTorqueScaleSlope, kMaxTorqueScaleSlope);
  } else if (strcmp(key, "IMU_FILTER") == 0) {
    if (!g_armed && vu <= (uint32_t)FILTER_COMP) g_filter_mode = (FilterMode)vu;
  }
  portEXIT_CRITICAL(&g_cfgMux);
}

/**
 * @brief WebSocket 文本消息总入口 （遥控指令、PID 调参、校准、电机测试等）
 */
static void handleWsText(AsyncWebSocketClient *client, char *msg) {
  if (msg == nullptr) return;
  if (strcmp(msg, "K") == 0) { requestKill(FS_KILL); return; }
  if (strcmp(msg, "GET") == 0) { sendCfgToClient(client); sendAck(client, "GET", nullptr, true, nullptr); return; }
  if (strcmp(msg, "SAVE") == 0) { saveTunings(); sendAck(client, "SAVE", nullptr, true, nullptr); return; }
  char *saveptr = nullptr;
  char *head = strtok_r(msg, ",", &saveptr);
  if (head == nullptr) return;
  if (strcmp(head, "PID") == 0) { handlePidMsg(saveptr); return; }
  if (strcmp(head, "CFG") == 0) { handleCfgMsg(saveptr); return; }
  if (strcmp(head, "CAL") == 0) {
    const char *tag = strtok_r(nullptr, ",", &saveptr);
    if (tag == nullptr) { sendAck(client, "CAL", nullptr, false, "BAD"); return; }
    if (g_armed) { sendAck(client, "CAL", tag, false, "ARMED"); return; }
    if (strcmp(tag, "LEVEL") == 0) { requestCalibration(CAL_REQ_LEVEL); sendAck(client, "CAL", tag, true, "QUEUED"); return; }
    if (strcmp(tag, "GYRO") == 0) { requestCalibration(CAL_REQ_GYRO); sendAck(client, "CAL", tag, true, "QUEUED"); return; }
    sendAck(client, "CAL", tag, false, "BAD"); return;
  }
  if (strcmp(head, "MTEST") == 0) {
    const char *tag = strtok_r(nullptr, ",", &saveptr);
    if (tag == nullptr) { sendAck(client, "MTEST", nullptr, false, "BAD"); return; }
    if (strcmp(tag, "STOP") == 0) { stopMotorTest(); sendAck(client, "MTEST", "STOP", true, "OK"); return; }
    char *endptr = nullptr;
    const long idx_long = strtol(tag, &endptr, 10);
    if (endptr == tag || *endptr != '\0') { sendAck(client, "MTEST", tag, false, "BAD"); return; }
    const char *t_s = strtok_r(nullptr, ",", &saveptr);
    const char *ms_s = strtok_r(nullptr, ",", &saveptr);
    if (t_s == nullptr || ms_s == nullptr) { sendAck(client, "MTEST", tag, false, "BAD"); return; }
    if (g_armed) { sendAck(client, "MTEST", tag, false, "ARMED"); return; }
    if (g_motor_fault) { sendAck(client, "MTEST", tag, false, "MOTOR_FAIL"); return; }
    const RcCommand rc = getRcCommand();
    if (rc.arm_request || rc.throttle > kArmThrottleMax) { sendAck(client, "MTEST", tag, false, "RC_ACTIVE"); return; }
    if (idx_long < 0 || idx_long > 3) { sendAck(client, "MTEST", tag, false, "BAD_IDX"); return; }
    const float thr = (float)atof(t_s);
    const uint32_t ms = (uint32_t)atoi(ms_s);
    if (!startMotorTest((uint8_t)idx_long, thr, ms)) { sendAck(client, "MTEST", tag, false, "BAD"); return; }
    sendAck(client, "MTEST", tag, true, "RUN"); return;
  }
  // 蜂鸣器全局静音开关
  if (strcmp(head, "BUZZER_MUTE") == 0) {
    const char *val_s = strtok_r(nullptr, ",", &saveptr);
    if (val_s) {
      bool mute = (atoi(val_s) != 0);
      setBuzzerMute(mute);
      saveTunings();
      sendAck(client, "BUZZER_MUTE", nullptr, true, mute ? "MUTED" : "UNMUTED");
    } else {
      sendAck(client, "BUZZER_MUTE", nullptr, false, "BAD");
    }
    return;
  }
  // 设置起飞音效重复次数
  if (strcmp(head, "TAKEOFF_REPEAT") == 0) {
    const char *val_s = strtok_r(nullptr, ",", &saveptr);
    if (val_s) {
      uint8_t times = (uint8_t)atoi(val_s);
      setTakeoffRepeat(times);
      saveTunings();
      sendAck(client, "TAKEOFF_REPEAT", nullptr, true, "OK");
    } else {
      sendAck(client, "TAKEOFF_REPEAT", nullptr, false, "BAD");
    }
    return;
  }
  // 遥控指令 C 命令（关键部分）
  if (strcmp(head, "C") == 0) {
    const char *t_s = strtok_r(nullptr, ",", &saveptr);
    const char *r_s = strtok_r(nullptr, ",", &saveptr);
    const char *p_s = strtok_r(nullptr, ",", &saveptr);
    const char *y_s = strtok_r(nullptr, ",", &saveptr);
    const char *a_s = strtok_r(nullptr, ",", &saveptr);
    if (t_s == nullptr || r_s == nullptr || p_s == nullptr || y_s == nullptr || a_s == nullptr) return;
    setRcCommand(atof(t_s), atof(r_s), atof(p_s), atof(y_s), atoi(a_s) != 0);
    return;
  }
}

/**
 * @brief WebSocket 事件回调
 */
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  (void)server; (void)arg;
  if (type == WS_EVT_CONNECT) {
    g_websocket_connected = true;
    sendCfgToClient(client);
    return;
  }
  if (type == WS_EVT_DISCONNECT) {
    g_websocket_connected = false;
    requestKill(FS_WS_DISCONNECT);
    return;
  }
  if (type != WS_EVT_DATA) return;
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info == nullptr) return;
  if (!info->final || info->index != 0 || info->len != len) return;
  if (info->opcode != WS_TEXT) return;
  char buf[192];
  const size_t copy_len = (len < (sizeof(buf)-1)) ? len : (sizeof(buf)-1);
  memcpy(buf, data, copy_len);
  buf[copy_len] = '\0';
  handleWsText(client, buf);
}