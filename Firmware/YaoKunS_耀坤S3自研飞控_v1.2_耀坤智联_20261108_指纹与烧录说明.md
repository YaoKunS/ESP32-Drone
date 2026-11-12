# 耀坤S3自研飞控 / 耀坤智联 —— 固件指纹与烧录说明

## 一、这份固件是什么

纯自研 ESP32-S3 四轴有刷无人机飞控（工程名 **YaoKunDrone**），不是 ESP-Drone / LiteWing / Flix 的派生版本，控制算法、Web 遥控界面、音效灯效全部自研。

- **硬件对应**：立创 ESP32-S3 R8N8 开发板（8MB Flash + 8MB OPI PSRAM），即完整版 PCB「YaoKun_K1.0」（板型宏 `PCB_YaoKun_LCKFB_ESP32S3`）
- **传感器**：MPU6050 / MPU6500 六轴（I2C，上电自动识别，WHO_AM_I 判定）
- **控制**：串级双环 PID（角度环 → 角速度环 → 四轴混控）+ 姿态解算（互补滤波 / Mahony / Madgwick 可在网页里切换）
- **遥控**：飞机自建 WiFi 热点，手机或电脑浏览器直接打开 `192.168.4.1` 遥控，WebSocket 双向通信（上行摇杆/命令，下行 20Hz 遥测），无需 App、无需遥控器
- **音效灯效**：14 种蜂鸣音效（上电/解锁/起飞/低电/失联/校准…非阻塞状态机）+ 红绿蓝三色灯语（红绿走 LEDC 硬件 PWM，蓝灯指示 WebSocket 连接）
- **界面**：Android 风格 Web UI，12 套主题，姿态球（ADI 民航姿态仪风格）
- **不含**：MAVLink / CRTP / SBUS / 光流 / 气压计（本版本为纯姿态自稳 + 手动油门）

> ★ 默认热点：**SSID = `YaoKun_Drone`，密码 = `12345678`**
> 刷完后手机连该热点，浏览器访问 <http://192.168.4.1/> 即可。
>
> ★ 本固件只适配「立创 ESP32-S3 R8N8 全功能板」这一种板型。
> 若你的机器是 Supermini 版（S1.0A / S1.0B / S1.0C），马达与传感器引脚不同，
> 直接刷本固件会出现电机不转或姿态错乱，必须改 `config.h` 的板型宏后重新编译。

## 二、版本与来源

| 项目 | 内容 |
|---|---|
| 固件版本 | **1.2**（源码 `YaoKun_Drone.ino` 头部 @version 1.0 / 20260613） |
| 品牌 | YaoKunS / 耀坤智联 / 耀坤无人机 |
| 编译时间 | Nov  8 2026 18:06:41 (GMT+8) |
| 源码目录 | `D:\YaoKunS\YaoKunDrone` |
| 工具链 | arduino-cli 1.5.1 + esp32 core 3.3.8（内含 ESP-IDF v5.5.4） |
| 编译参数 | `esp32:esp32:esp32s3` + `FlashSize=8M, PSRAM=opi, PartitionScheme=no_ota, FlashMode=qio, CPUFreq=240, USBMode=hwcdc, CDCOnBoot=cdc` |
| 第三方库 | ESP Async WebServer 3.12.1 (ESP32Async) ／ Async TCP 3.5.0 (ESP32Async) |
| 资源占用 | 应用 1128608 B / 2MB（54%）；静态变量 47808 B / 320KB（14%） |

> ⚠️ **关于「读版本号」的说明**（与 ESPDrone 固件不同，别照那套读）：
> Arduino 构建的镜像里，`0x10020` 处的 `esp_app_desc_t` 是由 ESP32 核心预编译库写入的，`version` 字段读出的是核心库标识（`14a0af9`），`project_name` 读出的是 `arduino-lib-builder`，都不代表本工程版本。
> 本固件的版本统一由**开机 IDENTITY 横幅**打印（见第四节②），校验版本请以 IDENTITY 横幅 + SHA-256 为准。

## 三、怎么烧（Flash Download Tool）

1. 打开 Flash Download Tool，芯片选 **ESP32-S3**，工作模式 **Develop**。
2. 只添加这一个文件，地址填 **`0x0`**：
   `YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin`
3. 设置：**SPI SPEED = 80MHz，SPI MODE = DIO，FLASH SIZE = 8MB**（16MB 的板子也选 8MB），COM 口选你的板子，BAUD 选 921600（不稳就降到 115200）。
4. ⚠️ **务必勾选 DoNotChgBin**。
   原因：镜像头里已写好 SPI 模式/频率/容量（DIO 80MHz 8MB）与镜像尾部 SHA-256，工具若按所选参数改写头部，会破坏校验和导致无法开机。勾上就按原样写入。
5. 点 START 等待完成，然后按 RST 或重新上电。

用 esptool 一条命令也行（等价）：

```bash
esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin
```

