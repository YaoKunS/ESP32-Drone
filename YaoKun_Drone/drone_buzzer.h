#pragma once

/**
 * @file drone_buzzer.h
 * @brief 蜂鸣器驱动（支持单音、循环报警、起飞音效重复、电压报警等）

 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 
 * 本模块提供非阻塞的蜂鸣器控制，所有音效通过 buzzerPlay() 触发，
 * 在 loop() 中周期性调用 buzzerUpdate() 实现状态机刷新。
 * 支持全局静音、起飞音效重复次数设置。
 * 
 * 在模块化拆分后，本文件仅保留函数声明、外部变量声明和 static inline 辅助函数。
 * 具体的实现（全局变量定义、函数体）已移至 hardware.ino。
 */

#include <Arduino.h>
#include "config.h"   // 获取 BUZZER_PIN 及系统常量

#define BUZZER_ACTIVE_HIGH   true   // 蜂鸣器高电平有效（可根据硬件修改）

// ============================================================
// 外部变量声明（定义在 hardware.ino 中）
// ============================================================
extern BuzzerPattern g_buzzer_pattern;   // 当前播放的音效
extern uint32_t g_buzzer_start_ms;       // 音效开始时间戳（毫秒）
extern uint8_t g_buzzer_sub_state;       // 多段音效的子状态（如双短鸣的步数）
extern uint32_t g_buzzer_until_ms;       // 当前子状态结束时间戳

extern bool g_buzzer_muted;              // 全局静音开关（true=静音）
extern uint8_t g_takeoff_repeat;         // 起飞音效重复次数（默认3次，范围1~10）

// ============================================================
// 底层驱动（static inline 可安全用于头文件）
// ============================================================

/** 硬件写入：根据静音标志决定是否驱动蜂鸣器 */
static inline void buzzerSet(bool on) {
    if (g_buzzer_muted) return;
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

/** 立即停止所有音效 */
static inline void buzzerStop() {
    g_buzzer_pattern = BUZ_SILENT;
    buzzerSet(false);
}

// ============================================================
// 对外接口函数声明（实现位于 hardware.ino）
// ============================================================

/** 蜂鸣器硬件初始化（设置引脚模式，初始为低电平） */
void buzzerInit();

/** 蜂鸣器状态更新（非阻塞，必须在 loop() 中周期性调用） */
void buzzerUpdate();

/**
 * @brief 播放指定音效（非阻塞）
 * @param pattern 音效枚举（见 BuzzerPattern）
 */
void buzzerPlay(BuzzerPattern pattern);

/** 设置全局静音状态（true=静音，false=允许发声） */
void setBuzzerMute(bool mute);

/** 获取当前静音状态 */
bool isBuzzerMuted();

/** 设置起飞音效重复次数（范围1~10，超出自动修正） */
void setTakeoffRepeat(uint8_t times);

/** 获取起飞音效重复次数 */
uint8_t getTakeoffRepeat();

/** 停止循环报警音效（用于退出故障或解锁等待状态） */
void buzzerStopLoop();