// config.h - 统一硬件引脚与飞控常量配置,20260612
// 本文件合并了原 drone_pins.h 的所有条件编译引脚定义，
// 以及主文件中所有的 static const 参数（PWM、控制、滤波器等）。
// 后续任何模块只需包含本文件，即可获得全部常量。
// * @brief 淘宝店: 耀坤智联（提供本项目所需电子元器件套件，基础元件仅需9.9元） ； 
// * @brief 哔哩哔哩B站：耀坤智联 ； 
// * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// 第一部分：多 PCB 板型选择（引脚定义）
// 默认使用 PCB_YaoKun_LCKFB_ESP32S3配置，即立创ESP32S3-R8N8开发板 
// 全功能完整版,预留6针VL53L1x激光模块插座、9针PMW3901光流模块插座
// 立创·ESP32S3R8N8开发板 https://lckfb.com/project/detail/lckfb-esp32s3r8n8
// ============================================================================
// 自动默认板型（需包含所有已知板型宏名）
// ============================================================================
#if !defined(PCB_YaoKun_LCKFB_ESP32S3) && \
    !defined(PCB_YaoKun_S3mini) && \
    !defined(PCB_YaoKun_S3mini_A) && \
    !defined(PCB_YaoKun_S3mini_B)
    #define PCB_YaoKun_LCKFB_ESP32S3   // 默认板型，切换板型只需修改这一行！
#endif
// ============================================================================
// 编译前可修改#define PCB_YaoKun_LCKFB_ESP32S3，用于切换板型，可选值：
//   PCB_YaoKun_LCKFB_ESP32S3 - 主力全功能版本！对应Yaokun_K1.0版PCB，使用立创ESP32-S3 R8N8开发板，预留插座，可直插VL53L1x激光、PMW3901光流
//   PCB_YaoKun_S3mini_A   - 仅用于内部测试第1版,YaoKun_S1.0A使用ESP32-S3 Supermini开发板,马达引脚为4、1、9、8，预留VL53L1x激光,因引脚紧张，无光流插座
//   PCB_YaoKun_S3mini_B   - 仅用于内部测试第2版,YaoKun_S1.0B使用ESP32-S3 Supermini开发板,马达引脚为4、3、11、10，预留VL53L1x激光,因引脚紧张，无光流插座
//   PCB_YaoKun_S3mini   - 已开源，对应YaoKun_S1.0C版PCB，使用ESP32-S3 Supermini开发板,马达引脚为4、3、9、8，预留VL53L1x激光,因引脚紧张，无光流插座
//   PCB_YaoKun_ESP32Dev  -即将发布YaoKun_D1.0版PCB，全功能版！适配30P的ESP32开发板(LCEDA器件名为ESP32 DEVKIT V1），预留VL53L1x激光、PMW3901光流插座
//   PCB_YaoKun_C3mini   - 即将发布YaoKun_C3mini_1.0版PCB，适配16P的ESP32C3 Supermini开发板,马达引脚10、2、3、1，传感器I2C引脚9、8,无独立的激光、光流插座
//   PCB_YaoKun_ESP32S3-CAM  - 计划带摄像头的版本，YaoKun_ESP32S3_CAM_1.0版PCB，适配ESP32S3-CAM开发板，40针引脚
//   PCB_YaoKun_ESP32C3  - 计划YaoKun_ESP32C3_1.0版PCB，适配ESP32C3经典款/简约款开发板，32针引脚
// 后续可继续添加更多板型

