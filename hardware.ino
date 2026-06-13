/**
 * @file hardware.ino
 * @brief 硬件驱动实现（LED、蜂鸣器、电机、IMU）,20260612
 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 * 本文件包含所有底层硬件相关的函数实现和全局变量定义。
 * 这些函数在 drone_types.h 中声明，供主程序调用。
 * 模块化拆分后，所有硬件代码集中在此，便于维护和移植。
 
 */

#include "drone_types.h"
#include "drone_led.h"
#include "drone_buzzer.h"
#include "drone_imu.h"
#include "config.h"

// ============================================================
// 1. LED 硬件实现
// ============================================================

// LED 全局变量定义
LedMode g_led_mode = LED_BOOT; // 当前 LED 模式

/**
 * @brief LED 硬件初始化
 * 
 * 功能：
 *   - 将红色、绿色 LED 引脚分别绑定到 LEDC 通道 4 和 5（与电机通道 0~3 完全隔离）
 *   - 将蓝色 LED 引脚配置为普通 GPIO 输出
 *   - 初始全部熄灭
 * 
 * 注意：后续写入 PWM 时直接使用 ledcWrite(pin, duty) 通过引脚号输出，
 *       ESP32 内部会自动根据引脚找到对应的通道（4 或 5），
 *       从而与电机通道完全分离，避免资源冲突。
 */
void ledInit() {
    // 红色 LED 绑定到通道 4，绿色 LED 绑定到通道 5
    // 使用 ledcAttachChannel 手动指定通道，确保与电机通道（0~3）不重叠
    ledcAttachChannel(led1, kLedPwmFreqHz, kLedPwmResolutionBits, 4);
    ledcAttachChannel(led2, kLedPwmFreqHz, kLedPwmResolutionBits, 5);

    // 初始化蓝色 LED 为普通 GPIO 输出，初始熄灭
    pinMode(led3, OUTPUT);
    digitalWrite(led3, LOW);

    // 初始全部熄灭
    setLeds(0.0f, 0.0f);
    setBlueLed(false);
}

/**
 * @brief LED 状态更新函数
 * 
 * 根据全局变量 g_led_mode 的值，设置红绿 LED 的显示模式。
 * 此函数在 loop() 中周期性调用（约 500Hz），实现非阻塞呼吸、闪烁效果。
 */
void ledUpdate() {
    const uint32_t now_ms = millis();   // 当前毫秒时间戳

    switch (g_led_mode) {
        // ---------- 启动模式：红绿交替闪烁 ----------
        case LED_BOOT:
            {
                const uint32_t period_ms = 220;   // 交替周期 220ms
                // 每 220ms 切换一次，right_on 为 true 时红色亮绿色灭，false 时红色灭绿色亮
                const bool right_on = ((now_ms / period_ms) & 1U) == 0U;
                setLeds(right_on ? 1.0f : 0.0f, right_on ? 0.0f : 1.0f);
            }
            break;

        // ---------- 故障保护模式：红绿同时快速闪烁 ----------
        case LED_FAILSAFE:
            {
                const uint32_t period_ms = 120;   // 闪烁周期 120ms
                const bool on = ((now_ms / period_ms) & 1U) == 0U;
                const float level = on ? 1.0f : 0.0f;
                setLeds(level, level);            // 红绿同亮同灭
            }
            break;

        // ---------- 解锁等待模式：红绿同时慢闪 ----------
        case LED_ARMING:
            {
                const uint32_t period_ms = 300;   // 闪烁周期 300ms
                const bool on = ((now_ms / period_ms) & 1U) == 0U;
                const float level = on ? 1.0f : 0.0f;
                setLeds(level, level);
            }
            break;

        // ---------- 已解锁模式：红绿常亮 ----------
        case LED_ARMED:
            setLeds(1.0f, 1.0f);
            break;

        // ---------- 已上锁/待机模式：红绿同步呼吸灯（默认）----------
        case LED_DISARMED:
        default:
            {
                const float level = breatheLevel(now_ms, 1400);   // 呼吸周期 1.4 秒
                setLeds(level, level);
            }
            break;
    }
    // 注意：蓝色 LED 不在此处更新，由主循环中的独立逻辑控制
}

// ============================================================
// 2. 蜂鸣器硬件实现
// ============================================================

