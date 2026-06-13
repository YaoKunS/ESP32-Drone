/**
 * @file storage.ino
 * @brief 参数存储模块（NVS 非易失性存储）,20260612
 * 
 * 本文件包含飞控所有可调节参数的 NVS 存储实现。
 * 包括 PID 参数、运行时配置（最大角度、超时等）、水平校准偏置、滤波器模式等。
 * 这些参数在 setup() 中加载，在 Web 调参时保存。
 */

#include "drone_types.h"
#include "config.h"
#include <Preferences.h>

// ============================================================
// 参数存储全局变量（定义）
// ============================================================

// 运行时配置（从网页可调节的参数）
RuntimeConfig g_config = {
  kDefaultMaxAngleDeg, kDefaultMaxYawRateDps, kDefaultTiltDisarmDeg,
  kDefaultCmdTimeoutMs, kDefaultTelemPeriodMs,
  kDefaultTorqueScaleMin, kDefaultTorqueScaleSlope,
};

// PID 调参组
Tunings g_tunings = {
  {2.0f, 0.001f, 0.0f},     // 角度环 PID
  {0.025f, 0.0f, 0.0002f},  // 角速率环 PID
  {0.030f, 0.0f, 0.0f},     // 偏航角速率 PID
};

// 水平校准偏置
float g_level_roll_offset_deg = 0.0f;
float g_level_pitch_offset_deg = 0.0f;
float g_level_yaw_offset_deg = 0.0f;

// PID 参数脏标志及互斥锁
volatile bool g_tunings_dirty = false;
portMUX_TYPE g_cfgMux = portMUX_INITIALIZER_UNLOCKED;

// 当前姿态滤波器模式（0: Madgwick, 1: Mahony, 2: 互补滤波）
FilterMode g_filter_mode = kFilterMode;

// NVS 偏好设置对象
Preferences g_prefs;

// ============================================================
// 辅助函数：对运行时配置进行限幅（内部使用，static）
// ============================================================
static void clampConfig(RuntimeConfig *cfg) {
  cfg->max_angle_deg = clampf(cfg->max_angle_deg, kMinMaxAngleDeg, kMaxMaxAngleDeg);
  cfg->max_yaw_rate_dps = clampf(cfg->max_yaw_rate_dps, kMinMaxYawRateDps, kMaxMaxYawRateDps);
  cfg->tilt_disarm_deg = clampf(cfg->tilt_disarm_deg, kMinTiltDisarmDeg, kMaxTiltDisarmDeg);
  cfg->cmd_timeout_ms = clampu32(cfg->cmd_timeout_ms, kMinCmdTimeoutMs, kMaxCmdTimeoutMs);
  cfg->telem_period_ms = clampu32(cfg->telem_period_ms, kMinTelemPeriodMs, kMaxTelemPeriodMs);
  cfg->torque_scale_min = clampf(cfg->torque_scale_min, kMinTorqueScaleMin, kMaxTorqueScaleMin);
  cfg->torque_scale_slope = clampf(cfg->torque_scale_slope, kMinTorqueScaleSlope, kMaxTorqueScaleSlope);
}