烧完若「不跑、停在下载模式」（日志出现 `waiting for download`）：说明 GPIO0 被拉低没放开。
先 `--after no-reset` 连一下确认镜像可读，再执行 `esptool --after watchdog-reset run`，或直接拔插 USB / 按 RST。

## 四、怎么验证烧进去的是这一版（指纹探测）

**① 直接比文件哈希**（最可靠，不用上电）：

```bat
certutil -hashfile "YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin" SHA256
```

→ 应为 `30c63a4595365286b72e9935a9e999777b36118730a5e973e51cce3636c20472`

**② 串口开机 IDENTITY 横幅**（115200，走原生 USB-CDC 口），开机第一屏就是：

```
IDENTITY: ============================================
IDENTITY:   YaoKunS   耀坤S3自研飞控 / YaoKunDrone
IDENTITY:   Version : v1.2
IDENTITY:   Source  : 20260613
IDENTITY:   Built   : Nov  8 2026 18:06:41   ← 本次编译时刻，写在固件里
IDENTITY:   Author  : 耀坤 <espnow@163.com>
IDENTITY:   BiliBili : 耀坤智联  https://space.bilibili.com/3707011699050767/
IDENTITY:   OSHWHub  : 耀坤无人机  https://oshwhub.com/lindahe/
IDENTITY:   Taobao   : 耀坤智联  https://shop35432590.taobao.com/
IDENTITY:   GitHub   : https://github.com/YaoKunS/
IDENTITY: ============================================
```

横幅后面接着应出现：`Calibrating gyro... keep still` → `Gyro calibration done` → `Ready. Open http://192.168.4.1/`（收尾标志行）。

**③ 连上热点验证界面**（最直观）：
WiFi 搜到 `YaoKun_Drone` → 密码 `12345678` → 浏览器 <http://192.168.4.1/>。
网页标题应为「耀坤无人机Web遥控」，界面右下角有 12 套主题切换，姿态页为蓝天地平的民航姿态仪（ADI）样式。

**④ 在 bin 文件里搜明文**（不用设备也能确认是本工程）：

- 搜索 `YaoKun_Drone` → 命中 1 次（默认热点名）
- 搜索 `耀坤无人机Web遥控` → 命中 1 次（网页标题）
- 搜索 `Madgwick` / `Mahony` → 各命中 1 次（姿态滤波器名）
- 搜索 `shop35432590.taobao.com` → 命中 1 次（IDENTITY 横幅里的淘宝店链接）

**⑤ 读 flash 里的镜像头**（不改动任何东西）：

```bash
esptool --chip esp32s3 --port COMx read_flash 0x10000 0x200 app_hdr.bin
```

→ 首字节应为 `E9`（ESP 镜像魔数）；`+0x20` 处为 `esp_app_desc_t`（magic `32 54 CD AB`），但 version 字段无参考价值（见第二节说明）。

## 五、产物与校验（SHA-256）

| 产物 | 烧写地址 | SHA-256 | 大小 |
|---|---|---|---|
| **merged 单文件固件** | `0x0` 整体刷 | `30c63a4595365286b72e9935a9e999777b36118730a5e973e51cce3636c20472` | 1194144 B (0x1238A0) |
| bootloader | `0x0` | `6f232c4aafe7d21239a1c59404723ca2cfdca063ef73501a21126a8cef39f432` | 19984 B (0x4E10) |
| 分区表 | `0x8000` | `6a88d59601a83a16a19a08114b59d338324b8dec267d8b43e5d61ad56ec92102` | 3072 B (0xC00) |
| boot_app0 | `0xE000` | esp32 core 3.3.8 自带（把 otadata 选到 app0，缺它不启动） | 8192 B |
| 应用程序 | `0x10000` | `5d5083e72b084f4de7fecfb65062b5fecdfb93a1cfc97c80fb801d8843f24da7` | 1128608 B (0x1138A0) |

- 合并段布局：`0x0 bootloader + 0x8000 分区表 + 0xE000 boot_app0 + 0x10000 应用程序`（段间空隙由 esptool 填 0xFF，文件在应用程序末尾即止）
- 分区表：nvs 0x9000(20K) / otadata 0xE000(8K) / app0 0x10000(2M) / spiffs 0x210000(1.875M) / coredump 0x3F0000(64K)
- 镜像头：`E9 xx 02 3F` → SPI 模式 DIO、80MHz、8MB

## 六、v1.2 相对 v1.1 的变更（PCB v2 适配，均为实机实测）

1. **SPI IMU 片选更正：GPIO17 → GPIO42**。PCB v2 实测：CS 接 17 时 WHO_AM_I 读到 0x00（线上无器件），接 42 读到 0x70（MPU6500），配置寄存器写入回读一致、静止 |a|≈1.05 g。GPIO17 是给光流预留的片选，不是 IMU 的（将来加光流时 17 需保持高电平）。
   🔴 用 v1 老板子（SPI 片选 = 17）的用户：本版在旧板上探测不到 SPI IMU，请改用 I2C 座（6050 系），或取 v1.1。