// 蜂鸣器全局变量定义
BuzzerPattern g_buzzer_pattern = BUZ_SILENT;
uint32_t g_buzzer_start_ms = 0;
uint8_t g_buzzer_sub_state = 0;
uint32_t g_buzzer_until_ms = 0;
bool g_buzzer_muted = false;
uint8_t g_takeoff_repeat = 3;

// 播放一段简单音效（单段固定时长）
static void buzzerPlaySingle(BuzzerPattern pattern, uint32_t duration_ms) {
    if (pattern == BUZ_SILENT) return;
    if (g_buzzer_muted) return;
    g_buzzer_pattern = pattern;
    g_buzzer_sub_state = 0;
    g_buzzer_start_ms = millis();
    g_buzzer_until_ms = g_buzzer_start_ms + duration_ms;
    buzzerSet(true);
}

void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    buzzerSet(false);
}

void buzzerUpdate() {
    uint32_t now = millis();
    
    // 简单单段音效
    if (g_buzzer_pattern == BUZ_SHORT_BEEP || g_buzzer_pattern == BUZ_LONG_BEEP ||
        g_buzzer_pattern == BUZ_POWER_ON || g_buzzer_pattern == BUZ_READY ||
        g_buzzer_pattern == BUZ_MOTOR_TEST) {
        if (now >= g_buzzer_until_ms) {
            buzzerStop();
        }
        return;
    }
    
    // 循环音效：快速滴答（解锁等待）
    if (g_buzzer_pattern == BUZ_QUICK_TICK) {
        uint32_t period = 200;
        bool on = (now % period) < 50;
        buzzerSet(on);
        return;
    }
    
    // 循环音效：急促报警（故障）
    if (g_buzzer_pattern == BUZ_FAST_ALARM) {
        uint32_t period = 160;
        bool on = (now % period) < 80;
        buzzerSet(on);
        return;
    }

    // 电压报警音效
    if (g_buzzer_pattern == BUZ_VOLT_ALERT_SLOW) {
        uint32_t period = 1000;
        bool on = (now % period) < 200;
        buzzerSet(on);
        return;
    }
    if (g_buzzer_pattern == BUZ_VOLT_ALERT_MED) {
        uint32_t period = 500;
        bool on = (now % period) < 100;
        buzzerSet(on);
        return;
    }
    if (g_buzzer_pattern == BUZ_VOLT_ALERT_FAST) {
        uint32_t period = 200;
        bool on = (now % period) < 50;
        buzzerSet(on);
        return;
    }
    
    // 双短鸣
    if (g_buzzer_pattern == BUZ_DOUBLE_BEEP) {
        if (g_buzzer_sub_state == 0) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                g_buzzer_sub_state = 1;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 1) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(true);
                g_buzzer_sub_state = 2;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 2) {
            if (now >= g_buzzer_until_ms) {
                buzzerStop();
            }
        }
        return;
    }
    
    // 三短鸣
    if (g_buzzer_pattern == BUZ_TRIPLE_BEEP) {
        if (g_buzzer_sub_state == 0) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                g_buzzer_sub_state = 1;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 1) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(true);
                g_buzzer_sub_state = 2;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 2) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                g_buzzer_sub_state = 3;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 3) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(true);
                g_buzzer_sub_state = 4;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 4) {
            if (now >= g_buzzer_until_ms) {
                buzzerStop();
            }
        }
        return;
    }
    
    // 紧急停止音效
    if (g_buzzer_pattern == BUZ_KILL) {
        if (g_buzzer_sub_state == 0) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                g_buzzer_sub_state = 1;
                g_buzzer_until_ms = now + 50;
            }
        } else if (g_buzzer_sub_state == 1) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(true);
                g_buzzer_sub_state = 2;
                g_buzzer_until_ms = now + 100;
            }
        } else if (g_buzzer_sub_state == 2) {
            if (now >= g_buzzer_until_ms) {
                buzzerStop();
            }
        }
        return;
    }
    
    // 起飞音效：重复多次双短鸣
    if (g_buzzer_pattern == BUZ_TAKEOFF) {
        uint8_t round = g_buzzer_sub_state / 3;
        uint8_t phase = g_buzzer_sub_state % 3;
        
        if (round >= g_takeoff_repeat) {
            buzzerStop();
            return;
        }
        
        if (phase == 0) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                g_buzzer_sub_state++;
                g_buzzer_until_ms = now + 100;
            }
        } else if (phase == 1) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(true);
                g_buzzer_sub_state++;
                g_buzzer_until_ms = now + 100;
            }
        } else if (phase == 2) {
            if (now >= g_buzzer_until_ms) {
                buzzerSet(false);
                if (round + 1 >= g_takeoff_repeat) {
                    buzzerStop();
                } else {
                    g_buzzer_sub_state = (round + 1) * 3;
                    g_buzzer_until_ms = now + 200;
                }
            }
        }
        return;
    }
}

