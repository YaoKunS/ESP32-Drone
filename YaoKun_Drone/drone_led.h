#pragma once

/**
 * @file drone_led.h
 * @brief 红绿蓝三色 LED 驱动（红绿使用 PWM 呼吸/闪烁，蓝色使用 GPIO 数字控制）

 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 *        开源工程页：https://oshwhub.com/lindahe/project_sulkgarf

 * 本模块提供 LED 初始化、状态更新函数，通过全局变量 g_led_mode 控制显示模式。
 * 注意：蓝色 LED 由主循环中的独立逻辑控制（慢闪/常亮/快闪），不在此模块管理。
 * 
 * 重要设计：红绿 LED 的 PWM 使用固定 LEDC 通道（红色 4，绿色 5），与电机通道（0~3）完全隔离。
 * 写入时直接使用 ledcWrite(pin, duty) 通过引脚号写入，ESP32 会自动映射到对应通道，
 * 从而避免通道冲突，确保电机和 LED 互不干扰。
 */

#include <Arduino.h>
#include <esp32-hal-ledc.h>
#include <math.h>
#include "config.h"           // 获取 led1, led2, led3 引脚及 PWM 常量

// ============================================================
// 飞控状态对应的 LED 模式（由主程序设置 g_led_mode）
// ============================================================
/**
 * @brief LED 模式（红绿双色指示灯的状态）
 */
enum LedMode : uint8_t {
  LED_BOOT = 0,       // 系统启动中，红绿交替闪烁
  LED_DISARMED = 1,   // 已上锁（待机），红绿呼吸渐变
  LED_ARMING = 2,     // 解锁等待中，红绿同步慢闪
  LED_ARMED = 3,      // 已解锁（飞行中），红绿常亮
  LED_FAILSAFE = 4    // 故障保护，红绿快速同步闪烁
};

// ============================================================
// 外部变量声明（定义在 hardware.ino 中）
// ============================================================
extern LedMode g_led_mode;       // 当前 LED 模式（由主程序更新）

// ============================================================
// 内部辅助函数（static inline 避免多重定义，且无链接冲突）
// ============================================================

/** 限幅函数（浮点） */
static inline float led_clampf(float v, float vmin, float vmax) {
    if (v < vmin) return vmin;
    if (v > vmax) return vmax;
    return v;
}

/**
 * @brief 通过 LEDC 直接写入指定引脚的 PWM 占空比（使用引脚号）
 * @param pin   GPIO 引脚号（例如 led1, led2, kLedSinglePin）
 * @param level 亮度值，0.0（灭）~ 1.0（最亮）
 *
 * 说明：ESP32 的 ledcWrite 函数支持直接传入引脚号，内部会自动查找该引脚绑定的 LEDC 通道。
 * 由于在 hardware.ino 的 ledInit() 中已经将 led1、led2 分别绑定到通道 4 和 5，
 * 因此这里直接写引脚号是安全的，且不会影响电机（电机使用通道 0~3）。
 *
 * 🔴 负引脚保护（2026-11-16 加）：config.h 里"不存在的灯"用 -1 表示
 *   （火龙果 2.0 只有一颗灯，led1/led2/led3 全是 -1）。若不判空，
 *   ledcWrite(-1, ...) 会去操作一个不存在的通道号，轻则无效重则越界写寄存器。
 *   所有写灯入口都必须经过这里或 ledOutput()，不允许在别处直接 ledcWrite。
 */
static inline void ledWriteLevel(int pin, float level) {
    if (pin < 0) return;    // -1 = 该灯在本板型上不存在
    const float l = led_clampf(level, 0.0f, 1.0f);
    const uint32_t duty = (uint32_t)lrintf(l * (float)kLedPwmMaxDuty);
    ledcWrite(pin, duty);   // 直接通过引脚号输出 PWM
}

// ------------------------------------------------------------
// 三路逻辑灯状态的"影子副本"
// 为什么需要：单灯板型要把红/绿/蓝三路合成到一颗灯上，而三路是由两个
// 不同入口分别设置的（setLeds 每周期刷红绿、setBlueLed 每 50ms 刷蓝）。
// 任何一方变化时都必须知道另外两路当前是什么值，否则会互相覆盖
// （典型现象：蓝灯逻辑一跑，红绿呼吸就被刷灭）。
// ------------------------------------------------------------
static float g_led_red_level   = 0.0f;   // 最近一次设定的红色亮度 0~1
static float g_led_green_level = 0.0f;   // 最近一次设定的绿色亮度 0~1
static bool  g_led_blue_state  = false;  // 最近一次设定的蓝色数字状态

/**
 * @brief 三路逻辑状态 → 实际物理灯的唯一输出口
 * @param red   红色亮度 0~1（S3：红色 LED 的 PWM）
 * @param green 绿色亮度 0~1（S3：绿色 LED 的 PWM）
 * @param blue  蓝色数字状态（S3：蓝色 LED 的高/低电平）
 *
 * 单灯板型（kLedSingle=1，火龙果 2.0）的合成规则：
 *   亮度 = max(红, 绿, 蓝亮?1:0) —— "任一路要亮就亮"。
 *   为什么不取平均：三路语义是"任一告警/状态就要看见"，平均会让
 *   故障快闪在呼吸灯上被拉暗，现场根本看不清。
 *   各状态在单灯上的表现：BOOT = 亮/暗交替（见 ledUpdate 的 kLedSingle 分支，
 *   因为两颗灯交替在单灯上没有意义，改成亮度两态）；上锁 = 呼吸；解锁等待 =
 *   慢闪；已解锁 = 常亮；故障 = 快闪。闪频/亮度语义全部保留。
 *
 * 调用时机：setLeds() / setBlueLed() / 电压告警覆盖（applyVoltageLed）内部。
 */
