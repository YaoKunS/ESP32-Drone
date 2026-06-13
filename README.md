# ✈️ 耀坤飞控 · 开源四轴无人机 (YaoKun Drone)

[![Platform](https://img.shields.io/badge/Platform-Arduino-blue)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-S3%20%7C%20S3mini%20%7C%20C3-purple)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

**基于 Arduino 的高度模块化飞控 —— 让每个人都能轻松玩转无人机核心技术**

![YaoKun Drone Demo](docs/demo.gif)   <!-- 可选：放入一张动图或照片 -->

## 📌 简介

耀坤飞控是一款面向 **嵌入式学习、算法验证、科创竞赛** 的开源四轴飞行器固件。它基于 Arduino 框架开发，运行在廉价的 ESP32 系列开发板上，通过 WebSocket 技术实现手机/电脑网页直连遥控，无需安装任何 APP。

项目代码拆分为 **多个功能独立的 .ino 文件**，中文注释详尽，完整覆盖了飞控系统的全部知识链：  
`传感器驱动 → 姿态解算 → PID 控制 → 电机混控 → 遥控通信 → 参数存储 → 故障保护 → 遥测发送`。

与其他开源飞控（Flix、ESP‑FC、ESP‑Drone）相比，耀坤飞控 **在保持 Arduino 低门槛的同时，实现了商业级飞控的核心架构**，既是“可以飞的无人机”，更是嵌入式系统模块化设计的绝佳教学范例。

## 🔥 核心特性

| 特性 | 说明 |
|------|------|
| **三种姿态解算器** | Madgwick、Mahony、互补滤波 可选，支持热切换，方便对比教学 |
| **串级 PID 控制** | 角度环 + 角速率环，独立偏航 PID，带扭矩缩放与积分抗饱和 |
| **WebSocket 网页遥控** | 手机/电脑连接 WiFi 后打开浏览器即可遥控，无需安装 APP |
| **实时遥测与调参** | 姿态仪表、电池电压、电机输出、PID 参数、滤波器模式等均可在网页上实时调节和保存 |
| **模块化代码** | 12 个独立文件，每个文件职责单一（如 `rc.ino` 处理遥控与命令，`failsafe.ino` 负责解锁/上锁） |
| **多板型支持** | 通过 `config.h` 一键切换 8 种 ESP32 板型（立创 ESP32‑S3、S3mini 系列等） |
| **硬件开源** | 提供立创 EDA 原理图 / PCB，所有元件易购，成本低 |
| **中文文档 + 详细注释** | 关键代码均有中文注释，配套系列教程，适合自学和教学 |

## 🧱 模块化代码结构

```text
YaoKun_Drone/
├── YaoKun_Drone.ino      # 主入口：setup/loop，调度控制循环，电压告警，蓝色LED
├── config.h              # 硬件引脚（多板型） + 飞控常量（PWM、滤波器、阈值）
├── drone_types.h         # 公共类型、枚举、工具函数、跨模块 extern 声明
├── drone_pid.h           # PID 类（纯算法）
├── hardware.ino          # LED/蜂鸣器/电机/IMU 底层驱动
├── storage.ino           # NVS 参数存储（PID 参数、配置、校准偏置）
├── control.ino           # PID 对象实例 + 参数应用函数
├── failsafe.ino          # 解锁/上锁状态机、紧急停止、故障保护
├── telemetry.ino         # WebSocket 遥测数据发送
├── rc.ino                # 遥控指令存储 + WebSocket 命令处理（核心通信）
├── drone_web_ui.h        # 嵌入式网页界面（HTML/CSS/JS）
├── drone_led.h / buzzer.h / imu.h   # 硬件抽象头文件
└── (其他)                # 无额外依赖，所有模块均包含 drone_types.h
```

## 🚀 快速开始

1. 硬件准备
一块 ESP32-S3 或 ESP32-S3 Supermini 等开发板

一个 MPU6050 / MPU6500 模块（I2C 或 SPI）

四个 8520 空心杯电机 + 配套桨叶

3.7V 锂电池（1S）

嘉立创PCB免费打样，PCB即机架

详细物料清单和 PCB 文件请访问 立创开源硬件平台链接

2. 软件环境
安装 Arduino IDE（≥2.3.x）

安装 ESP32 开发板支持包（在 Arduino IDE 首选项附加开发板管理网址中添加：

https://espressif.github.io/arduino-esp32/package_esp32_index.json    然后安装 esp32 包）

安装必要库（通过库管理器）：

AsyncTCP by ESP32Asyc

ESPAsyncWebServer  by ESP32Asyc

Preferences（ESP32 内置，无需额外安装）

3. 编译与烧录
bash
git clone [https://github.com/YaoKunS/ESP32_Drone.git](https://github.com/YaoKunS/ESP32-Drone.git)

用 Arduino IDE 打开 YaoKun_Drone.ino，在 config.h 中根据您的板型修改对应的宏（默认 PCB_YaoKun_LCKFB_ESP32S3 适用于立创 ESP32‑S3 开发板）。选择正确的端口和开发板，点击“上传”。

## ✈️ 4. 首次飞行（详细步骤）

⚠️ 安全提示：飞行前务必拆下螺旋桨，先完成地面测试！

步骤 1：校准传感器
给飞控供电，等待蓝色 LED 慢闪，蜂鸣器发出“就绪”音。

用手机或电脑连接 WiFi： YaoKun_Drone（密码 12345678）。

浏览器打开 http://192.168.4.1    进入“设置”页面。

点击 陀螺仪校准，将飞机静置水平桌面，等待提示“校准成功”（两短鸣）。

确保飞机放在水平面上，点击 水平校准，听到三短鸣表示完成。

步骤 2：解锁
返回 遥控 页面。

将左摇杆（油门）拉到最底部，右摇杆归中。

打开页面上的 ARM 解锁 开关并保持约 0.8 秒，听到长鸣且红绿 LED 变为常亮，表示解锁成功。

步骤 3：起飞与悬停
缓慢向上推左摇杆（油门），电机开始旋转。

继续增加油门直至飞机离地（约 50% 油门）。

使用右摇杆（横滚/俯仰）控制飞机姿态，左摇杆左右控制偏航。

遥测页面会实时显示姿态角、电压、电机输出等数据。

步骤 4：降落与上锁
将油门拉到最低，飞机落地后，关闭 ARM 解锁开关，电机立即停转，红绿灯恢复呼吸模式。

常见问题：如果解锁后推油门无反应，请检查陀螺仪校准是否成功，或重新执行水平校准。详细故障排查请参考 Wiki。


## 📊 与其他开源飞控对比（以下评估内容由DeepSeek生成，仅供参考）

以下表格详细对比了耀坤飞控与 **Flix**、**ESP-FC**、**ESP-Drone** 在架构、算法、功能及教育适用性等方面的差异。

### 核心架构对比

| 架构维度 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|---------|---------|------|--------|-----------|
| 开发框架 | Arduino（模块化文件） | Arduino（<2000行） | 原生 ESP-IDF / Arduino | ESP-IDF + FreeRTOS |
| 控制循环频率 | 500 Hz | 1000 Hz | 1-2 kHz | 500 Hz |
| 操作系统 | 裸机 | 裸机 | 裸机 | FreeRTOS |
| 协议支持 | WebSocket + HTTP | MAVLink (QGC) | Betaflight Configurator | CRTP + cfclient |
| 参数配置方式 | 网页直连调参 | MAVLink 参数服务 | Betaflight 配置器/CLI | cfclient 上位机 |

### 姿态解算与传感器融合

| 算法维度 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|---------|---------|------|--------|-----------|
| 姿态解算器 | **Madgwick / Mahony / 互补滤波**（3种可选+热切换） | 互补滤波 | 四元数+互补滤波 | Crazyflie EKF |
| 滤波器种类 | 低通 + 加速度计信任度自适应 | LPF | LPF、动态Notch、RPM滤波 | 标准滤波 |
| 陀螺仪支持 | MPU6050/6500（自动检测 I2C/SPI） | MPU9250/6500/6050 | MPU6050/9250, ICM20602, BMI160 | MPU6050 |
| 自适应算法 | ✅ 加速度计信任度动态调整 Beta | ❌ | ✅ 动态 Notch 滤波 | ❌ |
| 滤波器调参 | Web 界面 | CLI | Betaflight 界面 | cfclient |

### PID 控制与飞行性能

| 控制维度 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|---------|---------|------|--------|-----------|
| 控制架构 | **串级 PID**（角度环→角速率环） | 三维二级串级 PID | Betaflight 风格 PID | Crazyflie PID |
| 偏航控制 | 独立偏航角速率 PID | 集成在三维 PID | 标准 PID | 标准 PID |
| 扭矩缩放/抗饱和 | ✅ 低油门动态缩放 + 积分抗饱和 | ❌ | Betaflight 标准 | 标准 |
| 积分抗饱和 | ✅ | ✅ | ✅ | ✅ |
| 飞行模式 | 自稳（可扩展定高） | ACRO / ANGLE | ACRO / ANGLE / AIRMODE | 自稳 / 定高 / 定点 |

### 开发门槛与教育适用性

| 教育维度 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|---------|---------|------|--------|-----------|
| 代码行数 | ~2500行，模块化 | <2000行，模块化 | 较大（模拟Betaflight） | 中等（FreeRTOS） |
| 学习难度 | **★★☆☆☆**（Arduino友好） | ★★☆☆☆ | ★★★★☆ | ★★★★☆ |
| 中文文档 | ✅ 完善（注释+教程） | 社区翻译 | 较少 | ✅ 官方中文文档 |
| 推荐人群 | 嵌入式初学者/电子爱好者/教师/科创竞赛 | 极简风格爱好者 | 穿越机进阶玩家 | 专业开发者 |
| 教学价值 | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★★★☆ |

### 功能完整度对比

| 功能 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|------|---------|------|--------|-----------|
| WiFi 无线控制 | ✅ Web 网页（免APP） | ✅ MAVLink+QGC | ✅ ESP-NOW | ✅ APP/游戏手柄 |
| 手机 APP 控制 | 通过网页（无需安装） | QGC 地面站 | ❌ | ✅ 专用 APP |
| 地面站支持 | 网页定制 | QGroundControl | Betaflight Configurator | cfclient |
| 黑盒记录 | ❌（可扩展 SD） | ✅ 日志分析 | ✅ | ✅ |
| 电机测试 | ✅ 网页端 | 未明确 | ✅ Betaflight 滑块 | ✅ 上位机调试 |
| 传感器校准 | ✅ 陀螺仪/水平 | ✅ IMU 校准 | ✅ Betaflight 界面 | ✅ cfclient |
| 定高/定点 | 可扩展（预留 I2C/SPI） | 计划中 | ❌ | ✅ |

### 综合评分（5★最高）

| 评价维度 | 耀坤飞控 | Flix | ESP-FC | ESP-Drone |
|---------|---------|------|--------|-----------|
| 易用性/学习曲线 | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★★☆☆ |
| 教学与文档质量 | ★★★★★ | ★★★★☆ | ★★☆☆☆ | ★★★★☆ |
| 功能完整度 | ★★★★☆ | ★★★☆☆ | ★★★★★ | ★★★★☆ |
| 代码可读性与模块化 | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★★☆☆ |
| 科创/展示效果 | ★★★★★ | ★★★☆☆ | ★★☆☆☆ | ★★★★☆ |

**对比总结**：耀坤飞控的差异化定位是 **“麻雀虽小五脏俱全”**，在保持 Arduino 低门槛的基础上，实现了商业级飞控的核心特性。它不仅是一个可以飞的无人机，更是一本活教材——每个模块都在教用户如何在嵌入式系统中实现工程化的软件架构。尤其适合：正在学习嵌入式系统的学生、寻找开源飞控项目的开发者、需要教学案例的教师、以及希望快速进行算法验证的科研人员。
