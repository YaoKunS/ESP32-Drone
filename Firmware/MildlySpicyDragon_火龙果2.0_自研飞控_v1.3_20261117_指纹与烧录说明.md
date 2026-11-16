# 微辣火龙果 2.0 · 耀坤自研飞控 v1.3 —— 指纹与烧录说明

> 固件：**YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_merged.bin**（1,209,936 字节，单文件从 `0x0` 刷完）
> 版本：**v1.3**　构建：**Nov 17 2026 00:14:55**　打包日期：20261117
> SHA-256（merged）：`aa0171c36c8014b26a2500f9b96eedffbe6d8297ffa231457978efa59f1d2b20`
> SHA-256（app 分片）：`b9f442c10427fb0ba9f5702ff849b549955f0053e402cb8cf09fde1b4e7a1c55`

---

## 一、这是什么

耀坤自研飞控（YaoKunDrone）**为微辣火龙果 2.0 版 PCB 适配的固件**。
同一份源码，靠编译期板型宏切换两套引脚与灯语：

| | 微辣火龙果 2.0（本固件） | 耀坤 S3 版 |
| --- | --- | --- |
| 芯片 | ESP32 经典片（DevKit V1） | ESP32-S3 |
| Flash | 4 MB | 8 MB |
| IMU | SPI 6500/9250（CS5）或 I2C 6050（21/22） | SPI（CS42）或 I2C（9/8） |
| 灯 | 单灯 GPIO2 | 三色 41/7/14 |
| 编译宏 | `-DPCB_HLG_V2` | 无（默认） |

> 🔴 两套固件**不能互刷**：引脚不同，刷错会四轴乱序起转。认板看开机横幅的 `Board` 行。

---

## 二、包内文件

| 文件 | 用途 |
| --- | --- |
| `YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_merged.bin` | **单文件整片固件**，地址 `0x0`，适合新板 / 想重置 |
| `YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_app_0x10000.bin` | 仅应用分片，地址 `0x10000`，**NVS 参数（标定/PID/热点名）全保留**，日常升级用 |

---

## 三、烧录

### 方式 A：Flash Download Tool（推荐，图形界面）

1. 芯片选 **ESP32**（⚠️ 不是 ESP32-S3）、Develop 模式、波特率 460800；
2. 只添加一个文件 `YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_merged.bin`，地址填 **`0x0`**；
3. START → 等 `FINISH`，按 RST 复位。

### 方式 B：esptool

```bash
# 整片烧（清 NVS：标定、热点名、PID 全回出厂）
python -m esptool --port COMx --baud 460800 --after hard-reset \
  write-flash 0x0 YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_merged.bin

# 只更 app（NVS 全保留）
python -m esptool --port COMx --baud 460800 --after hard-reset \
  write-flash 0x10000 YaoKunS_HLG2_YaoKunDrone_FW_v1.3_20261117_app_0x10000.bin
```

> 🔴 本板 bootloader 地址是 **`0x1000`**（经典片规则），S3 才是 `0x0`——单独烧 bootloader 时别照抄。
> 🔴 串口芯片是 CH340（虚拟 COM）。**别在串口助手里手动拉 DTR/RTS**，`DTR=1,RTS=0` 会把芯片按进复位。

---

## 四、首次上电（串口 115200）

```
IDENTITY:   微辣火龙果 2.0 飞控（耀坤自研飞控适配版） MildlySpicyDragon
IDENTITY:   Version : v1.3
IDENTITY:   Board   : HLG 2.0 (ESP32 classic, single LED)
LEDC attach: 1 1 1 1
IMU axis map: SPI 6500-family (WHO=0x70) -> swap + flipX + flipY
Detected MPU6500 (SPI)
Calibrating gyro... keep still
AP IP: 192.168.4.1
Ready. Open http://192.168.4.1/
```

| 项 | 值 |
| --- | --- |
| 热点名 | `YaoKun_Drone`（固定，不带 MAC 后缀） |
| 热点密码 | 12345678（8 位数字，逐位「一~八」，**中间没有连字符/空格**） |
| 网页调参 | 连上热点后浏览器打开 `http://192.168.4.1` |

**自研飞控没有串口命令行**（没有 `imu` / `p` / `cr` 这类命令），所有操作都在网页里。

---

## 五、刷完必须做的两件校准（🔴 否则起飞瞬间侧翻）

