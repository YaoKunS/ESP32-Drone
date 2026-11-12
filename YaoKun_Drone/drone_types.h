// ============================================================================
// 文件名：drone_types.h
// 描述：全局类型定义、枚举、工具函数、跨模块函数声明
//       本文件作为飞控项目的“公共头文件”，所有模块都应包含它。
//       它不包含任何硬件相关定义（引脚等），只定义数据类型和接口。

//       重要：硬件模块函数（ledInit, buzzerInit, motorInit 等）声明为普通外部函数，
//             它们的实现在 hardware.ino 中。
//             多数控制、故障保护、遥测函数已拆分为独立模块，并在本文件中提供了 extern 声明。
//             仅有 controlStep 等少数函数仍保留 static，因其完全封装在主文件内部。
// ============================================================================
#ifndef DRONE_TYPES_H_
#define DRONE_TYPES_H_

#include <Arduino.h>
#include <math.h>
#include "config.h"
// ============================================================================
// 第二部分：工具函数（内联，避免重复定义）
// ============================================================================

/**
 * @brief 单精度浮点数限幅
 * @param v 输入值
 * @param vmin 最小值
 * @param vmax 最大值
 * @return 限幅后的值
 */
static inline float clampf(float v, float vmin, float vmax) {
  if (v < vmin) return vmin;
  if (v > vmax) return vmax;
  return v;
}

/**
 * @brief 32 位无符号整数限幅
 * @param v 输入值
 * @param vmin 最小值
 * @param vmax 最大值
 * @return 限幅后的值
 */
static inline uint32_t clampu32(uint32_t v, uint32_t vmin, uint32_t vmax) {
  if (v < vmin) return vmin;
  if (v > vmax) return vmax;
  return v;
}

/**
 * @brief 判断浮点数是否为有限值（非无穷、非 NaN）
 * @param v 输入值
 * @return true 为有限值，false 为无穷或 NaN
 */
static inline bool isFinitef(float v) {
  return isfinite(v) != 0;
}

// ============================================================================
//把（#include "drone_pid.h"从文件开头（#include "config.h" 之后）
//移动到 clampf 等工具函数定义之后。否则可能提示编译错误！
//头文件依赖管理的基本准则：被依赖的定义应该放在依赖者之前。
// ============================================================================
#include "drone_pid.h"

// 前向声明（避免循环依赖，实际定义在其他文件或本文件后面）
class AsyncWebSocketClient;
class AsyncWebSocket;

// ============================================================================
// 第一部分：基础数据结构（原有注释完整保留）
// ============================================================================

/**
 * @brief 遥控指令结构体（从网页或接收机传来的控制量）
 */
struct RcCommand {
  float throttle;      // 油门：0 ~ 1，0 为最底，1 为最顶
  float roll;          // 横滚：-1 ~ 1，-1 左倾，1 右倾
  float pitch;         // 俯仰：-1 ~ 1，-1 抬头，1 低头
  float yaw;           // 偏航：-1 ~ 1，-1 左转，1 右转
  bool arm_request;    // 解锁请求（true 请求解锁，false 请求上锁）
  uint32_t last_ms;    // 最后收到指令的时间戳（毫秒）
};

/**
 * @brief IMU 原始数据样本（加速度计和陀螺仪）
 */
struct ImuSample {
  float ax_g;          // X 轴加速度（g）
  float ay_g;          // Y 轴加速度（g）
  float az_g;          // Z 轴加速度（g）
  float gx_dps;        // X 轴角速度（度/秒）
  float gy_dps;        // Y 轴角速度（度/秒）
  float gz_dps;        // Z 轴角速度（度/秒）
};

/**
 * @brief 运行时配置（从网页可调节的参数）
 */
struct RuntimeConfig {
  float max_angle_deg;         // 最大倾斜角度（度）
  float max_yaw_rate_dps;      // 最大偏航角速度（度/秒）
  float tilt_disarm_deg;       // 超过此角度自动上锁（度）
  uint32_t cmd_timeout_ms;     // 遥控指令超时时间（毫秒）
  uint32_t telem_period_ms;    // 遥测数据发送周期（毫秒）
  float torque_scale_min;      // 最小扭矩缩放系数（低油门时）
  float torque_scale_slope;    // 扭矩缩放系数随油门的斜率
};

