// config.h - 统一硬件引脚与飞控常量配置,20260612
// 本文件合并了原 drone_pins.h 的所有条件编译引脚定义，
// 以及主文件中所有的 static const 参数（PWM、控制、滤波器等）。
// 后续任何模块只需包含本文件，即可获得全部常量。
// * @brief 淘宝店: 耀坤智联 https://shop35432590.taobao.com/
// * @brief 提供本项目所需电子元器件套件，基础元件仅需9.9元 ； 
// * @brief 哔哩哔哩B站：耀坤智联 ； 
// * @brief 嘉立创开源硬件平台：耀坤无人机（星火计划2026开源项目）
// *        开源工程页：https://oshwhub.com/lindahe/project_sulkgarf

#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
// ============================================================================
// 第零部分：固件身份信息（开机 IDENTITY 横幅 + 指纹校验用）
// 为什么放在 config.h：版本号/品牌信息全工程只允许这一处数据源，
// 以后发新版本只改这里，开机横幅、发布文档自动跟着变。
// __DATE__ / __TIME__ 是编译器内置宏，在预处理期展开为本次编译的日期时间，
// 因此每台机器固件里的 Built 字段都精确对应它真实的那一次编译。
// ============================================================================
#define FK_ID_NAME     "YaoKunS   耀坤S3自研飞控 / YaoKunDrone"  // 产品名（IDENTITY 首行）
#define FK_ID_VERSION  "v1.3"                                     // 固件版本（发版时同步改！）
                                                                   // v1.2（2026-11-07）：SPI IMU 片选更正为 GPIO42（PCB v2 实测）、
                                                                   //   IMU 方向按「总线+芯片指纹」自动适配（支持进口/国产 MPU6050、MPU6500）
                                                                   // v1.3（2026-11-17）：新增微辣火龙果 2.0 板型（PCB_HLG_V2，经典 ESP32）；
                                                                   //   单灯合成层；火龙果版专属品牌横幅。S3 版行为不变（仅版本号与板型行）。
#define FK_ID_SOURCE   "Source  : 20260613"                       // 源码基线日期（对应 .ino 头部 @version）
#define FK_ID_AUTHOR   "Author  : 耀坤 <espnow@163.com>"
#define FK_ID_BILI     "BiliBili : 耀坤智联  https://space.bilibili.com/3707011699050767/"
#define FK_ID_OSH      "OSHWHub  : 耀坤无人机  https://oshwhub.com/lindahe/"
#define FK_ID_TAOBAO   "Taobao   : 耀坤智联  https://shop35432590.taobao.com/"
#define FK_ID_GITHUB   "GitHub   : https://github.com/YaoKunS/"

// ----------------------------------------------------------------------------
// 微辣火龙果 2.0 版（PCB_HLG_V2）的【品牌身份覆盖】
// ----------------------------------------------------------------------------
// 【归属关系（🔴 铁律，改文案前必读）】
//   耀坤智联是「替微辣火龙果作者（MildlySpicyDragon / malagis）做固件适配与维护」，
//   不是火龙果本人，两板品牌相互独立：
//     · 火龙果版横幅：宣传火龙果的品牌与开源页，末尾保留耀坤的淘宝/固件仓库署名；
//     · S3 版横幅：【严禁】出现火龙果的立创开源页 / B站链接（本块只在 HLG 板型生效，
//       就是为了在编译期把这条规则钉死，不靠人记）。
//   与 flix v1.2 发布件（Forks/flix/Firmware/flix/i18n.h 的 BOARD_HLG_V2 段）同一套措辞规则；
//   火龙果的英文名固定写 MildlySpicyDragon —— 那是作者的用户名，不是真实水果，禁止意译。
// 【为什么用 #undef 覆盖而不是在各板型分支里各写一份】
//   身份文案与引脚是两件正交的事：引脚按板型分，文案默认用 S3 那套，
//   只有火龙果需要整体换品牌。放在这里一块，改文案只需改一处，也不会漏改 S3。
#if defined(PCB_HLG_V2)
    #undef  FK_ID_NAME
    #undef  FK_ID_AUTHOR
    #undef  FK_ID_BILI
    #undef  FK_ID_OSH
    #undef  FK_ID_GITHUB

    #define FK_ID_NAME   "微辣火龙果 2.0 飞控（耀坤自研飞控适配版） MildlySpicyDragon"
    #define FK_ID_AUTHOR "Author  : 硬件设计 微辣火龙果（malagis）· 固件适配 耀坤智联"
    #define FK_ID_BILI   "BiliBili : 微辣火龙果  https://space.bilibili.com/544479100"
    #define FK_ID_OSH    "OSHWHub  : 微辣火龙果  https://oshwhub.com/malagis/esp32-mini-plane"
    // 固件仓库行指向自研飞控仓库（flix 是另一个产品，不能混）
    #define FK_ID_GITHUB "GitHub   : https://github.com/YaoKunS/ESP32-Drone"