void buzzerPlay(BuzzerPattern pattern) {
    if (g_buzzer_pattern != BUZ_SILENT && pattern != BUZ_KILL) {
        buzzerStop();
    }
    switch (pattern) {
        case BUZ_SHORT_BEEP:
            buzzerPlaySingle(pattern, 100);
            break;
        case BUZ_LONG_BEEP:
            buzzerPlaySingle(pattern, 400);
            break;
        case BUZ_POWER_ON:
            buzzerPlaySingle(pattern, 200);
            break;
        case BUZ_READY:
            buzzerPlaySingle(pattern, 100);
            break;
        case BUZ_MOTOR_TEST:
            buzzerPlaySingle(pattern, 50);
            break;
        case BUZ_DOUBLE_BEEP:
            g_buzzer_pattern = pattern;
            g_buzzer_sub_state = 0;
            g_buzzer_start_ms = millis();
            g_buzzer_until_ms = g_buzzer_start_ms + 100;
            buzzerSet(true);
            break;
        case BUZ_TRIPLE_BEEP:
            g_buzzer_pattern = pattern;
            g_buzzer_sub_state = 0;
            g_buzzer_start_ms = millis();
            g_buzzer_until_ms = g_buzzer_start_ms + 100;
            buzzerSet(true);
            break;
        case BUZ_KILL:
            g_buzzer_pattern = pattern;
            g_buzzer_sub_state = 0;
            g_buzzer_start_ms = millis();
            g_buzzer_until_ms = g_buzzer_start_ms + 100;
            buzzerSet(true);
            break;
        case BUZ_TAKEOFF:
            g_buzzer_pattern = pattern;
            g_buzzer_sub_state = 0;
            g_buzzer_start_ms = millis();
            g_buzzer_until_ms = g_buzzer_start_ms + 100;
            buzzerSet(true);
            break;
        case BUZ_QUICK_TICK:
        case BUZ_FAST_ALARM:
        case BUZ_VOLT_ALERT_SLOW:
        case BUZ_VOLT_ALERT_MED:
        case BUZ_VOLT_ALERT_FAST:
            buzzerStop();
            g_buzzer_pattern = pattern;
            break;
        default:
            break;
    }
}

void setBuzzerMute(bool mute) {
    g_buzzer_muted = mute;
    if (mute) {
        buzzerStop();
    }
}

bool isBuzzerMuted() {
    return g_buzzer_muted;
}

void setTakeoffRepeat(uint8_t times) {
    if (times < 1) times = 1;
    if (times > 10) times = 10;
    g_takeoff_repeat = times;
}

uint8_t getTakeoffRepeat() {
    return g_takeoff_repeat;
}

void buzzerStopLoop() {
    if (g_buzzer_pattern == BUZ_QUICK_TICK || g_buzzer_pattern == BUZ_FAST_ALARM ||
        g_buzzer_pattern == BUZ_VOLT_ALERT_SLOW || g_buzzer_pattern == BUZ_VOLT_ALERT_MED ||
        g_buzzer_pattern == BUZ_VOLT_ALERT_FAST) {
        buzzerStop();
    }
}

// ============================================================
// 3. 电机驱动实现
// ============================================================
// 电机相关全局变量（已在 drone_types.h 中声明为 extern）
float g_motor_last[4] = {0,0,0,0};
MotorTestState g_motor_test = {false,0,0.0f,0};
portMUX_TYPE g_motorTestMux = portMUX_INITIALIZER_UNLOCKED;
bool g_motor_fault = false;

// 电机测试状态变量（已在主文件中定义，但需要访问，此处不定义）
// 注意：g_motor_test, g_motorTestMux 等变量仍在主文件中，因为电机测试状态与主逻辑紧密相关。
// 我们只将底层硬件操作函数移到这里。

// ============================================================
// 3. 电机驱动实现（完整）
// ============================================================