// ---------- PCB_YaoKun_LCKFB_ESP32S3 硬件引脚配置（适配立创ESP32-S3 R8N8开发板） ----------
#if defined(PCB_YaoKun_LCKFB_ESP32S3)
    
    // WiFi AP 热点信息
    #define kApSsid "YaoKun_Drone"
    #define kApPass "12345678"
    
    // 第1组 I2C (MPU6050/MPU6500 惯性测量单元)
    #define kI2cSda 9   // I2C 数据线
    #define kI2cScl 8   // I2C 时钟线
    #define kMpuAddr 0x68   // MPU6050/MPU6500 设备地址

    // IMU 轴映射（根据传感器在机身上的安装方向调整）
    #define kImuSwapXY true   // 交换 X 和 Y 轴
    #define kImuFlipX  true   // 横滚方向取反
    #define kImuFlipY  true   // 俯仰方向取反
    #define kImuFlipZ  false  // Z 轴方向正常

    // 电机引脚 (X 型配置：机头方向位于 M4 和 M1 之间)
    #define kMotor0Pin 16    // 左前 (M4 / FL)
    #define kMotor1Pin 6     // 右前 (M1 / FR)
    #define kMotor2Pin 5     // 右后 (M2 / RR)
    #define kMotor3Pin 15    // 左后 (M3 / RL)

    // LED 指示灯（高电平有效），板载还有一个GPIO=48的LED灯可以利用
    #define led1 41  // 红色 LED（右侧）
    #define led2 7   // 绿色 LED（左侧）
    #define led3 14  // 蓝色 LED（状态）

    // 电池电压检测（ADC 输入）
    #define kVbattAdcPin 2               // ADC1_CH2，GPIO2
    #define kVbattDividerRatio 2.0f     // 分压电路100K+100K，电阻分压比（Vbat = ADC * ratio）

    // 蜂鸣器引脚
    #define BUZZER_PIN 39   // K1.0 蜂鸣器在 GPIO39，请根据实际硬件确认

    // ========== 以下为预留外设引脚（暂未使用，仅定义以备后续扩展）==========
    // SPI 总线（预留用于光流传感器或 SPI 接口 IMU）
    #define kSpiMosi 18   // MOSI
    #define kSpiClk  38   // 时钟
    #define kSpiMiso 21   // MISO
    #define kSpiCs   17   // 片选
    // 第二组 I2C（预留用于 VL53L1X 激光测距等）
    #define kI2c2Scl 4    // 时钟
    #define kI2c2Sda 3    // 数据
    // ELRS 接收机串口（预留）
    #define kElrsTx 12 
    #define kElrsRx 11
    // SBUS 接收机输入引脚（使用 UART 硬件反相）
    #define kSbusPin 1    // GPIO1

// ---------- PCB_YaoKun_S3mini 配置 ----------
// 适配 ESP32-S3 Supermini (S1.0版PCB，马达引脚为4、3、9、8)，引脚紧张，无光流插座
#elif defined(PCB_YaoKun_S3mini)
    
    // WiFi AP 热点信息
    #define kApSsid "YaoKun_Drone"
    #define kApPass "12345678"

    // 第1组 I2C (MPU6050)
    #define kI2cSda 6    // I2C 数据线
    #define kI2cScl 7    // I2C 时钟线
    #define kMpuAddr 0x68   // MPU6050 设备地址

    // IMU 轴映射（根据传感器在机身上的安装方向调整）
    #define kImuSwapXY true   // 交换 X 和 Y 轴
    #define kImuFlipX  true   // 横滚方向取反
    #define kImuFlipY  true   // 俯仰方向取反
    #define kImuFlipZ  false  // Z 轴方向正常

    // 电机引脚 (X 型配置：机头方向位于 M4 和 M1 之间)
    #define kMotor0Pin 8   // 左前 (M4 / FL)
    #define kMotor1Pin 4   // 右前 (M1 / FR)
    #define kMotor2Pin 3   // 右后 (M2 / RR)
    #define kMotor3Pin 9   // 左后 (M3 / RL)

    // LED 指示灯（高电平有效），注意 S3mini 板通常只焊接了蓝色 LED，红绿可能未焊接
    #define led1 33   // 红色 LED ,引脚紧张,实际PCB没有红色LED
    #define led2 34   // 绿色 LED ,引脚紧张,实际PCB没有绿色LED
    #define led3 1    // 蓝色 LED ,实板载有一颗很小的RGB灯，可用GPIO48控制

    // 电池电压检测（ADC 输入）
    #define kVbattAdcPin 2                 // ADC1_CH2，GPIO2
    #define kVbattDividerRatio 2.0f       // 分压电路100K+100K，电阻分压比（Vbat = ADC * ratio）

    // 蜂鸣器
    #define BUZZER_PIN 13

    // 预留外设引脚（可选，与原有预留定义一致）
    // SPI 总线（预留用于光流传感器或 SPI 接口 IMU）
    #define kSpiMosi 6
    #define kSpiClk  5
    #define kSpiMiso 7
    #define kSpiCs   12
    // 第二组 I2C（预留用于 VL53L1X 激光测距等）
    #define kI2c2Scl 11
    #define kI2c2Sda 10
    // ELRS 接收机串口（预留）
    #define kElrsTx 14
    #define kElrsRx 15
    // SBUS 接收机输入引脚（使用 UART 硬件反相）
    #define kSbusPin 16