#endif

// ============================================================================
// 第一部分：多 PCB 板型选择（引脚定义）
// 默认使用 PCB_YaoKun_LCKFB_ESP32S3配置，即立创ESP32S3-R8N8开发板 
// 全功能完整版,预留6针VL53L1x激光模块插座、9针PMW3901光流模块插座
// 立创·ESP32S3R8N8开发板 https://lckfb.com/project/detail/lckfb-esp32s3r8n8
// ============================================================================
// ============================================================================
// 板型选择器（🔴 整份工程切换板型，只有这一处需要改）
// ============================================================================
// 切换板型两种方式，任选一种：
//   ① 改这里：取消下面某一行的注释（Arduino IDE 里最直观）
//   ② 编译选项：-DPCB_HLG_V2（arduino-cli 批量构建用；编译选项与这里的
//      默认值等价，不需要两边都写）
// 两个都不指定 ⇒ 默认 PCB_YaoKun_LCKFB_ESP32S3（立创 S3 版，行为保持现状）
//
// 为什么要做"二选一 + 冲突报错"：
//   电机引脚没有身份标识，程序无法自己认出插的是哪块板；一旦"以为编的 A、
//   实际编成 B"，上电就会四轴乱序起转。所以板型必须由人明确指定，
//   且同时定义两种板型时直接 #error 中止编译，绝不允许程序去猜。
// ----------------------------------------------------------------------------
// #define PCB_HLG_V2          // ← 火龙果 2.0 版（经典 ESP32 DevKit V1，单灯）

// 自动默认板型（需包含所有已知板型宏名）
// ============================================================================
#if !defined(PCB_YaoKun_LCKFB_ESP32S3) && \
    !defined(PCB_YaoKun_S3mini) && \
    !defined(PCB_YaoKun_S3mini_B) && \
    !defined(PCB_YaoKun_S3mini_A) && \
    !defined(PCB_HLG_V2)
    #define PCB_YaoKun_LCKFB_ESP32S3   // 默认板型
#endif

// 板型互斥检查：两个以上板型宏同时生效 = 编译期直接报错（见上面"为什么"）
#if defined(PCB_HLG_V2) && (defined(PCB_YaoKun_LCKFB_ESP32S3) || \
                            defined(PCB_YaoKun_S3mini) || \
                            defined(PCB_YaoKun_S3mini_B) || \
                            defined(PCB_YaoKun_S3mini_A))
    #error "板型宏冲突：PCB_HLG_V2 不能与其他 PCB_YaoKun_* 板型同时定义"
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
//   PCB_HLG_V2          - 微辣火龙果 2.0 版PCB（ESP32 DevKit V1 经典片，30 针，板载仅 1 颗灯）
//                         由耀坤智联代为适配维护；原作者 malagis（oshwhub.com/malagis/esp32-mini-plane）
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
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    // 2026-11-07 板 001 实测（PCB v2 + MPU6500 SPI）：6050 时代的 false/false 在 6500 上
    // 方向全反（右侧压低 roll 为负）。两代模块外形/摆放相同，但芯片坐标系相对丝印
    // 相差 180° ⇒ swapXY + flipX + flipY 三者一起构成"绕 Z 轴 180°"刚体旋转（保持手性，
    // AHRS 自洽；严禁只翻单轴，会变镜像导致陀螺与加速度不一致）。与另一板段的
    // "MPU6500模块为true"实测结论一致。若改回 MPU6050，需把下面两个改回 false。
    #define kImuFlipX  true   // 横滚方向取反（MPU6500 模块为 true，2026-11-07 实测确认）
    #define kImuFlipY  true   // 俯仰方向取反（MPU6500 模块为 true，2026-11-07 实测确认）
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