/**
 * @brief PID 三参数组
 */
struct PidTriplet {
  float kp;            // 比例系数
  float ki;            // 积分系数
  float kd;            // 微分系数
};

/**
 * @brief 飞控 PID 调参组（角度环、角速率环、偏航环）
 */
struct Tunings {
  PidTriplet angle;    // 角度环 PID（横滚/俯仰）
  PidTriplet rate;     // 角速率环 PID（横滚/俯仰）
  PidTriplet yaw;      // 偏航角速率 PID
};

/**
 * @brief 电机测试状态（用于 Web 界面控制单个电机转动）
 */
struct MotorTestState {
  bool active;         // 电机测试是否激活
  uint8_t index;       // 电机索引 0~3
  float throttle;      // 测试油门（0 ~ 0.2）
  uint32_t until_ms;   // 测试结束时间戳（毫秒）
};


// ============================================================================
// 第三部分：全局枚举定义

// ============================================================
// 音效模式枚举
// ============================================================
/**
 * @brief 蜂鸣器音效枚举
 */
enum BuzzerPattern : uint8_t {
  BUZ_SILENT = 0,            // 静音
  BUZ_SHORT_BEEP,            // 短鸣 100ms
  BUZ_LONG_BEEP,             // 长鸣 400ms
  BUZ_QUICK_TICK,            // 快速滴答（解锁等待）
  BUZ_FAST_ALARM,            // 急促报警（故障）
  BUZ_DOUBLE_BEEP,           // 两短鸣（陀螺仪校准完成）
  BUZ_TRIPLE_BEEP,           // 三短鸣（水平校准完成）
  BUZ_POWER_ON,              // 上电鸣 200ms
  BUZ_READY,                 // 就绪鸣 100ms
  BUZ_MOTOR_TEST,            // 电机测试提示 50ms
  BUZ_KILL,                  // 紧急停止：长100ms + 短100ms
  BUZ_TAKEOFF,               // 起飞音效：重复多次双短鸣
  BUZ_VOLT_ALERT_SLOW,       // 慢速电压报警（周期1s）
  BUZ_VOLT_ALERT_MED,        // 中速电压报警（周期0.5s）
  BUZ_VOLT_ALERT_FAST        // 快速电压报警（周期0.2s）
};

/**
 * @brief 故障保护原因（用于记录上次故障类型）
 */
enum FailSafeReason : uint8_t {
  FS_NONE = 0,               // 无故障
  FS_CMD_TIMEOUT = 1,        // 遥控指令超时
  FS_WS_DISCONNECT = 2,      // WebSocket 断开
  FS_IMU_FAIL = 3,           // IMU 失效
  FS_TILT_LIMIT = 4,         // 倾斜角度超限
  FS_DT_LIMIT = 5,           // 控制循环时间超限
  FS_KILL = 6,               // 紧急停止（KILL 命令）
  FS_MANUAL = 7,             // 手动上锁
  FS_MOTOR_FAIL = 8          // 电机硬件故障
};

/**
 * @brief 校准请求类型
 */
enum CalRequest : uint8_t {
  CAL_REQ_NONE = 0,          // 无请求
  CAL_REQ_LEVEL = 1,         // 请求水平校准
  CAL_REQ_GYRO = 2           // 请求陀螺仪校准
};

/**
 * @brief 校准状态
 */
enum CalState : uint8_t {
  CAL_STATE_IDLE = 0,        // 空闲
  CAL_STATE_LEVEL = 1,       // 水平校准中
  CAL_STATE_GYRO = 2,        // 陀螺仪校准中
  CAL_STATE_FAIL = 3         // 校准失败
};

// ============================================================================
// 第四部分：跨模块函数声明
// ============================================================
// 电机相关全局变量（定义在 hardware.ino）
// ============================================================
extern float g_motor_last[4];
extern MotorTestState g_motor_test;
extern portMUX_TYPE g_motorTestMux;
extern bool g_motor_fault;

// ----------------------------- 故障保护模块相关全局变量（定义在 failsafe.ino） -----------------------------
extern bool g_armed;
extern bool g_arm_inhibit;
extern uint32_t g_arm_hold_start_ms;
extern uint8_t g_last_fs;
extern volatile bool g_kill_pending;
extern volatile uint8_t g_kill_reason;
extern portMUX_TYPE g_killMux;