bool motorInit() {
    bool ok0 = ledcAttachChannel(kMotor0Pin, kPwmFreqHz, kPwmResolutionBits, 0);
    bool ok1 = ledcAttachChannel(kMotor1Pin, kPwmFreqHz, kPwmResolutionBits, 1);
    bool ok2 = ledcAttachChannel(kMotor2Pin, kPwmFreqHz, kPwmResolutionBits, 2);
    bool ok3 = ledcAttachChannel(kMotor3Pin, kPwmFreqHz, kPwmResolutionBits, 3);
    Serial.printf("LEDC attach: %d %d %d %d\n", ok0, ok1, ok2, ok3);
    ledcWrite(kMotor0Pin, 0);
    ledcWrite(kMotor1Pin, 0);
    ledcWrite(kMotor2Pin, 0);
    ledcWrite(kMotor3Pin, 0);
    return ok0 && ok1 && ok2 && ok3;
}

static void motorWriteNorm(int pin, float u) {
    const float uu = clampf(u, 0.0f, 1.0f);
    const uint32_t duty = (uint32_t)lrintf(uu * (float)kPwmMaxDuty);
    ledcWrite(pin, duty);
}

void motorsWriteAll(float m0, float m1, float m2, float m3) {
    g_motor_last[0] = m0; g_motor_last[1] = m1; g_motor_last[2] = m2; g_motor_last[3] = m3;
    motorWriteNorm(kMotor0Pin, m0);
    motorWriteNorm(kMotor1Pin, m1);
    motorWriteNorm(kMotor2Pin, m2);
    motorWriteNorm(kMotor3Pin, m3);
}

void stopMotorTest() {
    bool was_active;
    portENTER_CRITICAL(&g_motorTestMux);
    was_active = g_motor_test.active;
    g_motor_test.active = false;
    g_motor_test.throttle = 0.0f;
    g_motor_test.until_ms = 0;
    portEXIT_CRITICAL(&g_motorTestMux);
    if (was_active) {
        buzzerPlay(BUZ_MOTOR_TEST);
    }
}

bool startMotorTest(uint8_t index, float throttle, uint32_t duration_ms) {
    if (index > 3) return false;
    const float thr = clampf(throttle, 0.0f, kMotorTestMaxThrottle);
    if (thr <= 0.0f) { stopMotorTest(); return false; }
    const uint32_t dur = clampu32(duration_ms, kMotorTestMinMs, kMotorTestMaxMs);
    const uint32_t until_ms = millis() + dur;
    portENTER_CRITICAL(&g_motorTestMux);
    g_motor_test.active = true;
    g_motor_test.index = index;
    g_motor_test.throttle = thr;
    g_motor_test.until_ms = until_ms;
    portEXIT_CRITICAL(&g_motorTestMux);
    buzzerPlay(BUZ_MOTOR_TEST);
    return true;
}

bool getMotorTestState(MotorTestState *out) {
    if (out == nullptr) return false;
    portENTER_CRITICAL(&g_motorTestMux);
    *out = g_motor_test;
    portEXIT_CRITICAL(&g_motorTestMux);
    return out->active;
}

// ============================================================
// 4. IMU 类方法实现
// ============================================================

// MadgwickImu 类方法
void MadgwickImu::begin(float beta) {
    beta_ = beta;
    q0_ = 1.0f;
    q1_ = 0.0f;
    q2_ = 0.0f;
    q3_ = 0.0f;
}

void MadgwickImu::setBeta(float beta) {
    if (beta > 0.0f) beta_ = beta;
}