// ---------- PCB_YaoKun_S3mini_B 配置 ----------
// 适配 ESP32-S3 Supermini (S1.0B版PCB，马达引脚为4、3、11、10)，引脚紧张，无光流插座
#elif defined(PCB_YaoKun_S3mini_B)
    
    // WiFi AP 热点信息
    #define kApSsid "YaoKun_Drone"
    #define kApPass "12345678"

    // 第1组 I2C (MPU6050)
    #define kI2cSda 8    // I2C 数据线
    #define kI2cScl 9    // I2C 时钟线
    #define kMpuAddr 0x68   // MPU6050 设备地址

    // IMU 轴映射
    #define kImuSwapXY true
    #define kImuFlipX  true
    #define kImuFlipY  true
    #define kImuFlipZ  false

    // 电机引脚 (X 型配置)
    #define kMotor0Pin 10   // 左前 (M4 / FL)
    #define kMotor1Pin 4    // 右前 (M1 / FR)
    #define kMotor2Pin 3    // 右后 (M2 / RR)
    #define kMotor3Pin 11   // 左后 (M3 / RL)

    // LED 指示灯
    #define led1 33   // 红色 LED,引脚紧张,实际PCB没有红色LED
    #define led2 34   // 绿色 LED,引脚紧张,实际PCB没有绿色LED
    #define led3 2    // 蓝色 LED,实板载有一颗很小的RGB灯，可用GPIO48控制


    // 电池电压检测（ADC 输入）
    #define kVbattAdcPin 1               // ADC1_CH2，GPIO1
    #define kVbattDividerRatio 2.0f      // 分压电路100K+100K，电阻分压比（Vbat = ADC * ratio）

    // 蜂鸣器
    #define BUZZER_PIN 13

    // 预留外设引脚
    #define kSpiMosi 8
    #define kSpiClk  7
    #define kSpiMiso 9
    #define kSpiCs   12
    #define kI2c2Scl 6
    #define kI2c2Sda 5
    #define kElrsTx 14
    #define kElrsRx 15
    #define kSbusPin 16

// ---------- PCB_YaoKun_S3mini_A 配置 ----------
// 适配 ESP32-S3 Supermini (S1.0A版PCB，马达引脚为4、1、9、8)，引脚紧张，无光流插座
#elif defined(PCB_YaoKun_S3mini_A)
    
    // WiFi AP 热点信息
    #define kApSsid "YaoKun_Drone"
    #define kApPass "12345678"

    // 第1组 I2C (MPU6050)
    #define kI2cSda 6
    #define kI2cScl 7
    #define kMpuAddr 0x68

    // IMU 轴映射
    #define kImuSwapXY true
    #define kImuFlipX  true
    #define kImuFlipY  true
    #define kImuFlipZ  false

    // 电机引脚
    #define kMotor0Pin 8   // 左前 (M4 / FL)
    #define kMotor1Pin 4   // 右前 (M1 / FR)
    #define kMotor2Pin 1   // 右后 (M2 / RR)
    #define kMotor3Pin 9   // 左后 (M3 / RL)

    // LED 指示灯
    #define led1 33
    #define led2 34
    #define led3 3

    // 电池电压检测 
    #define kVbattAdcPin 2   // ADC1_CH2，GPIO2
    #define kVbattDividerRatio 2.0f    // 分压电路100K+100K，电阻分压比（Vbat = ADC * ratio）

    // 蜂鸣器
    #define BUZZER_PIN 13

    // 预留外设引脚
    #define kSpiMosi 6
    #define kSpiClk  5
    #define kSpiMiso 7
    #define kSpiCs   12
    #define kI2c2Scl 11
    #define kI2c2Sda 10
    #define kElrsTx 14
    #define kElrsRx 15
    #define kSbusPin 16

#else
    #error "Unknown PCB_TYPE! 请在 config.h 或编译选项中定义正确的板型宏"
#endif

// ============================================================================
// 第二部分：飞控系统常量（原主文件中的 static const 参数）
// ============================================================================

// ------------------------ 电机测试安全限制 ------------------------
static const float kMotorTestMaxThrottle = 0.20f;   // 最大测试油门 (20%)
static const uint32_t kMotorTestMinMs = 100;        // 最短测试时间 (毫秒)
static const uint32_t kMotorTestMaxMs = 2000;       // 最长测试时间 (毫秒)

// ------------------------ PWM 参数（20kHz 静音）--------------------
static const uint32_t kPwmFreqHz = 20000;           // 电机 PWM 频率 20kHz（超出人耳）
static const uint8_t kPwmResolutionBits = 10;       // 10-bit 分辨率 (0~1023)
static const uint32_t kPwmMaxDuty = (1U << kPwmResolutionBits) - 1U;  // 1023

// LED 指示灯 PWM 参数（独立通道，不影响电机）
static const uint32_t kLedPwmFreqHz = 1000;
static const uint8_t kLedPwmResolutionBits = 8;
static const uint32_t kLedPwmMaxDuty = (1U << kLedPwmResolutionBits) - 1U;  // 255