// 关于IMU轴映射，目前已测试3种（原厂MPU6050模块、国产MPU6050、MPU6500模块），请看下面描述：
// 如果你使用其他IMU或者模块的贴装方向与“耀坤无人机”开源PCB不同，
// 请在 config.h 中依次尝试 “IMU 轴映射” 各参数true/false组合，多尝试几次！
// 每次修改后重新编译、上传，并观察 Web 姿态球地平线的地平线情况，以及横滚（Roll）和俯仰（Pitch）读数。
// 你可以把姿态球的情况，描述清楚，问AI分析，再重新修改!一般测试2-3次后，AI就可以分析给出正确的组合！
// ============================================================
// 验证方法：修改后重新编译上传，在Web浏览器里输入http://192.168.4.1
// 测试 Roll：将飞机向左倾斜约 30°，观察网页 Roll 读数应该是负值，同时姿态球应向左转（地平线左低右高）。
// 测试 Pitch：将飞机抬头（机头向上），观察网页 Pitch 读数是否为 正值（通常抬头为正），姿态球上方应显示天空。
// 即在“设置”页面观察姿态球的反应，正确的情况应该是：
// 飞机抬头（机头上仰）→ 姿态球应显示天空（俯仰角正）。
// 飞机低头（机头下俯）→ 姿态球应显示地面（俯仰角负）。
// 飞机左倾 → 横滚读数为负，姿态球地平线左低右高；飞机右倾 → 横滚读数为正，姿态球地平线左高右低；
// ============================================================
/**
    // IMU 轴映射（原厂MPU6050模块，芯片较大，手工测量约4*4mm）
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  false   // 横滚方向取反，原厂MPU6050设置为false
    #define kImuFlipY  false   // 俯仰方向取反，原厂MPU6050设置为false
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

    // IMU 轴映射（MPU6500模块）
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  true   // 横滚方向取反，MPU6500模块为true
    #define kImuFlipY  true   // 俯仰方向取反，MPU6500模块为true
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

    // IMU 轴映射（国产MPU6050，芯片较小，手工测量约3*3mm）
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  false   // 横滚方向取反，国产MPU6050也是false
    #define kImuFlipY  false   // 俯仰方向取反，国产MPU6050也是false
    #define kImuFlipZ  false  // Z 轴方向正常，默认false
 */
 // ============================================================

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
    // 片选（MPU6500 走 SPI 时的 NCS 脚）：
    //   PCB v2 实测 = GPIO42（2026-11-02 板 001 用 mpu6500_probe 扫出来的：
    //     GPIO17 读 WHO_AM_I 得 0x00（线上无器件），GPIO42 读到 0x70 = MPU6500，
    //     且 GYRO_CONFIG/ACCEL_CONFIG 写入回读一致、静止 |a|=1.05g）。
    //   与《耀坤S3无人机引脚定义》表完全对应：GPIO42 = MPU6500_CS（本宏），
    //   GPIO17 = SPI_CS 是**光流模块的片选**——同一条 SPI 总线（SCK38/MOSI18/MISO21）
    //   挂两个从设备：IMU 用 42、光流用 17。光流驱动尚未实现，17 暂时空闲；
    //   将来做光流时必须单独控制 17 保持高电平（拉低会误选中，但光流不插时无副作用）。
    //   🔴 注意别把 17 当成 IMU 片选写回这里——那是探测阶段的误判来源之一。
    #define kSpiCs   42   // MPU6500 片选（PCB v2 实测；GPIO17 是光流 CS，不是 IMU 的）
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
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  false  // 横滚方向取反，原厂国产MPU6050都是false，MPU6500模块为true
    #define kImuFlipY  false  // 俯仰方向取反，原厂国产MPU6050都是false，MPU6500模块为true
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

    // 电机引脚 (X 型配置：机头方向位于 M4 和 M1 之间)
    #define kMotor0Pin 8   // 左前 (M4 / FL)
    #define kMotor1Pin 4   // 右前 (M1 / FR)
    #define kMotor2Pin 3   // 右后 (M2 / RR)
    #define kMotor3Pin 9   // 左后 (M3 / RL)

    // LED 指示灯（高电平有效），注意S3mini版通常只焊接了蓝色 LED，红绿可能未焊接
    #define led1 33   // 红色 LED ,可用引脚紧张,实际PCB没有红色LED
    #define led2 34   // 绿色 LED ,可用引脚紧张,实际PCB没有绿色LED
    #define led3 1    // 蓝色 LED ,板载有一颗很小的RGB灯，可用GPIO48控制

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
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  true  // 横滚方向取反，MPU6500模块为true，原厂国产MPU6050都是false
    #define kImuFlipY  true  // 俯仰方向取反，MPU6500模块为true，原厂国产MPU6050都是false
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

    // 电机引脚 (X 型配置)
    #define kMotor0Pin 10   // 左前 (M4 / FL)
    #define kMotor1Pin 4    // 右前 (M1 / FR)
    #define kMotor2Pin 3    // 右后 (M2 / RR)
    #define kMotor3Pin 11   // 左后 (M3 / RL)

    // LED 指示灯
    #define led1 48   // 红色 LED,板载的LED灯、WS2812,可用GPIO48控制
    #define led2 34   // 绿色 LED,可用引脚紧张,实际PCB没有绿色LED
    #define led3 2    // 蓝色 LED,独立控制的蓝色LED


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
    #define kImuSwapXY true   // 交换 X 和 Y 轴，默认true
    #define kImuFlipX  true  // 横滚方向取反，MPU6500模块为true，原厂国产MPU6050都是false
    #define kImuFlipY  true  // 俯仰方向取反，MPU6500模块为true，原厂国产MPU6050都是false
    #define kImuFlipZ  false  // Z 轴方向正常，默认false

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