// ============================================================
// 从 NVS 加载所有参数
// ============================================================
void loadTunings() {
  g_prefs.begin("drone", true);   // 只读模式打开命名空间 "drone"

  // PID 参数
  g_tunings.angle.kp = g_prefs.getFloat("a_kp", g_tunings.angle.kp);
  g_tunings.angle.ki = g_prefs.getFloat("a_ki", g_tunings.angle.ki);
  g_tunings.angle.kd = g_prefs.getFloat("a_kd", g_tunings.angle.kd);
  g_tunings.rate.kp = g_prefs.getFloat("r_kp", g_tunings.rate.kp);
  g_tunings.rate.ki = g_prefs.getFloat("r_ki", g_tunings.rate.ki);
  g_tunings.rate.kd = g_prefs.getFloat("r_kd", g_tunings.rate.kd);
  g_tunings.yaw.kp = g_prefs.getFloat("y_kp", g_tunings.yaw.kp);
  g_tunings.yaw.ki = g_prefs.getFloat("y_ki", g_tunings.yaw.ki);
  g_tunings.yaw.kd = g_prefs.getFloat("y_kd", g_tunings.yaw.kd);

  // 运行时配置
  g_config.max_angle_deg = g_prefs.getFloat("max_ang", g_config.max_angle_deg);
  g_config.max_yaw_rate_dps = g_prefs.getFloat("max_yaw", g_config.max_yaw_rate_dps);
  g_config.tilt_disarm_deg = g_prefs.getFloat("tilt_dis", g_config.tilt_disarm_deg);
  g_config.cmd_timeout_ms = g_prefs.getUInt("cmd_to", g_config.cmd_timeout_ms);
  g_config.telem_period_ms = g_prefs.getUInt("telem_ms", g_config.telem_period_ms);
  g_config.torque_scale_min = g_prefs.getFloat("tq_min", g_config.torque_scale_min);
  g_config.torque_scale_slope = g_prefs.getFloat("tq_slope", g_config.torque_scale_slope);
  clampConfig(&g_config);

  // 水平校准偏置
  g_level_roll_offset_deg = g_prefs.getFloat("lvl_roll", g_level_roll_offset_deg);
  g_level_pitch_offset_deg = g_prefs.getFloat("lvl_pitch", g_level_pitch_offset_deg);
  g_level_yaw_offset_deg = g_prefs.getFloat("lvl_yaw", g_level_yaw_offset_deg);

  // 滤波器模式
  const uint32_t imu_filt = g_prefs.getUInt("imu_filt", (uint32_t)g_filter_mode);
  if (imu_filt <= (uint32_t)FILTER_COMP) {
    g_filter_mode = (FilterMode)imu_filt;
  }

  // 蜂鸣器设置（注意：这些函数在 hardware.ino 中实现）
  bool mute = g_prefs.getBool("buz_mute", false);
  setBuzzerMute(mute);
  uint8_t repeat = g_prefs.getUChar("to_repeat", 3);
  setTakeoffRepeat(repeat);

  g_prefs.end();
}

// ============================================================
// 保存所有参数到 NVS
// ============================================================
void saveTunings() {
  g_prefs.begin("drone", false);  // 读写模式打开命名空间 "drone"

  // PID 参数
  g_prefs.putFloat("a_kp", g_tunings.angle.kp);
  g_prefs.putFloat("a_ki", g_tunings.angle.ki);
  g_prefs.putFloat("a_kd", g_tunings.angle.kd);
  g_prefs.putFloat("r_kp", g_tunings.rate.kp);
  g_prefs.putFloat("r_ki", g_tunings.rate.ki);
  g_prefs.putFloat("r_kd", g_tunings.rate.kd);
  g_prefs.putFloat("y_kp", g_tunings.yaw.kp);
  g_prefs.putFloat("y_ki", g_tunings.yaw.ki);
  g_prefs.putFloat("y_kd", g_tunings.yaw.kd);

  // 运行时配置
  g_prefs.putFloat("max_ang", g_config.max_angle_deg);
  g_prefs.putFloat("max_yaw", g_config.max_yaw_rate_dps);
  g_prefs.putFloat("tilt_dis", g_config.tilt_disarm_deg);
  g_prefs.putUInt("cmd_to", g_config.cmd_timeout_ms);
  g_prefs.putUInt("telem_ms", g_config.telem_period_ms);
  g_prefs.putFloat("tq_min", g_config.torque_scale_min);
  g_prefs.putFloat("tq_slope", g_config.torque_scale_slope);

  // 水平校准偏置
  g_prefs.putFloat("lvl_roll", g_level_roll_offset_deg);
  g_prefs.putFloat("lvl_pitch", g_level_pitch_offset_deg);
  g_prefs.putFloat("lvl_yaw", g_level_yaw_offset_deg);

  // 滤波器模式
  g_prefs.putUInt("imu_filt", (uint32_t)g_filter_mode);

  // 蜂鸣器设置
  g_prefs.putBool("buz_mute", isBuzzerMuted());
  g_prefs.putUChar("to_repeat", getTakeoffRepeat());

  g_prefs.end();
}