2. **IMU 方向自动适配（总线 + 芯片指纹双判据）**：
   - I2C 排母（SDA 9 / SCL 8）→ 一律按 6050 系处理（SwapXY 开、不翻转）；进口原厂（0x68）与国产替代（实测报 0x68 或 0x70）行为一致。
   - SPI 座（CS 42）→ WHO 为 0x70/0x71/0x73/0x74 时按 6500/9250 系处理（SwapXY 开 + FlipX/FlipY 开，等效绕 Z 轴 180°）。
   - ⇒ 三种模块插上即用，**不需要改 config.h 宏、不需要重新编译**；未识别指纹保留编译期宏兜底并打印提示。
3. 开机日志新增 `IMU axis map: ...` 一行，打印本次实际生效的映射与判定依据——"方向不对"时先看这一行。
4. 6050 系与 6500 系陀螺 Z 轴极性物理相反，换 6050 后网页 yaw 显示方向可能与 6500 相反，属芯片差异，不影响飞行控制。
5. 本版未改动混控、PID 与控制增益：飞行手感与 v1.1 一致。

## 七、v1.1 相对 v1.0 的变更（均为真机实测问题驱动）

1. **关闭芯片欠压复位（brownout）**：v1.0 实测「放地上推油即将离地必重启」——根因是电机满载浪涌把 3.3V 拉到欠压阈值以下，芯片硬复位。本版按 Flix 同款机制关闭 brownout 检测。⚠️ 代价：真欠压时芯片不再自保护复位，只剩蜂鸣器三级告警（<3.70V 慢鸣 / <3.50V 快鸣 / <3.30V 急促），请飞手留意电压，及时返航更换电池。
2. **偏航 PID 加积分：Ki 0 → 0.005**。消除电机/桨扭矩不对称带来的匀速自旋（v1.0 的 yaw 环是纯 P，只能「残余转速换反扭矩」）。注意：整片刷写会清 NVS、新默认自动生效；若你的板子保存过参数，请到网页把偏航 Ki 手动改为 0.005（或点恢复默认）。
3. **偏航扭矩符号适配（`kYawTorqueSign = -1`）**：适配装机实测电机布局：左前(GPIO16)=顺时针、右前(GPIO6)=逆时针、左后(GPIO15)=逆时针、右后(GPIO5)=顺时针（对角两两同向）。与混控默认约定整体相反，不翻转符号时 yaw 环正反馈 → 起飞后加速自旋、「螺旋升天」。翻转点在 PID 输出之后：网页摇杆 / 遥控器杆向均不变。⚠️ 换机架/换电机方向布局的板子请改 `config.h` 的 `kYawTorqueSign`。
4. **出厂提示**：装机后请务必「不装桨」逐个点测试页 M0~M3 验证转向，四颗转向需与本条布局一致（转向错误的电机对调两根线即可修正）。
5. **试飞提示**：首次试飞前，把飞机放在确认水平的台面上，在网页里做一次「水平校准」（未解锁状态点，三短鸣为成功）。纯姿态自稳无法消除的水平缓漂属正常现象，飞行中需飞手打杆修正；固定方向的明显偏飘多为重心偏移，左右挪动电池位置即可改善。

## 八、六点须知

1. **单文件从 0x0 整片刷写，会把 0x9000~0x10000（nvs：WiFi 校准数据 + 已保存参数）一并写成 0xFF，等于清空。** 刷新板/空板无影响；刷自己已经调好 PID 的机器要留意，这种场合请改用分片写（只写 `0x0 / 0x8000 / 0xE000 / 0x10000` 四段，跳过 0x9000）。
2. 本版分区方案为 no_ota（单 app 2MB），没有 OTA 分区 —— 升级只能重新插线刷写。
3. 编译时 CDCOnBoot=cdc：Serial 输出走**原生 USB 口**。如果你只插了板上的 UART 口，看不到开机日志是正常的，改用原生 USB 口即可。
4. FLASH SIZE 请按 8MB 选；16MB 的板子（N16R8）也能刷，只是后 8MB 用不到。
5. 本机是「纯姿态自稳 + 手动油门」，没有 GPS / 气压计 / 测距 / 光流，**松杆不会悬停**（保持既有水平速度继续飘），松油门就会掉高度；首次试飞请在空旷场地、低空、有保护圈的前提下进行，并先确认网页里「解锁」与「上锁」正常。
6. 网页里改了 PID / 校准参数会自动存 NVS，下次上电仍在；要恢复出厂用网页的「恢复默认」按钮（不要用整片刷写来"恢复"，那会连带清掉 WiFi 校准）。

## 九、找到我们

- B站（哔哩哔哩）：[耀坤智联](https://space.bilibili.com/3707011699050767)
- 立创开源硬件平台：[耀坤无人机](https://oshwhub.com/lindahe)
- GitHub：<https://github.com/YaoKunS>
- 淘宝店：[耀坤智联](https://shop35432590.taobao.com/)

---

本说明由固件打包脚本自动生成；哈希、分段布局与编译时间均现场从固件镜像中提取，与 bin 严格一致。