static inline void ledOutput(float red, float green, bool blue) {
#if kLedSingle
    // ---------- 单灯板型：三路合成一颗灯 ----------
    float level = (red > green) ? red : green;
    if (blue && level < 1.0f) level = 1.0f;   // 蓝灯是数字语义，按最亮处理
    ledWriteLevel(kLedSinglePin, level);
#else
    // ---------- 三灯板型（S3 及 mini 系列）：各写各的 ----------
    ledWriteLevel(led1, red);
    ledWriteLevel(led2, green);
    if (led3 >= 0) digitalWrite(led3, blue ? HIGH : LOW);
#endif
}

/** 同时设置红、绿 LED 的亮度（PWM）；蓝灯保持上次状态 */
static inline void setLeds(float l1, float l2) {
    g_led_red_level = l1;
    g_led_green_level = l2;
    ledOutput(l1, l2, g_led_blue_state);
}

/**
 * @brief 蓝色 LED（状态灯）独立控制
 * @param on true=亮, false=灭
 *
 * 在单灯板型上这是一次"叠加"而不是"独占"：蓝灯亮时单灯取最亮，
 * 红绿亮度仍被记住，蓝灯灭后自动恢复（见 g_led_* 影子副本）。
 */
static inline void setBlueLed(bool on) {
    g_led_blue_state = on;
    ledOutput(g_led_red_level, g_led_green_level, on);
}

/**
 * @brief 呼吸灯效果计算（正弦波，0→1→0 循环）
 * @param now_ms     当前毫秒时间戳
 * @param period_ms  完整呼吸周期（毫秒）
 * @return 亮度值（0~1）
 */
static inline float breatheLevel(uint32_t now_ms, uint32_t period_ms) {
    const float phase = (float)(now_ms % period_ms) / (float)period_ms;
    return 0.5f - 0.5f * cosf(2.0f * (float)M_PI * phase);
}

// ============================================================
// 对外接口函数声明（实现位于 hardware.ino）
// ============================================================

/**
 * @brief LED 硬件初始化（配置 LEDC 通道，设置引脚模式）
 * 在 setup() 中调用一次。
 */
void ledInit();

/**
 * @brief LED 状态更新（根据 g_led_mode 刷新红绿 LED 显示）
 * 在 loop() 中周期性调用。
 */
void ledUpdate();

/* 红、绿、蓝：
根据代码中的 ledUpdate() 函数和 LedMode 枚举，红绿灯的状态含义如下：
🔴 红色 LED 与 🟢 绿色 LED 状态说明
飞控状态 (g_led_mode)	红色 LED 行为	绿色 LED 行为	含义
LED_BOOT (启动中)	闪烁（交替）	闪烁（交替）	系统启动，红绿交替闪烁（周期 220ms，红亮绿灭 → 红灭绿亮 → 循环）
LED_DISARMED (已上锁/待机)	呼吸灯渐变	呼吸灯渐变	飞控已上锁，等待解锁。红绿灯同步呼吸（亮度 0→1→0 循环，周期 1.4 秒）
LED_ARMING (解锁等待)	同步慢闪	同步慢闪	正在执行解锁操作（需要保持解锁开关一段时间），红绿灯同时慢闪（周期 300ms）
LED_ARMED (已解锁)	常亮	常亮	飞控已解锁，可以起飞。红绿灯同时常亮
LED_FAILSAFE (故障保护)	同步快闪	同步快闪	发生故障（如遥控超时、IMU失效等），红绿灯同时快速闪烁（周期 120ms）

 * ============================================================
 * 红绿灯状态说明（根据 g_led_mode 显示）
 * ============================================================
 * +-------------------+----------------+----------------+----------------------------------+
 * | 模式 (g_led_mode) | 红色 LED        | 绿色 LED        | 含义                             |
 * +-------------------+----------------+----------------+----------------------------------+
 * | LED_BOOT          | 交替闪烁        | 交替闪烁        | 系统启动，红绿交替，周期 220ms    |
 * | LED_DISARMED      | 呼吸渐变        | 呼吸渐变        | 待机上锁，呼吸灯，周期 1.4s       |
 * | LED_ARMING        | 同步慢闪        | 同步慢闪        | 解锁保持等待，周期 300ms          |
 * | LED_ARMED         | 常亮            | 常亮            | 已解锁，可起飞                    |
 * | LED_FAILSAFE      | 同步快闪        | 同步快闪        | 故障保护，周期 120ms              |
 * +-------------------+----------------+----------------+----------------------------------+
 * 注意：蓝色 LED 由独立逻辑控制（慢闪/常亮/快闪），不在此表内。
 * ============================================================

📌 总结
红绿灯是同步动作的（除了启动时的交替闪烁，其余状态都是同时亮/灭）。
呼吸灯：柔和渐变效果，表示待机。
慢闪：正在执行解锁操作（需要等待保持）。
常亮：已解锁，可飞行。
快闪：故障告警。

🔵 蓝色 LED 的独立状态
蓝色 LED 由额外添加的逻辑控制，与红绿灯无关：
慢闪：待机（无手机连接，未起飞）
常亮：手机已连接 WebSocket（网页打开）
快闪：正在飞行（已解锁且油门 >5%）
这样可以区分：红绿灯表示飞控核心状态（解锁/故障等），蓝灯表示遥控连接状态和飞行状态。
*/