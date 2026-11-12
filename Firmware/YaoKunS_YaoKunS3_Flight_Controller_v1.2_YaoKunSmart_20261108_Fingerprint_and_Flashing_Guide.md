# YaoKunS S3 Flight Controller / YaoKunSmart — Firmware Fingerprint & Flashing Guide

> English version of `YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_指纹与烧录说明.md`.
> The firmware binary referenced here is
> `Firmware/YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin` (same folder).

## 1. What this firmware is

A fully in-house developed ESP32-S3 brushed-quadcopter flight controller (project name **YaoKunDrone**). It is NOT a derivative of ESP-Drone / LiteWing / Flix — the control algorithms, the web remote UI, and the audio/LED effects are all original work.

- **Target hardware**: LCKFB (LCSC) ESP32-S3 R8N8 dev board (8 MB Flash + 8 MB OPI PSRAM), i.e. the full-featured PCB "YaoKun_K1.0" (board macro `PCB_YaoKun_LCKFB_ESP32S3`)
- **Sensors**: MPU6050 / MPU6500 6-axis IMU (I2C, auto-detected at boot via WHO_AM_I)
- **Control**: cascaded dual-loop PID (attitude loop → angular-rate loop → quad mixer) + attitude estimation (complementary / Mahony / Madgwick, switchable in the web UI)
- **Remote control**: the drone hosts its own WiFi access point; open `192.168.4.1` in any phone/PC browser to fly. Bidirectional WebSocket (uplink: sticks & commands; downlink: 20 Hz telemetry). No app, no RC transmitter needed.
- **Audio & LEDs**: 14 buzzer effects (power-on / arm / takeoff / low battery / failsafe / calibration… non-blocking state machine) + RGB LED signals (red & green on LEDC hardware PWM, blue indicates WebSocket connection)
- **UI**: Android-style web UI, 12 themes, attitude indicator (ADI / civil-aviation style)
- **Not included**: MAVLink / CRTP / SBUS / optical flow / barometer (this version is attitude-stabilized + manual throttle only)

> ★ **Default hotspot: SSID = `YaoKun_Drone`, password = `12345678`**
> After flashing, connect your phone to this hotspot and open <http://192.168.4.1/>.
>
> ★ This firmware fits **only** the "LCKFB ESP32-S3 R8N8 full-featured board".
> If your drone is a Supermini variant (S1.0A / S1.0B / S1.0C), motor and sensor pins differ;
> flashing this image as-is will cause dead motors or wrong attitude. You must change the
> board macro in `config.h` and rebuild.

## 2. Version & provenance

| Item | Value |
|---|---|
| Firmware version | **v1.2** (source header `@version 1.2 / 20260613` in `YaoKun_Drone.ino`) |
| Brand | YaoKunS / YaoKunSmart / 耀坤无人机 |
| Built | Nov  8 2026 18:06:41 (GMT+8) |
| Source directory | `D:\YaoKunS\YaoKunDrone` |
| Toolchain | arduino-cli 1.5.1 + esp32 core 3.3.8 (bundles ESP-IDF v5.5.4) |
| Build options | `esp32:esp32:esp32s3` + `FlashSize=8M, PSRAM=opi, PartitionScheme=no_ota, FlashMode=qio, CPUFreq=240, USBMode=hwcdc, CDCOnBoot=cdc` |
| Third-party libs | ESP Async WebServer 3.12.1 (ESP32Async) / Async TCP 3.5.0 (ESP32Async) |
| Resource usage | app 1128608 B / 2 MB (54%); static RAM 47808 B / 320 KB (14%) |

> ⚠️ **About "reading the version number"** (different from the ESPDrone firmware — do not follow that procedure):
> In an Arduino-built image, the `esp_app_desc_t` at `0x10020` is written by the ESP32 Arduino core's prebuilt libraries: the `version` field reads back the core-library id (`14a0af9`) and `project_name` reads back `arduino-lib-builder`. Neither identifies this project.
> This firmware prints its version in the **boot-time IDENTITY banner** (see section 4-②); verify versions with the IDENTITY banner + SHA-256.

## 3. How to flash (Flash Download Tool)

1. Open Flash Download Tool, chip = **ESP32-S3**, work mode = **Develop**.
2. Add this single file only, address = **`0x0`**:
   `YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin`
3. Settings: **SPI SPEED = 80MHz, SPI MODE = DIO, FLASH SIZE = 8MB** (also choose 8MB on 16MB boards), pick your COM port, BAUD 921600 (drop to 115200 if unstable).
4. ⚠️ **You MUST tick DoNotChgBin**.
   Why: the image header already carries the SPI mode/frequency/size (DIO 80MHz 8MB) and a trailing SHA-256. If the tool rewrites the header for the selected parameters, the checksum breaks and the board will not boot. Ticking it writes the image byte-for-byte as-is.
5. Click START, wait for completion, then press RST or power-cycle.

Equivalent one-liner with esptool:

