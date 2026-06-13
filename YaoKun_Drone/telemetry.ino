/**
 * @file telemetry.ino
 * @brief 遥测数据发送模块（WebSocket JSON），20260612
 * 本文件包含发送飞控状态数据到 Web 界面的功能。
 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 */

#include "drone_types.h"
#include "config.h"
#include "drone_imu.h"       // 需要 ImuSample 等定义
#include <ESPAsyncWebServer.h>   // 新增：提供 AsyncWebSocket 完整定义


// ============================================================
// 遥测相关全局变量
// ============================================================
uint32_t g_last_telem_ms = 0;   // 上次发送遥测时间（毫秒）

// ============================================================
// 辅助函数（被遥测模块自身及其他模块调用，故保留外部链接）
// ============================================================

/**
 * @brief 获取校准状态标签字符串
 */
const char* calStateLabel(uint8_t state) {
  switch (state) {
    case CAL_STATE_LEVEL: return "LEVEL";
    case CAL_STATE_GYRO:  return "GYRO";
    case CAL_STATE_FAIL:  return "FAIL";
    default: return "IDLE";
  }
}

/**
 * @brief 获取故障原因标签字符串
 */
const char* failSafeLabel(uint8_t reason) {
  switch (reason) {
    case FS_CMD_TIMEOUT:   return "CMD_TIMEOUT";
    case FS_WS_DISCONNECT: return "WS_DISCONNECT";
    case FS_IMU_FAIL:      return "IMU_FAIL";
    case FS_TILT_LIMIT:    return "TILT_LIMIT";
    case FS_DT_LIMIT:      return "DT_LIMIT";
    case FS_KILL:          return "KILL";
    case FS_MANUAL:        return "MANUAL";
    case FS_MOTOR_FAIL:    return "MOTOR_FAIL";
    default: return "NONE";
  }
}

/**
 * @brief 获取电机索引标签字符串（用于调试）
 */
const char* motorIndexLabel(uint8_t index) {
  switch (index) {
    case 0: return "M0 FL";
    case 1: return "M1 FR";
    case 2: return "M2 RR";
    case 3: return "M3 RL";
    default: return "M?";
  }
}

// ============================================================
// 遥测发送主函数
// ============================================================

/**
 * @brief 发送遥测数据到 WebSocket 客户端
 * @param dt          控制循环间隔时间（秒）
 * @param rc          当前遥控指令
 * @param cmd_age_ms  遥控指令年龄（毫秒）
 * @param cfg         运行时配置
 * 
 * 注意：此函数依赖外部全局变量：g_last_telem_ms, g_armed, g_roll_deg,
 *       g_pitch_deg, g_yaw_deg, g_motor_last, g_last_fs, g_cal_state,
 *       g_status_msg, g_ws, g_websocket_connected（但此处仅使用 g_ws）
 */
// ============================================================================
// 发送遥测数据（WebSocket JSON）
// ============================================================================
void sendTelemetry(float dt, const RcCommand &rc, uint32_t cmd_age_ms, const RuntimeConfig &cfg) {
  const uint32_t now_ms = millis();
  if ((now_ms - g_last_telem_ms) < cfg.telem_period_ms) return;
  g_last_telem_ms = now_ms;
  
  const float loop_hz = (dt > 1e-6f) ? (1.0f / dt) : 0.0f;
  const float vbatt = readVbatt();  // 需要外部函数 readVbatt()
  
  char vbatt_buf[16];
  if (isFinitef(vbatt)) snprintf(vbatt_buf, sizeof(vbatt_buf), "%.3f", vbatt);
  else snprintf(vbatt_buf, sizeof(vbatt_buf), "null");
  
  const char *cal = calStateLabel(g_cal_state);
  
  char msg_buf[96];
  strncpy(msg_buf, g_status_msg, sizeof(msg_buf)-1);
  msg_buf[sizeof(msg_buf)-1] = '\0';
  
  char buf[480];
  snprintf(buf, sizeof(buf),
    "{\"type\":\"tel\",\"t\":%lu,\"armed\":%d,\"roll\":%.2f,\"pitch\":%.2f,\"yaw\":%.2f,"
    "\"dt_ms\":%.2f,\"loop_hz\":%.1f,\"cmd_age\":%lu,\"m0\":%.3f,\"m1\":%.3f,\"m2\":%.3f,\"m3\":%.3f,"
    "\"vbatt\":%s,\"fs\":%u,\"cal\":\"%s\",\"msg\":\"%s\"}",
    (unsigned long)now_ms, g_armed ? 1 : 0, g_roll_deg, g_pitch_deg, g_yaw_deg,
    dt * 1000.0f, loop_hz, (unsigned long)cmd_age_ms,
    g_motor_last[0], g_motor_last[1], g_motor_last[2], g_motor_last[3],
    vbatt_buf, (unsigned int)g_last_fs, cal, msg_buf);
    
  g_ws.textAll(buf);   // g_ws 需要声明为 extern
}