void MadgwickImu::update(float gx_rads, float gy_rads, float gz_rads,
                         float ax, float ay, float az, float dt) {
    float q0 = q0_;
    float q1 = q1_;
    float q2 = q2_;
    float q3 = q3_;
    const float norm_a = sqrtf(ax * ax + ay * ay + az * az);
    if (norm_a < 1e-6f) {
        integrateGyroOnly(gx_rads, gy_rads, gz_rads, dt);
        return;
    }
    ax /= norm_a; ay /= norm_a; az /= norm_a;
    const float f1 = 2.0f * (q1 * q3 - q0 * q2) - ax;
    const float f2 = 2.0f * (q0 * q1 + q2 * q3) - ay;
    const float f3 = 2.0f * (0.5f - q1 * q1 - q2 * q2) - az;
    const float j_11or24 = -2.0f * q2;
    const float j_12or23 = 2.0f * q3;
    const float j_13or22 = -2.0f * q0;
    const float j_14or21 = 2.0f * q1;
    const float j_32 = 2.0f * j_14or21;
    const float j_33 = 2.0f * j_11or24;
    float s0 = j_11or24 * f1 + j_14or21 * f2;
    float s1 = j_12or23 * f1 - j_13or22 * f2 - j_32 * f3;
    float s2 = j_13or22 * f1 + j_12or23 * f2 + j_33 * f3;
    float s3 = j_14or21 * f1 - j_11or24 * f2;
    const float norm_s = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    if (norm_s > 1e-6f) {
        s0 /= norm_s; s1 /= norm_s; s2 /= norm_s; s3 /= norm_s;
    }
    const float qDot0 = 0.5f * (-q1 * gx_rads - q2 * gy_rads - q3 * gz_rads) - beta_ * s0;
    const float qDot1 = 0.5f * ( q0 * gx_rads + q2 * gz_rads - q3 * gy_rads) - beta_ * s1;
    const float qDot2 = 0.5f * ( q0 * gy_rads - q1 * gz_rads + q3 * gx_rads) - beta_ * s2;
    const float qDot3 = 0.5f * ( q0 * gz_rads + q1 * gy_rads - q2 * gx_rads) - beta_ * s3;
    q0 += qDot0 * dt; q1 += qDot1 * dt; q2 += qDot2 * dt; q3 += qDot3 * dt;
    const float norm_q = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm_q > 1e-6f) {
        q0 /= norm_q; q1 /= norm_q; q2 /= norm_q; q3 /= norm_q;
    }
    q0_ = q0; q1_ = q1; q2_ = q2; q3_ = q3;
}

void MadgwickImu::getEulerDeg(float *roll_deg, float *pitch_deg, float *yaw_deg) const {
    const float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;
    const float roll = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
    const float sinp = 2.0f * (q0 * q2 - q3 * q1);
    float pitch = 0.0f;
    if (fabsf(sinp) >= 1.0f) pitch = copysignf((float)M_PI / 2.0f, sinp);
    else pitch = asinf(sinp);
    const float yaw = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));
    *roll_deg = roll * 180.0f / (float)M_PI;
    *pitch_deg = pitch * 180.0f / (float)M_PI;
    *yaw_deg = yaw * 180.0f / (float)M_PI;
}

void MadgwickImu::setEulerDeg(float roll_deg, float pitch_deg, float yaw_deg) {
    const float roll = roll_deg * (float)M_PI / 180.0f;
    const float pitch = pitch_deg * (float)M_PI / 180.0f;
    const float yaw = yaw_deg * (float)M_PI / 180.0f;
    setEulerRad(roll, pitch, yaw);
}

void MadgwickImu::setEulerRad(float roll, float pitch, float yaw) {
    const float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    const float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    const float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
    q0_ = cr * cp * cy + sr * sp * sy;
    q1_ = sr * cp * cy - cr * sp * sy;
    q2_ = cr * sp * cy + sr * cp * sy;
    q3_ = cr * cp * sy - sr * sp * cy;
}

void MadgwickImu::integrateGyroOnly(float gx_rads, float gy_rads, float gz_rads, float dt) {
    float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;
    const float qDot0 = 0.5f * (-q1 * gx_rads - q2 * gy_rads - q3 * gz_rads);
    const float qDot1 = 0.5f * ( q0 * gx_rads + q2 * gz_rads - q3 * gy_rads);
    const float qDot2 = 0.5f * ( q0 * gy_rads - q1 * gz_rads + q3 * gx_rads);
    const float qDot3 = 0.5f * ( q0 * gz_rads + q1 * gy_rads - q2 * gx_rads);
    q0 += qDot0 * dt; q1 += qDot1 * dt; q2 += qDot2 * dt; q3 += qDot3 * dt;
    const float norm_q = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm_q > 1e-6f) {
        q0 /= norm_q; q1 /= norm_q; q2 /= norm_q; q3 /= norm_q;
    }
    q0_ = q0; q1_ = q1; q2_ = q2; q3_ = q3;
}

// MahonyImu 类方法
void MahonyImu::begin(float kp, float ki) {
    kp_ = kp; ki_ = ki; reset();
}

void MahonyImu::reset() {
    q0_ = 1.0f; q1_ = 0.0f; q2_ = 0.0f; q3_ = 0.0f;
    integral_fb_x_ = 0.0f; integral_fb_y_ = 0.0f; integral_fb_z_ = 0.0f;
}

void MahonyImu::setGains(float kp, float ki) {
    kp_ = kp; ki_ = ki;
}

void MahonyImu::setIntegralLimitRads(float limit_rads) {
    integral_limit_rads_ = fabsf(limit_rads);
}