```bash
esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin
```

If after flashing it "doesn't run and sits in download mode" (log shows `waiting for download`): GPIO0 is being held low. First connect with `--after no-reset` to confirm the image is readable, then run `esptool --after watchdog-reset run`, or simply re-plug the USB / press RST.

## 4. How to verify this is exactly this build (fingerprinting)

**① Compare the file hash** (most reliable, no hardware needed):

```bat
certutil -hashfile "YaoKunS_耀坤S3自研飞控_v1.2_耀坤智联_20261108_merged.bin" SHA256
```

→ expected `30c63a4595365286b72e9935a9e999777b36118730a5e973e51cce3636c20472`

**② Boot-time IDENTITY banner** (115200 baud, on the native USB-CDC port) — the very first thing printed:

```
IDENTITY: ============================================
IDENTITY:   YaoKunS   耀坤S3自研飞控 / YaoKunDrone
IDENTITY:   Version : v1.2
IDENTITY:   Source  : 20260613
IDENTITY:   Built   : Nov  8 2026 18:06:41   ← build moment, baked into the image
IDENTITY:   Author  : 耀坤 <espnow@163.com>
IDENTITY:   BiliBili : 耀坤智联  https://space.bilibili.com/3707011699050767/
IDENTITY:   OSHWHub  : 耀坤无人机  https://oshwhub.com/lindahe/
IDENTITY:   Taobao   : 耀坤智联  https://shop35432590.taobao.com/
IDENTITY:   GitHub   : https://github.com/YaoKunS/
IDENTITY: ============================================
```

Right after the banner you should see: `Calibrating gyro... keep still` → `Gyro calibration done` → `Ready. Open http://192.168.4.1/` (final marker line).

**③ Join the hotspot and check the UI** (most intuitive):
Find WiFi network `YaoKun_Drone` → password `12345678` → browser <http://192.168.4.1/>.
The page title should be 「耀坤无人机Web遥控」 (*YaoKun Drone Web RC*), with 12 switchable themes and a blue-sky/brown-ground ADI-style attitude indicator.

**④ Search for plain strings in the .bin** (confirm provenance without any hardware):

- search `YaoKun_Drone` → 1 hit (default hotspot name)
- search `耀坤无人机Web遥控` → 1 hit (web page title)
- search `Madgwick` / `Mahony` → 1 hit each (attitude filter names)
- search `shop35432590.taobao.com` → 1 hit (Taobao link in the IDENTITY banner)

**⑤ Read the image header from flash** (changes nothing):

```bash
esptool --chip esp32s3 --port COMx read_flash 0x10000 0x200 app_hdr.bin
```

→ first byte must be `E9` (ESP image magic); at `+0x20` sits `esp_app_desc_t` (magic `32 54 CD AB`), but its `version` field is meaningless here (see section 2).

## 5. Artifacts & checksums (SHA-256)

