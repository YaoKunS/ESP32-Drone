/**
 * @file control.ino
 * @brief 飞控控制算法（PID 控制器、参数应用、混控辅助）20260612
 * @brief 淘宝店: 耀坤智联 https://shop35432590.taobao.com/
 * @brief 提供本项目所需电子元器件套件，基础元件仅需9.9元 ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）

 * 本文件包含 PID 对象定义、参数应用函数和复位函数。
 * 控制主循环 controlStep() 仍在主文件中，后续可进一步拆分。
 */

#include "drone_types.h"  // 必须最先，因为它提供了 clampf 等基础工具
#include "config.h"       
#include "drone_pid.h"     // PID 类

// ============================================================
// PID 对象定义（角度环 + 角速率环）
// ============================================================
Pid g_roll_angle_pid, g_pitch_angle_pid;      // 角度环 PID
Pid g_roll_rate_pid, g_pitch_rate_pid, g_yaw_rate_pid; // 角速率环 PID

// ============================================================
// PID 参数应用与复位函数
// ============================================================

/**
 * @brief 复位所有 PID 控制器（清零积分、微分项）
 */
void resetAllPid() {
  g_roll_angle_pid.reset();
  g_pitch_angle_pid.reset();
  g_roll_rate_pid.reset();
  g_pitch_rate_pid.reset();
  g_yaw_rate_pid.reset();
}

/**
 * @brief 将调参结构体的参数应用到 PID 控制器
 * @param t 调参结构体（包含角度环、角速率环、偏航环的 Kp/Ki/Kd）
 */
void applyTunings(const Tunings &t) {
  // 角度环（横滚/俯仰共用一套参数）
  g_roll_angle_pid.setGains(t.angle.kp, t.angle.ki, t.angle.kd);
  g_pitch_angle_pid.setGains(t.angle.kp, t.angle.ki, t.angle.kd);
  g_roll_angle_pid.setOutputLimits(-250.0f, 250.0f);
  g_pitch_angle_pid.setOutputLimits(-250.0f, 250.0f);

  // 角速率环（横滚/俯仰共用一套参数）
  g_roll_rate_pid.setGains(t.rate.kp, t.rate.ki, t.rate.kd);
  g_pitch_rate_pid.setGains(t.rate.kp, t.rate.ki, t.rate.kd);
  g_roll_rate_pid.setOutputLimits(-0.5f, 0.5f);
  g_pitch_rate_pid.setOutputLimits(-0.5f, 0.5f);

  // 偏航角速率环
  g_yaw_rate_pid.setGains(t.yaw.kp, t.yaw.ki, t.yaw.kd);
  g_yaw_rate_pid.setOutputLimits(-0.3f, 0.3f);
}

/**
 * @brief 检查调参脏标志，若脏则应用新参数并复位 PID
 * 
 * 此函数在控制循环开始时调用，确保 PID 参数热更新生效。
 * 使用互斥锁保护 g_tunings_dirty 和 g_tunings 的读取。
 */
void applyTuningsIfDirty() {
  bool dirty = false;
  Tunings t;
  portENTER_CRITICAL(&g_cfgMux);
  if (g_tunings_dirty) {
    t = g_tunings;
    g_tunings_dirty = false;
    dirty = true;
  }
  portEXIT_CRITICAL(&g_cfgMux);
  if (dirty) {
    applyTunings(t);
    resetAllPid();
  }
}
