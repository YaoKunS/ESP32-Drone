#pragma once

/**
 * @file drone_imu.h
 * @brief IMU 驱动和姿态解算算法（Madgwick、Mahony、互补滤波）

 * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
 * @brief 哔哩哔哩B站：耀坤智联 ； 
 * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
 
 * 本文件包含：
 *   - MadgwickImu 类：梯度下降姿态解算
 *   - MahonyImu 类：基于比例积分修正的姿态解算
 *   - Mpu6050 类：I2C 接口的 MPU6050 驱动
 *   - Mpu6500 类：SPI 接口的 MPU6500 驱动
 *   - ImuDriver 类：自动检测并统一访问 IMU
 *   - 辅助函数：加速度计转欧拉角、静止判断
 * 
 * 在模块化拆分后，本文件仅保留类声明和 static inline 辅助函数。
 * 具体的类方法实现已移至 hardware.ino。
 */

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// ============================================================
// Madgwick IMU-only 姿态解算类（仅用四元数）
// ============================================================
class MadgwickImu {
 public:
  // 初始化，设置 beta 参数（陀螺仪信任度）
  void begin(float beta);
  // 运行时调整 beta 值（自适应）
  void setBeta(float beta);
  // 核心更新函数：输入角速度（rad/s）、加速度（g）、时间间隔 dt
  void update(float gx_rads, float gy_rads, float gz_rads,
              float ax, float ay, float az,
              float dt);
  // 获取欧拉角（度）
  void getEulerDeg(float *roll_deg, float *pitch_deg, float *yaw_deg) const;
  // 从欧拉角（度）设置四元数（用于初始对准）
  void setEulerDeg(float roll_deg, float pitch_deg, float yaw_deg);
  void setEulerRad(float roll, float pitch, float yaw);

 private:
  // 当加速度计数据无效时，仅用陀螺仪积分更新四元数
  void integrateGyroOnly(float gx_rads, float gy_rads, float gz_rads, float dt);

  float beta_ = 0.08f;   // 算法参数，控制加速度计修正强度
  float q0_ = 1.0f;      // 四元数分量
  float q1_ = 0.0f;
  float q2_ = 0.0f;
  float q3_ = 0.0f;
};

// ============================================================
// Mahony (Betaflight-style DCM) IMU-only 姿态解算类
// ============================================================
class MahonyImu {
 public:
  // 初始化，设置比例增益 kp 和积分增益 ki
  void begin(float kp, float ki);
  // 复位状态（清空积分项）
  void reset();
  void setGains(float kp, float ki);
  void setIntegralLimitRads(float limit_rads);
  void setSpinRateLimitDps(float limit_dps);
  // 核心更新函数
  void update(float gx_rads, float gy_rads, float gz_rads,
              float ax, float ay, float az,
              float dt, float acc_trust = 1.0f);
  // 获取欧拉角（度）
  void getEulerDeg(float *roll_deg, float *pitch_deg, float *yaw_deg) const;
  // 设置欧拉角（度）
  void setEulerDeg(float roll_deg, float pitch_deg, float yaw_deg);
  void setEulerRad(float roll, float pitch, float yaw);

 private:
  float kp_ = 2.0f;
  float ki_ = 0.0f;

  float integral_fb_x_ = 0.0f;
  float integral_fb_y_ = 0.0f;
  float integral_fb_z_ = 0.0f;
  float integral_limit_rads_ = 0.35f;  // 积分项限幅（rad/s）
  float spin_rate_limit_rads_ = 0.0f;  // 自旋速率限制，0表示禁用

  float q0_ = 1.0f;
  float q1_ = 0.0f;
  float q2_ = 0.0f;
  float q3_ = 0.0f;
};

// ============================================================
// MPU6050 驱动 (I2C协议) - 类声明
// ============================================================
class Mpu6050 {
 public:
  bool begin();
  void calibrateGyro(uint16_t samples, uint16_t delay_ms = 2);
  bool read(ImuSample *out);

 private:
  bool writeReg(uint8_t reg, uint8_t val);
  bool readRaw(int16_t *ax, int16_t *ay, int16_t *az,
               int16_t *gx, int16_t *gy, int16_t *gz);

  float gyro_bias_x_ = 0.0f;
  float gyro_bias_y_ = 0.0f;
  float gyro_bias_z_ = 0.0f;
};

// ============================================================
// MPU6500 驱动 (SPI协议) - 类声明
// ============================================================
class Mpu6500 {
 public:
  bool begin();
  void calibrateGyro(uint16_t samples, uint16_t delay_ms = 2);
  bool read(ImuSample *out);
  static void initSpiPins();

 private:
  uint8_t readReg(uint8_t reg);
  void writeReg(uint8_t reg, uint8_t val);
  bool readRaw(int16_t *ax, int16_t *ay, int16_t *az,
               int16_t *gx, int16_t *gy, int16_t *gz);

  float gyro_bias_x_ = 0.0f, gyro_bias_y_ = 0.0f, gyro_bias_z_ = 0.0f;
};

// ============================================================
// 统一 IMU 驱动：自动检测 MPU6050 (I2C) 或 MPU6500 (SPI) - 类声明
// ============================================================
enum ImuType : uint8_t {
  IMU_NONE = 0,
  IMU_MPU6050 = 1,
  IMU_MPU6500 = 2
};

class ImuDriver {
 public:
  bool begin();
  bool read(ImuSample *out);
  void calibrateGyro(uint16_t samples, uint16_t delay_ms = 2);

 private:
  bool testMpu6050();
  bool testMpu6500();

  ImuType type_ = IMU_NONE;
  Mpu6050 mpu6050_;
  Mpu6500 mpu6500_;
};

// ============================================================
// 辅助函数（static inline，仅用于内部计算，无外部链接冲突）
// ============================================================

/**
 * @brief 通过加速度计计算欧拉角（用于互补滤波的初始化）
 * @param s         IMU 样本
 * @param roll_deg  输出横滚角（度）
 * @param pitch_deg 输出俯仰角（度）
 */
static inline void accelToRollPitchDeg(const ImuSample &s, float *roll_deg, float *pitch_deg) {
  const float ax = s.ax_g;
  const float ay = s.ay_g;
  const float az = s.az_g;

  const float roll = atan2f(ay, az);
  const float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

  *roll_deg = roll * 180.0f / (float)M_PI;
  *pitch_deg = pitch * 180.0f / (float)M_PI;
}

/**
 * @brief 判断 IMU 是否处于静止状态（用于自动水平校准）
 * @param s IMU 样本
 * @return true 表示静止，false 表示有运动
 */
static inline bool isImuStationary(const ImuSample &s) {
  const float acc_norm = sqrtf(s.ax_g * s.ax_g + s.ay_g * s.ay_g + s.az_g * s.az_g);
  if (!isFinitef(acc_norm)) {
    return false;
  }
  // 加速度模长应接近 1g
  if (fabsf(acc_norm - 1.0f) > kRelevelAccTolG) {
    return false;
  }
  // 角速度应小于阈值
  if (fabsf(s.gx_dps) > kRelevelGyroDps ||
      fabsf(s.gy_dps) > kRelevelGyroDps ||
      fabsf(s.gz_dps) > kRelevelGyroDps) {
    return false;
  }
  return true;
}