void MahonyImu::setSpinRateLimitDps(float limit_dps) {
    spin_rate_limit_rads_ = fabsf(limit_dps) * (float)M_PI / 180.0f;
}

void MahonyImu::update(float gx_rads, float gy_rads, float gz_rads,
                       float ax, float ay, float az,
                       float dt, float acc_trust) {
    if (dt <= 0.0f) return;
    float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;
    float ex = 0.0f, ey = 0.0f, ez = 0.0f;
    const float norm_a = sqrtf(ax * ax + ay * ay + az * az);
    const float trust = clampf(acc_trust, 0.0f, 1.0f);
    const bool acc_ok = (norm_a > 1e-6f) && (trust > 0.0f);
    if (acc_ok) {
        ax /= norm_a; ay /= norm_a; az /= norm_a;
        const float vx = 2.0f * (q1 * q3 - q0 * q2);
        const float vy = 2.0f * (q0 * q1 + q2 * q3);
        const float vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
        ex = (ay * vz - az * vy);
        ey = (az * vx - ax * vz);
        ez = (ax * vy - ay * vx);
        ex *= trust; ey *= trust; ez *= trust;
        if (ki_ > 0.0f) {
            const float gyro_mag = sqrtf(gx_rads * gx_rads + gy_rads * gy_rads + gz_rads * gz_rads);
            const bool spin_ok = (spin_rate_limit_rads_ <= 0.0f) || (gyro_mag <= spin_rate_limit_rads_);
            if (spin_ok) {
                integral_fb_x_ += ki_ * ex * dt;
                integral_fb_y_ += ki_ * ey * dt;
                integral_fb_z_ += ki_ * ez * dt;
                if (integral_limit_rads_ > 0.0f) {
                    integral_fb_x_ = clampf(integral_fb_x_, -integral_limit_rads_, integral_limit_rads_);
                    integral_fb_y_ = clampf(integral_fb_y_, -integral_limit_rads_, integral_limit_rads_);
                    integral_fb_z_ = clampf(integral_fb_z_, -integral_limit_rads_, integral_limit_rads_);
                }
            }
        }
    }
    const float gx = gx_rads + kp_ * ex + integral_fb_x_;
    const float gy = gy_rads + kp_ * ey + integral_fb_y_;
    const float gz = gz_rads + kp_ * ez + integral_fb_z_;
    const float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    const float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    const float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    const float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);
    q0 += qDot0 * dt; q1 += qDot1 * dt; q2 += qDot2 * dt; q3 += qDot3 * dt;
    const float norm_q = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm_q > 1e-6f) {
        q0 /= norm_q; q1 /= norm_q; q2 /= norm_q; q3 /= norm_q;
    }
    q0_ = q0; q1_ = q1; q2_ = q2; q3_ = q3;
}

void MahonyImu::getEulerDeg(float *roll_deg, float *pitch_deg, float *yaw_deg) const {
    const float q0 = q0_, q1 = q1_, q2 = q2_, q3 = q3_;
    const float roll = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
    const float sinp = 2.0f * (q0 * q2 - q3 * q1);
    float pitch = 0.0f;
    if (fabsf(sinp) >= 1.0f) pitch = copysignf((float)M_PI / 2.0f, sinp);
    else pitch = asinf(sinp);
    const float yaw = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));
    *roll_deg = roll * 180.0f / (float)M_PI;
    *pitch_deg = pitch * 180.0f / (float)M_PI;
    *yaw_deg = yaw * 180.0f / (float)M_PI;
}

void MahonyImu::setEulerDeg(float roll_deg, float pitch_deg, float yaw_deg) {
    const float roll = roll_deg * (float)M_PI / 180.0f;
    const float pitch = pitch_deg * (float)M_PI / 180.0f;
    const float yaw = yaw_deg * (float)M_PI / 180.0f;
    setEulerRad(roll, pitch, yaw);
}

void MahonyImu::setEulerRad(float roll, float pitch, float yaw) {
    const float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    const float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    const float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
    q0_ = cr * cp * cy + sr * sp * sy;
    q1_ = sr * cp * cy - cr * sp * sy;
    q2_ = cr * sp * cy + sr * cp * sy;
    q3_ = cr * cp * sy - sr * sp * cy;
}

// Mpu6050 类方法
bool Mpu6050::begin() {
    Wire.begin(kI2cSda, kI2cScl, 400000);
    if (!writeReg(0x6B, 0x00)) return false;
    if (!writeReg(0x1A, 0x03)) return false;
    if (!writeReg(0x1B, 0x18)) return false;
    if (!writeReg(0x1C, 0x10)) return false;
    delay(50);
    return true;
}