// ----------------------------- 主文件中需要被故障保护模块访问的变量（去掉 static 后声明） -----------------------------
extern bool g_takeoff_sounded;
extern float g_roll_deg, g_pitch_deg;
extern float g_yaw_deg;   // 偏航角（若后续模块需要）

// 状态消息和校准状态（被 telemetry 和主文件使用）
extern char g_status_msg[96];
extern uint8_t g_cal_state;

// ----------------------------- 硬件模块（led, buzzer, motor, imu） -----------------------------
// 注意：这些函数实现在 hardware.ino 中，为普通外部函数（无 static）
void ledInit();
void ledUpdate();

void buzzerInit();
void buzzerUpdate();
void buzzerPlay(BuzzerPattern pattern);
void setBuzzerMute(bool mute);
bool isBuzzerMuted();
void setTakeoffRepeat(uint8_t times);
uint8_t getTakeoffRepeat();
void buzzerStopLoop();

bool motorInit();
void motorsWriteAll(float m0, float m1, float m2, float m3);
bool startMotorTest(uint8_t index, float throttle, uint32_t duration_ms);
void stopMotorTest();
bool getMotorTestState(MotorTestState *out);

// ----------------------------- WebSocket 对象（定义在主文件） -----------------------------
extern AsyncWebSocket g_ws;
// WebSocket 连接标志（定义在主文件）
extern bool g_websocket_connected;

// ----------------------------- 参数存储模块（定义在 storage.ino） -----------------------------
extern RuntimeConfig g_config;
extern Tunings g_tunings;
extern float g_level_roll_offset_deg;
extern float g_level_pitch_offset_deg;
extern float g_level_yaw_offset_deg;
extern volatile bool g_tunings_dirty;
extern portMUX_TYPE g_cfgMux;
extern FilterMode g_filter_mode;

void loadTunings();
void saveTunings();

// ----------------------------- 控制算法模块（实现在 control.ino） -----------------------------
extern Pid g_roll_angle_pid, g_pitch_angle_pid;
extern Pid g_roll_rate_pid, g_pitch_rate_pid, g_yaw_rate_pid;

void resetAllPid();
void applyTunings(const Tunings &t);
void applyTuningsIfDirty();
// runControlLoop 未实现，预留接口
static void runControlLoop(float dt, const ImuSample &imu, const RcCommand &rc,
                    const RuntimeConfig &cfg, float roll_deg, float pitch_deg,
                    float *motor_out);

// ----------------------------- 故障保护模块（实现在 failsafe.ino） -----------------------------
void disarmNow(uint8_t reason);
void requestKill(uint8_t reason);
void updateArmingState(const RcCommand &rc, uint32_t now_ms);

// ----------------------------- 遥控指令模块（实现在 rc.ino） -----------------------------
extern RcCommand g_rc;
extern portMUX_TYPE g_rcMux;
RcCommand getRcCommand();
void setRcCommand(float throttle, float roll, float pitch, float yaw, bool arm_request);

// ----------------------------- 遥测数据发送（实现在 telemetry.ino） -----------------------------
extern uint32_t g_last_telem_ms;
void sendTelemetry(float dt, const RcCommand &rc, uint32_t cmd_age_ms, const RuntimeConfig &cfg);

// ----------------------------- WebSocket 命令处理（实现在 rc.ino）-----------------------------
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

// ----------------------------- 主文件辅助函数（部分已拆分，仍在主文件中） -----------------------------
void setStatusMsg(const char *msg);
void setStatusMsgFs(uint8_t reason);
// 电池电压读取（被遥测模块调用）
float readVbatt();
// 遥测模块用到的函数
const char* calStateLabel(uint8_t state);
const char* failSafeLabel(uint8_t reason);
const char* motorIndexLabel(uint8_t index);
// 校准请求函数,（改为非 static 声明）
void requestCalibration(uint8_t req);
// 以下仍保持 static（实现仍在主文件）
static uint8_t takeCalibrationRequest();
static void setCalState(uint8_t state, uint32_t hold_ms);
static void updateCalState(uint32_t now_ms);
static void updateVoltageAlert();
static void applyVoltageLed();

#endif // DRONE_TYPES_H_