// ---------- PCB_HLG_V2 配置 ----------
// 微辣火龙果 2.0 版 PCB（ESP32 DevKit V1 经典片，30 针，4MB Flash）
// 引脚基线：flix 仓库 Firmware/flix/board.h 的 BOARD_HLG_V2 分板
//           （2026-11 已在 flix 上实测适配，本段与之逐脚一致）
//
// 🔴 经典 ESP32 只有 GPIO0~39：凡是 S3 版用到的 42 / 41 / 39 之类编号，
//    在经典片上一律不能出现（对 42 号脚 digitalWrite 属于越界写寄存器，
//    未定义行为，可能直接死机）。所以本段所有引脚都必须 <= 39，不用的直接 -1。
// 🔴 本板与耀坤 S3 版的关系（2026-11-16 用户实测确认）：
//    · IMU 贴装方向、芯片朝向完全一致（MPU6050 / MPU6500 都一样）
//    · 马达/桨叶安装方式、PCB 尺寸完全一致
//    ⇒ IMU 轴映射、混控分配、偏航扭矩符号 kYawTorqueSign 全部沿用 S3 的结论，
//      不按板型区分（这正是本段只换引脚、不动算法的原因）。
#elif defined(PCB_HLG_V2)

    // WiFi AP 热点信息（与 S3 版完全一致，一套说明书通用）
    #define kApSsid "YaoKun_Drone"
    #define kApPass "12345678"

    // 第1组 I2C（MPU6050 / 磁罗盘 / 气压计 排母）—— 经典片 I2C0 默认脚
    #define kI2cSda 21   // I2C 数据线（SDA）
    #define kI2cScl 22   // I2C 时钟线（SCL）
    #define kMpuAddr 0x68   // MPU6050/MPU6500 设备地址

    // IMU 轴映射（编译期兜底值）
    // 🔴 正常情况下这几行用不上：上电时 applyAxisMapByWhoAmI() 会按
    //    「总线 + 芯片指纹」自动覆盖（I2C 排母 = 6050 系、SPI 座 = 6500 系）。
    //    这里保持与 S3 段同一组值，含义是"探测失败时两板行为一致"。
    //    不要再按板型写不同的值——两板 IMU 贴装方向实测完全相同。
    #define kImuSwapXY true   // 交换 X 和 Y 轴
    #define kImuFlipX  true   // 横滚方向取反（MPU6500 模块为 true）
    #define kImuFlipY  true   // 俯仰方向取反（MPU6500 模块为 true）
    #define kImuFlipZ  false  // Z 轴方向正常

    // 电机引脚 (X 型配置：机头方向位于 M4 和 M1 之间)
    // 板丝印编号是顺时针 M1~M4，对应关系：
    //   M4 左前 → 32   M1 右前 → 33   M2 右后 → 27   M3 左后 → 26
    // 混控公式只认机体位置（M0=左前…M3=左后），不认 GPIO ⇒ 不用改混控。
    #define kMotor0Pin 32    // 左前 (M4 / FL)
    #define kMotor1Pin 33    // 右前 (M1 / FR)
    #define kMotor2Pin 27    // 右后 (M2 / RR)
    #define kMotor3Pin 26    // 左后 (M3 / RL)

    // LED 指示灯：本板只有 1 颗板载单色灯，接法 = GPIO2 → 限流电阻 → LED → GND
    //（拉电流，高电平点亮；已确认，非灌电流接法）
    // 🔴 led1/led2/led3 一律置 -1：三个逻辑灯在这块板上都不存在。
    //    三路状态由 drone_led.h 的单灯合成层合并到 kLedSinglePin 输出：
    //    红色/绿色（亮度）、蓝色（数字状态）取"最亮者"，呼吸/交替退化为亮灭，
    //    闪频语义保留（待机呼吸、解锁常亮、故障快闪仍可区分）。
    // 🔴 特别注意别把 led3 写成 14 —— 那是光流模块的 CLK 脚，拉低会打死光流时钟。
    #define led1 (-1)          // 无红色独立灯
    #define led2 (-1)          // 无绿色独立灯
    #define led3 (-1)          // 无蓝色独立灯
    #define kLedSingle 1       // 1 = 单灯板型（三色语义合成到一颗灯）
    #define kLedSinglePin 2    // 唯一板载灯引脚

    // 电池电压检测（ADC 输入）
    // 🔴 必须落在 ADC1（32/33/34/35/36/39）：ADC2 与 Wi-Fi 互斥，而本固件
    //    热点常开 ⇒ 若用 ADC2 会读到恒 0 ⇒ 误判低电 ⇒ 蜂鸣器无限急鸣。
    //    GPIO35 = ADC1_CH7，且是 input-only，不会被误配成输出，最合适。
    #define kVbattAdcPin 35             // ADC1_CH7，GPIO35
    #define kVbattDividerRatio 2.0f     // 分压 100K+100K，Vbat = ADC * ratio

    // 蜂鸣器引脚（有源蜂鸣器，高电平响；两板都是有源 ⇒ digitalWrite 即可，不需要方波）
    #define BUZZER_PIN 25

    // SPI 总线：IMU 走 VSPI 默认脚（SCK18/MISO19/MOSI23/CS5）
    // 注意 ESP-Drone 的命名是 MOS/MIS：SDI=MOSI=23，SDO=MISO=19，别搞反。
    #define kSpiMosi 23   // MOSI (SDI)
    #define kSpiClk  18   // SCLK
    #define kSpiMiso 19   // MISO (SDO)
    #define kSpiCs   5    // MPU6500 片选（NCS）；本板只有一个 SPI 从设备，无候选片选

    // ========== 以下为预留外设引脚（暂未使用，仅定义以备后续扩展）==========
    // 第二组 I2C（预留 VL53L1X 激光测距）
    // 🔴 16/17 仅在 WROOM 模组可用；WROVER 模组被 PSRAM 占用。本固件未接，仅登记。
    #define kI2c2Scl 17
    #define kI2c2Sda 16
    // ELRS 接收机串口（预留；本板没有空闲串口，置 -1 = 未分配）
    #define kElrsTx (-1)
    #define kElrsRx (-1)
    // SBUS 接收机输入引脚（自研固件暂未实现 SBUS 驱动，仅登记引脚）
    #define kSbusPin 4

