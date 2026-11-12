/**
 * @file failsafe.ino  
 * @brief 故障保护与解锁状态机（解锁/上锁、紧急停止、故障处理）20260612
 * 
 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）

 * 本文件包含飞控的安全核心逻辑：
 *   - 解锁条件判断 (canArm)
 *   - 解锁状态机 (updateArmingState)
 *   - 上锁/紧急停止 (disarmNow, requestKill)
 * 
 * 这些函数被主控制循环 controlStep() 调用。
 */

#include "drone_types.h"
#include "config.h"
#include "drone_led.h"
#include "drone_buzzer.h"
#include "drone_pid.h"

// ============================================================
// 故障保护相关全局变量定义
// ============================================================
bool g_armed = false;
bool g_arm_inhibit = false;    // 解锁禁止（故障后）
uint32_t g_arm_hold_start_ms = 0;  // 解锁保持开始时间

uint8_t g_last_fs = FS_NONE;   // 最后一次故障原因
volatile bool g_kill_pending = false;      // 紧急停止请求标志
volatile uint8_t g_kill_reason = FS_KILL;  // 紧急停止原因
portMUX_TYPE g_killMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================
// 故障保护函数实现
// ============================================================

/**
 * @brief 请求紧急停止（触发故障保护）
 * @param reason 故障原因（FailSafeReason）
 */
void requestKill(uint8_t reason) {
  portENTER_CRITICAL(&g_killMux);
  g_kill_pending = true;
  g_kill_reason = reason;
  portEXIT_CRITICAL(&g_killMux);
}

/**
 * @brief 立即上锁（停止电机，进入故障保护状态）
 * @param reason 故障原因
 */
void disarmNow(uint8_t reason) {
  g_armed = false;
  g_takeoff_sounded = false;               // 重置起飞音效标志（注意：g_takeoff_sounded 在主文件中，需 extern）
  // 紧急停止时不播放短鸣（已在 controlStep 中播放了 BUZ_KILL）
  if (reason != FS_KILL) {
    buzzerPlay(BUZ_SHORT_BEEP);            // 上锁短鸣
  }
  buzzerStopLoop();                        // 停止循环报警
  g_arm_inhibit = true;
  g_arm_hold_start_ms = 0;
  g_last_fs = reason;
  setStatusMsgFs(reason);                  // 设置状态消息（需 extern）
  resetAllPid();                           // 来自 control.ino
  motorsWriteAll(0.0f, 0.0f, 0.0f, 0.0f);  // 电机停转
}

// ============================================================================
// 解锁条件判断与状态机
// ============================================================================
/**
 * @brief 检查是否满足解锁条件
 * @param rc 当前遥控指令
 * @return true 表示可以解锁，false 表示禁止解锁
 */
bool canArm(const RcCommand &rc) {
  if (rc.throttle > kArmThrottleMax) return false;
  if (fabsf(rc.roll) > kStickCenterMax || fabsf(rc.pitch) > kStickCenterMax || fabsf(rc.yaw) > kStickCenterMax) return false;
  if (fabsf(g_roll_deg) > 45.0f || fabsf(g_pitch_deg) > 45.0f) return false;
  return true;
}

/**
 * @brief 解锁/上锁状态机（在 controlStep 中周期性调用）
 * @param rc 当前遥控指令
 * @param now_ms 当前毫秒时间戳
 */
void updateArmingState(const RcCommand &rc, uint32_t now_ms) {
  const bool has_fs = (g_last_fs != FS_NONE);
  if (!rc.arm_request && rc.throttle <= kArmThrottleMax) g_arm_inhibit = g_motor_fault ? true : false;
  if (!g_armed) {
    motorsWriteAll(0.0f, 0.0f, 0.0f, 0.0f);
    if (g_motor_fault) { g_arm_hold_start_ms = 0; return; }
    if (g_arm_inhibit) { g_arm_hold_start_ms = 0; return; }
    if (rc.arm_request) {
      if (rc.throttle > kArmThrottleMax) g_arm_hold_start_ms = 0;
      else if (fabsf(rc.roll) > kStickCenterMax || fabsf(rc.pitch) > kStickCenterMax || fabsf(rc.yaw) > kStickCenterMax) g_arm_hold_start_ms = 0;
      else if (fabsf(g_roll_deg) > 45.0f || fabsf(g_pitch_deg) > 45.0f) g_arm_hold_start_ms = 0;
      else if (canArm(rc)) {
        if (g_arm_hold_start_ms == 0) g_arm_hold_start_ms = now_ms;
        if ((now_ms - g_arm_hold_start_ms) >= kArmHoldMs) {
          g_armed = true; g_last_fs = FS_NONE; setStatusMsg("ARMED"); resetAllPid();
          buzzerPlay(BUZ_LONG_BEEP);   // 解锁成功长鸣
          buzzerStopLoop();            // 停止解锁等待的滴答声
        }
      }
    } else g_arm_hold_start_ms = 0;
    return;
  }
  if (!rc.arm_request) disarmNow(FS_MANUAL);
}