void Mpu6050::calibrateGyro(uint16_t samples, uint16_t delay_ms) {
    float sumx = 0, sumy = 0, sumz = 0;
    for (uint16_t i = 0; i < samples; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        if (readRaw(&ax, &ay, &az, &gx, &gy, &gz)) {
            const float scale = 2000.0f / 32768.0f;
            sumx += gx * scale; sumy += gy * scale; sumz += gz * scale;
        }
        delay(delay_ms);
    }
    gyro_bias_x_ = sumx / samples;
    gyro_bias_y_ = sumy / samples;
    gyro_bias_z_ = sumz / samples;
}

bool Mpu6050::read(ImuSample *out) {
    int16_t ax, ay, az, gx, gy, gz;
    if (!readRaw(&ax, &ay, &az, &gx, &gy, &gz)) return false;
    const float a_scale = 8.0f / 32768.0f;
    const float g_scale = 2000.0f / 32768.0f;
    float ax_g = ax * a_scale, ay_g = ay * a_scale, az_g = az * a_scale;
    float gx_dps = gx * g_scale - gyro_bias_x_;
    float gy_dps = gy * g_scale - gyro_bias_y_;
    float gz_dps = gz * g_scale - gyro_bias_z_;
    if (kImuSwapXY) {
        float t_a = ax_g; ax_g = ay_g; ay_g = t_a;
        float t_g = gx_dps; gx_dps = gy_dps; gy_dps = t_g;
        gx_dps = -gx_dps; gy_dps = -gy_dps;
    }
    if (kImuFlipX) { ax_g = -ax_g; gx_dps = -gx_dps; }
    if (kImuFlipY) { ay_g = -ay_g; gy_dps = -gy_dps; }
    if (kImuFlipZ) { az_g = -az_g; gz_dps = -gz_dps; }
    out->ax_g = ax_g; out->ay_g = ay_g; out->az_g = az_g;
    out->gx_dps = gx_dps; out->gy_dps = gy_dps; out->gz_dps = gz_dps;
    return true;
}

bool Mpu6050::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(kMpuAddr);
    Wire.write(reg); Wire.write(val);
    return Wire.endTransmission(true) == 0;
}

bool Mpu6050::readRaw(int16_t *ax, int16_t *ay, int16_t *az,
                      int16_t *gx, int16_t *gy, int16_t *gz) {
    Wire.beginTransmission(kMpuAddr);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) return false;
    const uint8_t n = Wire.requestFrom((int)kMpuAddr, 14, (int)true);
    if (n != 14) return false;
    *ax = (int16_t)((Wire.read() << 8) | Wire.read());
    *ay = (int16_t)((Wire.read() << 8) | Wire.read());
    *az = (int16_t)((Wire.read() << 8) | Wire.read());
    Wire.read(); Wire.read(); // skip temp
    *gx = (int16_t)((Wire.read() << 8) | Wire.read());
    *gy = (int16_t)((Wire.read() << 8) | Wire.read());
    *gz = (int16_t)((Wire.read() << 8) | Wire.read());
    return true;
}

// Mpu6500 类方法
bool Mpu6500::begin() {
    delay(10);
    if (readReg(0x75) != 0x70) return false;
    writeReg(0x6B, 0x00);
    delay(10);
    writeReg(0x1B, 0x18);
    writeReg(0x1C, 0x10);
    writeReg(0x1A, 0x03);
    delay(10);
    gyro_bias_x_ = gyro_bias_y_ = gyro_bias_z_ = 0.0f;
    return true;
}

void Mpu6500::calibrateGyro(uint16_t samples, uint16_t delay_ms) {
    float sumx = 0, sumy = 0, sumz = 0;
    for (uint16_t i = 0; i < samples; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        if (readRaw(&ax, &ay, &az, &gx, &gy, &gz)) {
            const float scale = 2000.0f / 32768.0f;
            sumx += gx * scale; sumy += gy * scale; sumz += gz * scale;
        }
        delay(delay_ms);
    }
    gyro_bias_x_ = sumx / samples;
    gyro_bias_y_ = sumy / samples;
    gyro_bias_z_ = sumz / samples;
}