整片刷写会把 NVS 里的标定清零，姿态 = 原始欧拉角 − 水平零偏，零偏归 0 而机架不是绝对水平 ⇒ 姿态环带着虚假倾角猛纠 ⇒ **固定方向侧翻**（v1.2 实测踩过）。

1. 机架放平 → 网页「设置」→ **陀螺仪校准**（期间不要动板子）；
2. 同一页 → **水平校准** → 确认姿态球横滚/俯仰在 **±2°** 内；
3. 改过 PID / 限制参数后点 **保存参数**（否则重启即丢）。

---

## 六、引脚表（本板，权威定义见源码 `config.h` 的 `PCB_HLG_V2` 段）

| 功能 | GPIO | 备注 |
| --- | --- | --- |
| 电机 左前 / 右前 / 右后 / 左后 | 32 / 33 / 27 / 26 | 板丝印 M4 / M1 / M2 / M3 |
| IMU SPI SCK / MISO / MOSI / CS | 18 / 19 / 23 / **5** | 6500/9250 走这里 |
| IMU I2C SDA / SCL | 21 / 22 | 6050 排母 |
| 电池电压 ADC | **35** | ADC1_CH7，分压系数 2.0（必须 ADC1，ADC2 与 Wi-Fi 互斥） |
| 蜂鸣器 | 25 | 有源，高电平响 |
| 板载灯 | 2 | 单灯，拉电流接法，高电平亮 |
| 预留：激光测距 I2C | 17 / 16 | 未实现驱动 |
| 预留：SBUS 输入 | 4 | 未实现驱动 |

> 经典片只有 GPIO0~39；本固件所有引脚都在 39 以内，不碰 GPIO12（strapping），**不需要烧 eFuse**。

---

## 七、本版（v1.3）改了什么

1. **新增火龙果 2.0 板型**（`PCB_HLG_V2`）：电机 / IMU / ADC / 蜂鸣器 / 灯全部按本板引脚；与 S3 版共用同一套混控与控制算法（两板电机布局、IMU 装向、桨装法完全一致）。
2. **单灯合成层**：本板只有一颗灯，红/绿/蓝三路语义按「谁要亮就亮」合成到 GPIO2，闪频语义保留（启动交替、待机呼吸、解锁常亮、故障快闪）。
3. **火龙果专属品牌横幅**：开机横幅显示火龙果品牌与立创开源 / B 站链接，末尾保留耀坤智联淘宝行。
4. **S3 版无功能变化**：仅版本号与新增的 `Board` 行（便于现场区分两套固件）。

> 角速率环默认 PID 仍是 **0.010 / 0 / 0.00008**（v1.2 双 IMU 实飞验证值），本版未改。

---

## 八、排障速查

| 现象 | 处理 |
| --- | --- |
| 烧完 verified 但不启动、串口 0 字节 | 芯片选成 S3，或 bootloader 烧到 0x0（应 0x1000）。整片 bin 从 0x0 烧最稳 |
| 横幅 `Board` 显示 YaoKun S3 | 刷错固件，重烧本包 |
| `LEDC attach` 里出现 0 | 该路 PWM 没绑上，电机不会转；查引脚是否被占用 |
| 找不到热点 | 串口看有没有 `AP IP:`；无则供电不足或 Wi-Fi 初始化失败 |
| 姿态球方向反 | 先重做水平校准；仍反看 `IMU axis map` 那行 |
| 网页摇杆一停电机就停 | 油门回中即强制停转，属设计 |
| 飞起来自动上锁 | `cmd_timeout` 默认仅 300 ms，Wi-Fi 抖动即触发；网页「设置」可调到 2000 ms |

---

## 九、品牌与归属

- **硬件设计**：微辣火龙果（malagis）· 立创开源硬件平台 <https://oshwhub.com/malagis/esp32-mini-plane>
- **微辣火龙果 B 站**：<https://space.bilibili.com/544479100>
- **固件**：耀坤智联（YaoKunSmart）为火龙果 2.0 做适配与维护 · <https://github.com/YaoKunS/ESP32-Drone>
- **淘宝**：<https://shop35432590.taobao.com/>

> 说明：火龙果 2.0 是微辣火龙果作者的开源硬件项目（2025 星火计划获奖项目「ESP32 迷你无人机」），
> 本固件由耀坤智联**代为适配与维护**，不是耀坤自有产品；两板固件与品牌相互独立。