// 以下通道常量保留以兼容旧版 LEDC API（新版 ledcWrite 直接用引脚号，但某些地方仍可能用到）
static const int kMotor0Ch = 0;
static const int kMotor1Ch = 1;
static const int kMotor2Ch = 2;
static const int kMotor3Ch = 3;
static const int kLed1Ch = 4;
static const int kLed2Ch = 5;

// ------------------------ 控制循环周期 ------------------------
static const uint32_t kLoopPeriodUs = 2000;         // 500Hz 控制周期 (2000us)
static const float kDtMaxSec = 0.02f;               // 最大允许 dt（秒），超过则触发保护

// ------------------------ RC 遥控与解锁参数 ------------------------
static const uint32_t kDefaultCmdTimeoutMs = 300;   // 指令超时时间（毫秒）
static const uint32_t kMinCmdTimeoutMs = 100;
static const uint32_t kMaxCmdTimeoutMs = 2000;
static const float kArmThrottleMax = 0.05f;         // 解锁时允许的最大油门值
static const float kStickCenterMax = 0.15f;         // 摇杆回中允许的偏差
static const uint32_t kArmHoldMs = 800;             // 解锁开关需保持的时间（毫秒）

// ------------------------ 扭矩缩放（低油门时保持操控性）--------------------
static const float kDefaultTorqueScaleSlope = 2.0f;
static const float kDefaultTorqueScaleMin = 0.20f;
static const float kMinTorqueScaleSlope = 0.0f;
static const float kMaxTorqueScaleSlope = 4.0f;
static const float kMinTorqueScaleMin = 0.0f;
static const float kMaxTorqueScaleMin = 1.0f;

// ------------------------ 飞行姿态限制 ------------------------
static const float kDefaultMaxAngleDeg = 30.0f;      // 最大倾斜角度（度）
static const float kMinMaxAngleDeg = 5.0f;
static const float kMaxMaxAngleDeg = 80.0f;
static const float kDefaultMaxYawRateDps = 180.0f;   // 最大偏航角速度（度/秒）
static const float kMinMaxYawRateDps = 30.0f;
static const float kMaxMaxYawRateDps = 360.0f;
static const float kDefaultTiltDisarmDeg = 80.0f;    // 倾斜超过此值自动上锁
static const float kMinTiltDisarmDeg = 20.0f;
static const float kMaxTiltDisarmDeg = 85.0f;

// ------------------------ 遥测数据发送周期 ------------------------
static const uint32_t kDefaultTelemPeriodMs = 50;    // 50ms = 20Hz
static const uint32_t kMinTelemPeriodMs = 20;
static const uint32_t kMaxTelemPeriodMs = 500;

// ------------------------ 姿态滤波器参数 ------------------------
// Madgwick 参数
static const float kMadgwickBeta = 0.08f;            // 默认 Beta 值
static const float kMadgwickBetaMin = 0.02f;
static const float kMadgwickBetaMax = 0.14f;

// Mahony 参数
static const float kMahonyKp = 2.2f;                 // 比例增益
static const float kMahonyKi = 0.0f;                 // 积分增益（暂不使用）
static const float kMahonyIntegralLimitRads = 0.35f;
static const float kMahonySpinLimitDps = 180.0f;

// 加速度计信任度范围（用于自适应滤波器）
static const float kAccTrustHi = 0.08f;              // 信任度上限（加速度偏差小）
static const float kAccTrustLo = 0.35f;              // 信任度下限（加速度偏差大）

// 滤波器模式枚举
enum FilterMode : uint8_t {
    FILTER_MADGWICK = 0,
    FILTER_MAHONY = 1,
    FILTER_COMP = 2,
};
static const FilterMode kFilterMode = FILTER_MAHONY; // 默认使用 Mahony 滤波器

// 互补滤波参数（当选择 FILTER_COMP 时使用）
static const float kCompAlpha = 0.98f;               // 融合系数（陀螺仪占0.98，加速度计占0.02）

// 静态检测参数（用于自动水平校准）
static const float kRelevelAccTolG = 0.10f;          // 加速度模长偏离 1g 的容差（G）
static const float kRelevelGyroDps = 1.5f;           // 角速度静止阈值（度/秒）
static const uint32_t kRelevelHoldMs = 400;          // 静态保持时间（毫秒）

// 调试输出（高频姿态数据，用于上位机可视化，正常飞行时请保持 false）
static const bool kEnableSerialAttDebug = false;   

#endif // CONFIG_H