bool Mpu6500::read(ImuSample *out) {
    int16_t ax, ay, az, gx, gy, gz;
    if (!readRaw(&ax, &ay, &az, &gx, &gy, &gz)) return false;
    const float a_scale = 8.0f / 32768.0f;
    const float g_scale = 2000.0f / 32768.0f;
    float ax_g = ax * a_scale, ay_g = ay * a_scale, az_g = az * a_scale;
    float gx_dps = gx * g_scale - gyro_bias_x_;
    float gy_dps = gy * g_scale - gyro_bias_y_;
    float gz_dps = gz * g_scale - gyro_bias_z_;
    if (kImuSwapXY) {
        float t_a = ax_g; ax_g = ay_g; ay_g = t_a;
        float t_g = gx_dps; gx_dps = gy_dps; gy_dps = t_g;
        gx_dps = -gx_dps; gy_dps = -gy_dps;
    }
    if (kImuFlipX) { ax_g = -ax_g; gx_dps = -gx_dps; }
    if (kImuFlipY) { ay_g = -ay_g; gy_dps = -gy_dps; }
    if (kImuFlipZ) { az_g = -az_g; gz_dps = -gz_dps; }
    out->ax_g = ax_g; out->ay_g = ay_g; out->az_g = az_g;
    out->gx_dps = gx_dps; out->gy_dps = gy_dps; out->gz_dps = gz_dps;
    return true;
}

void Mpu6500::initSpiPins() {
    SPI.begin(kSpiClk, kSpiMiso, kSpiMosi, kSpiCs);
    pinMode(kSpiCs, OUTPUT);
    digitalWrite(kSpiCs, HIGH);
}

uint8_t Mpu6500::readReg(uint8_t reg) {
    digitalWrite(kSpiCs, LOW);
    SPI.transfer(reg | 0x80);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(kSpiCs, HIGH);
    return val;
}

void Mpu6500::writeReg(uint8_t reg, uint8_t val) {
    digitalWrite(kSpiCs, LOW);
    SPI.transfer(reg & 0x7F);
    SPI.transfer(val);
    digitalWrite(kSpiCs, HIGH);
}

bool Mpu6500::readRaw(int16_t *ax, int16_t *ay, int16_t *az,
                      int16_t *gx, int16_t *gy, int16_t *gz) {
    digitalWrite(kSpiCs, LOW);
    SPI.transfer(0x3B | 0x80);
    uint8_t data[14];
    for (int i = 0; i < 14; i++) data[i] = SPI.transfer(0x00);
    digitalWrite(kSpiCs, HIGH);
    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
    *gx = (int16_t)((data[8] << 8) | data[9]);
    *gy = (int16_t)((data[10] << 8) | data[11]);
    *gz = (int16_t)((data[12] << 8) | data[13]);
    return true;
}

// ImuDriver 类方法
bool ImuDriver::begin() {
    Wire.begin(kI2cSda, kI2cScl);
    if (testMpu6050()) {
        type_ = IMU_MPU6050;
        if (!mpu6050_.begin()) return false;
        Serial.println("Detected MPU6050 (I2C)");
        return true;
    }
    Wire.end();
    delay(10);
    Mpu6500::initSpiPins();
    if (testMpu6500()) {
        type_ = IMU_MPU6500;
        if (!mpu6500_.begin()) return false;
        Serial.println("Detected MPU6500 (SPI)");
        return true;
    }
    type_ = IMU_NONE;
    return false;
}

bool ImuDriver::read(ImuSample *out) {
    if (type_ == IMU_MPU6050) return mpu6050_.read(out);
    if (type_ == IMU_MPU6500) return mpu6500_.read(out);
    return false;
}

void ImuDriver::calibrateGyro(uint16_t samples, uint16_t delay_ms) {
    if (type_ == IMU_MPU6050) mpu6050_.calibrateGyro(samples, delay_ms);
    else if (type_ == IMU_MPU6500) mpu6500_.calibrateGyro(samples, delay_ms);
}

bool ImuDriver::testMpu6050() {
    Wire.beginTransmission(kMpuAddr);
    Wire.write(0x75);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(kMpuAddr, 1);
    if (Wire.available()) {
        uint8_t whoami = Wire.read();
        return (whoami == 0x68 || whoami == 0x70);
    }
    return false;
}

bool ImuDriver::testMpu6500() {
    uint8_t whoami = 0;
    digitalWrite(kSpiCs, LOW);
    SPI.transfer(0x75 | 0x80);
    whoami = SPI.transfer(0x00);
    digitalWrite(kSpiCs, HIGH);
    return (whoami == 0x70);
}