#else
    #error "Unknown PCB_TYPE! 请在 config.h 或编译选项中定义正确的板型宏"
#endif

// ============================================================================
// 板型派生常量（板型分支之后统一生成，避免每个分支重复写）
// ============================================================================
// 单灯板型标记：只有火龙果分支会主动定义 kLedSingle=1，其余板型默认 0。
// DRONE_LED 层用 #if kLedSingle 决定是"三灯各写各的"还是"三路合成一颗灯"。
#ifndef kLedSingle
    #define kLedSingle 0
#endif

// 板型名（开机 IDENTITY 横幅显示）
// 用途：两套固件版本号相同时（例如都在 v1.2 开发中），现场接串口一眼就能
//       区分烧的是 S3 版还是火龙果版——这是发布件指纹校验的一部分。
// 为什么不在各分支里各写一遍：横幅文案与引脚是两件事，集中在这里以后
// 加板型只需改两处（分支 + 这一行），不会漏。
#if defined(PCB_HLG_V2)
    #define FK_ID_BOARD "Board   : HLG 2.0 (ESP32 classic, single LED)"
#elif defined(PCB_YaoKun_LCKFB_ESP32S3)
    #define FK_ID_BOARD "Board   : YaoKun S3 (LCKFB R8N8)"
#elif defined(PCB_YaoKun_S3mini)
    #define FK_ID_BOARD "Board   : YaoKun S1.0C (S3 Supermini)"