| Artifact | Flash address | SHA-256 | Size |
|---|---|---|---|
| **merged single-file firmware** | flash whole file at `0x0` | `30c63a4595365286b72e9935a9e999777b36118730a5e973e51cce3636c20472` | 1194144 B (0x1238A0) |
| bootloader | `0x0` | `6f232c4aafe7d21239a1c59404723ca2cfdca063ef73501a21126a8cef39f432` | 19984 B (0x4E10) |
| partition table | `0x8000` | `6a88d59601a83a16a19a08114b59d338324b8dec267d8b43e5d61ad56ec92102` | 3072 B (0xC00) |
| boot_app0 | `0xE000` | bundled with esp32 core 3.3.8 (selects app0 in otadata; the board won't boot without it) | 8192 B |
| application | `0x10000` | `5d5083e72b084f4de7fecfb65062b5fecdfb93a1cfc97c80fb801d8843f24da7` | 1128608 B (0x1138A0) |

- Merged layout: `0x0 bootloader + 0x8000 partition table + 0xE000 boot_app0 + 0x10000 application` (gaps filled with 0xFF by esptool; the file ends right after the application)
- Partition table: nvs 0x9000 (20K) / otadata 0xE000 (8K) / app0 0x10000 (2M) / spiffs 0x210000 (1.875M) / coredump 0x3F0000 (64K)
- Image header: `E9 xx 02 3F` → SPI mode DIO, 80 MHz, 8 MB

## 6. What changed in v1.2 (vs v1.1) — PCB v2 support

1. **SPI IMU chip-select corrected: GPIO17 -> GPIO42.** On PCB v2, CS=17 reads WHO_AM_I=0x00 (no device on that line); CS=42 reads 0x70 (MPU6500), with configuration registers written and read back correctly and |a|~1.05 g at rest. GPIO17 is reserved for the optical-flow module, not the IMU (keep 17 high when adding optical flow).
   NOTE: on the older v1 board (SPI CS = 17) this build will not detect an SPI IMU; use the I2C socket (6050 family) or stay on v1.1.
2. **IMU orientation auto-adaptation (bus + fingerprint)**:
   - I2C socket (SDA 9 / SCL 8) -> always treated as the 6050 family (SwapXY on, no flips); genuine (WHO=0x68) and clone parts (which report 0x68 or 0x70) behave identically.
   - SPI socket (CS 42) -> WHO of 0x70/0x71/0x73/0x74 is treated as the 6500/9250 family (SwapXY on + FlipX/FlipY on, i.e. 180 deg about Z).
   - => All three module types work out of the box; **no macro edit and no recompile needed**. Unknown fingerprints fall back to the compile-time defaults and a notice is printed.
3. New boot-log line `IMU axis map: ...` reports the orientation mapping actually applied — check it first if attitude axes look wrong.
4. The gyro Z polarity of the 6050 and 6500 families differs physically; after swapping to a 6050, the web yaw indication may point the opposite way. This is a chip difference and does not affect flight control.
5. Mixer, PID and control gains are unchanged: flight feel is identical to v1.1.

## 7. What changed in v1.1 (vs v1.0) — all driven by real-flight findings

1. **Chip brownout detector disabled**: on v1.0, arming on the ground and pushing throttle to the lift-off point always caused a reboot — the culprit was the motor surge sagging the 3.3 V rail below the chip's brownout threshold. This build disables the brownout detector (same approach as the Flix firmware). ⚠️ Trade-off: on a real undervoltage the chip no longer self-resets; only the three-stage buzzer warning remains (<3.70 V slow beeps / <3.50 V fast beeps / <3.30 V urgent). Watch your battery voltage and land early.
2. **Yaw PID now has integral term: Ki 0 → 0.005**. Removes the steady yaw spin caused by motor/prop torque asymmetry (v1.0's yaw loop was pure P). Note: whole-image flashing wipes NVS so the new default applies automatically; if your board has saved parameters, set yaw Ki to 0.005 manually in the web UI (or use "restore defaults").
3. **Yaw torque sign adaptation (`kYawTorqueSign = -1`)**: matches the as-built motor layout — front-left (GPIO16) = CW, front-right (GPIO6) = CCW, rear-left (GPIO15) = CCW, rear-right (GPIO5) = CW (diagonal pairs spin the same way). This is the exact opposite of the mixer's default convention; without the sign flip the yaw loop becomes positive feedback → accelerating spin / "spiral to the sky" after take-off. The flip is applied after the PID output, so web-joystick / transmitter stick directions are unchanged. ⚠️ If you build onto a different frame or motor layout, adjust `kYawTorqueSign` in `config.h`.
4. **Factory check**: after assembly, always verify each motor's spin direction without props, one at a time from the web test page (M0 FL / M1 FR / M2 RR / M3 RL), matching the layout above (a wrong-direction motor is fixed by swapping its two wires).
5. **First-flight tip**: before the first flight, put the craft on a known-level surface and run one web-UI "level calibration" (while disarmed; three short beeps = success). A mild residual horizontal drift is inherent to attitude-only stabilization and must be corrected with the sticks in flight; a consistent strong drift to one side usually means the center of gravity is off — shift the battery sideways.

## 8. Six things to know

1. **Flashing the single file from 0x0 also writes 0xFF over 0x9000–0x10000 (nvs: WiFi calibration + saved parameters), effectively erasing it.** Harmless on a blank/new board; if you are re-flashing a drone with tuned PIDs, use per-segment flashing instead (write only `0x0 / 0x8000 / 0xE000 / 0x10000`, skip 0x9000).
2. This build uses the no_ota partition scheme (single 2 MB app) — **no OTA partition**, upgrades require a wired re-flash.
3. Built with CDCOnBoot=cdc: `Serial` output goes to the **native USB port**. If you only plug the board's UART port you will see no boot log — that is normal; use the native USB port.
4. Choose FLASH SIZE = 8MB; a 16 MB board (N16R8) flashes fine, it just leaves the upper 8 MB unused.
5. This craft is "attitude-stabilized + manual throttle" only — no GPS / barometer / rangefinder / optical flow. **It will not hover hands-off** (it keeps drifting with whatever horizontal velocity it had), and releasing throttle loses altitude. For a first flight: open area, low altitude, prop guards on, and verify ARM/DISARM in the web UI first.
6. PID / calibration changes made in the web UI are stored in NVS and survive reboots. To factory-reset use the "restore defaults" button in the web UI (do not factory-reset by whole-image flashing — that also wipes the WiFi calibration data).

## 9. Find us

- Bilibili: [YaoKunSmart](https://space.bilibili.com/3707011699050767)
- LCSC OSHWHub: [YaoKun Drone](https://oshwhub.com/lindahe)
- GitHub: <https://github.com/YaoKunS>
- Taobao store: [YaoKunSmart](https://shop35432590.taobao.com/)

---

This guide is auto-generated by the firmware packaging script; all hashes, the segment layout and the build timestamp are extracted from the firmware image at packaging time, and always match the shipped .bin exactly.