#elif defined(PCB_YaoKun_S3mini_B)
    #define FK_ID_BOARD "Board   : YaoKun S1.0B (S3 Supermini)"
#elif defined(PCB_YaoKun_S3mini_A)
    #define FK_ID_BOARD "Board   : YaoKun S1.0A (S3 Supermini)"
#else
    #define FK_ID_BOARD "Board   : (unknown)"
#endif

// ============================================================================
// 第二部分：飞控系统常量（原主文件中的 static const 参数）
// ============================================================================

// ------------------------ 偏航扭矩符号（装机方向约定）------------------------
// 作用：混控前对 yaw_torque 乘以本系数，适配实际装机时四颗电机的旋转方向。
// 为什么需要它：
//   混控公式默认约定（俯视）：M0左前=CCW、M1右前=CW、M2右后=CCW、M3左后=CW
//   （对角线两两同向，"前桨向外"布局）。在此约定下 yaw_torque>0 产生
//   逆时针（CCW）净反扭矩，与 gz（CCW 为正）构成负反馈。
//   2026-10-23 实测 1 号机装机方向（不可改动，电机线序/桨叶均已固定）：
//     左前(GPIO16)=CW、右前(GPIO6)=CCW、左后(GPIO15)=CCW、右后(GPIO5)=CW
//   —— 与默认约定每颗都恰好相反（对角线仍两两同向，布局本身合法）。
//   若不改此符号，yaw 环会变成正反馈：表现为起飞后机头越转越快、
//   配合倾斜后推力矢量绕圈 = "螺旋升天"。
//   乘 -1 后扭矩方向整体翻转，yaw 环恢复负反馈；roll/pitch 扭矩只与电机
//   位置有关（位置没变），不受影响，无需改动。
// 为什么放在混控处翻转而不是改 kImuFlipZ：
//   kImuFlipZ 会把陀螺仪 az（垂直加速度）一起翻转，姿态球会整体翻 180°，
//   绝不能用；而且改 gz 符号会让"输入语义→期望角速度"的约定全部连锁变动。
//   在 PID 输出之后、混控之前翻转，杆向/网页摇杆/SBUS 全部不用动。
// 取值：+1.0f = 默认布局（前桨向外）；-1.0f = 全反布局（本机实测）。
static const float kYawTorqueSign = -1.0f;

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
