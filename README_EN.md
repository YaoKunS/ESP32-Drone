<div align="center">

# ✈️ YaoKun Drone Flight Controller

**A fully attitude-stabilized ESP32-S3 quadcopter firmware — driven straight from a browser, every parameter tunable live**

[![Platform](https://img.shields.io/badge/Platform-Arduino-blue)](https://www.arduino.cc/)
[![MCU](https://img.shields.io/badge/MCU-ESP32--S3-purple)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Bilibili](https://img.shields.io/badge/Bilibili-%E8%80%80%E5%9D%A4%E6%99%BA%E8%81%94-ff69b4)](https://space.bilibili.com/3707011699050767)
[![OSHWHub](https://img.shields.io/badge/OSHWHub-oshwhub.com%2Flindahe-orange)](https://oshwhub.com/lindahe)

[**简体中文**](README.md) · **English**

</div>

---

> 📖 **This is the complete technical document** (18 chapters), not a README summary. It matches
> `Docs/YaoKunDrone_README.html` / `.pdf` / `.docx` one-to-one. The web version adds typography and a
> highlighted table of contents; this Markdown version is meant for searching and citing inside the repo.

---

## Table of Contents

- [1. Project Overview](#1-project-overview) — what it is, what it can do
- [2. Features](#2-features) — 14 capabilities
- [3. System Architecture](#3-system-architecture) — five layers and module dependencies
- [4. Control Loop](#4-control-loop) — the 500 Hz pipeline and its 6 safety gates
- [5. Control Algorithms](#5-control-algorithms) — cascaded dual-loop PID and mixing
- [6. Attitude Estimation](#6-attitude-estimation) — three filters and adaptive accel trust
- [7. Safety Mechanisms](#7-safety-mechanisms) — 8 fault sources and the arming state machine
- [8. Communication & Web Remote](#8-communication--web-remote) — protocol timeline and UI structure
- [9. Buzzer & LED System](#9-buzzer--led-system) — 14 sound patterns and the tri-color LED language
- [10. Hardware & Pinout](#10-hardware--pinout) — board selection and wiring
- [11. Build & Flash](#11-build--flash) — toolchain, libraries, steps, pitfalls
- [12. Usage](#12-usage) — from first power-up to flight
- [13. PID Tuning Guide](#13-pid-tuning-guide) — PID and limit tuning
- [14. Troubleshooting](#14-troubleshooting) — symptom → cause → fix
- [15. Design Trade-offs & Known Limits](#15-design-trade-offs--known-limits) — an honest list
- [16. Roadmap](#16-roadmap) — where the reserved pins are meant to go
- [17. Comparison with Other Open-Source Flight Controllers](#17-comparison-with-other-open-source-flight-controllers) — vs Flix / ESP-FC / ESP-Drone
- [18. Links & Support](#18-links--support) — GitHub / OSHWHub / Bilibili / Taobao

---

## 1. Project Overview

**YaoKun Drone** is an independently developed quadcopter flight controller firmware for the ESP32-S3. It runs on the Arduino framework and packs attitude estimation, cascaded PID control, safety protection, telemetry reporting and a browser-based remote control UI into **5,173 lines** of highly readable C/C++. There is no graphical host tool and no dedicated ground station: connect a phone to the aircraft's own Wi-Fi hotspot, open the web page, and you can fly, tune and monitor.

The goal was never "a toy that flies", but **an open-source flight controller that a single person can fully read, and safely modify** — a teaching and prototyping platform. Every constant in the code is annotated with where it came from, and every non-obvious design explains *why it is written that way* and *what breaks otherwise*. It doubles as a runnable flight-control textbook.

> 📌 **Current firmware version: v1.1 (2026-10-23)** — three changes since v1.0, all driven by real flight-test findings (details in the referenced sections): ① chip brownout reset disabled — fixes the guaranteed reboot at ground-level critical throttle, see Chapter 7; ② yaw loop gained an integral term Ki=0.005 — eliminates the constant spin caused by motor/prop torque asymmetry, see 5.5; ③ yaw torque sign adaptation (`kYawTorqueSign`) — matches the as-built motor layout, fixes the accelerating spin after takeoff, see 5.3.

### 1.1 Hardware at a Glance

The aircraft is a **free-to-fabricate PCB frame** plus an ESP32-S3 dev board, an MPU6050/6500 module and four 8520 coreless motors. The flight controller's hotspot is `YaoKun_Drone`. Photos below come from the open-source repository [github.com/YaoKunS/ESP32-Drone](https://github.com/YaoKunS/ESP32-Drone):

![Two complete aircraft side by side](Docs/img/K1_01.jpg)

*Fig. 1-1　Two complete aircraft side by side — both are LCKFB ESP32-S3 full-function boards*

![Top view: the PCB is the frame](Docs/img/K1_03.jpg)

*Fig. 1-2　Top view — the PCB **is** the frame: X-shaped arms, 8520 coreless motors, 65 mm props and removable prop guards*

![K1.0 close-up](Docs/img/K1_02.jpg)

*Fig. 1-3　K1.0 close-up — LCKFB ESP32-S3 R8N8 core board plugged straight into the PCB socket, 1S battery held by nylon ties*

![Close comparison of the two airframes](Docs/img/K1_04.jpg)

*Fig. 1-4　Close comparison — both are LCKFB ESP32-S3 full-function boards sharing one PCB frame design (different prop colors for identification)*

### 1.2 Flight Envelope — Read This First

> ⚠️ **This is a pure attitude-stabilization controller with manual throttle. It has no position or velocity sensor of any kind.**<br><br>The board carries a single 6-axis IMU — no GPS, no barometer, no rangefinder, no optical flow, no magnetometer. Therefore:

| Capability | Explanation |
|---|---|
| **It can** | Hold attitude: when you release the sticks the airframe levels itself. Roll, pitch and yaw are all controllable; throttle is manual. |
| **It cannot** | Hover in place. Release the sticks and attitude is levelled, but **the aircraft keeps drifting at whatever horizontal velocity it had**; release the throttle and **it descends**. |
| **Consequence** | Continuous small corrections are required — an "action-cam style" manual flight experience, not the GPS-hold feel of a consumer brushless quad. |

> 💡 Real position hold requires a laser rangefinder (VL53L1X) plus optical flow (PMW3901). The board already reserves SPI and a second I²C bus for exactly this — see [Chapter 16, Roadmap](#16-roadmap).

### 1.3 Target Platform & Open-Source Info

| Item | Details |
|---|---|
| MCU | ESP32-S3 (defaults to the LCKFB ESP32-S3 R8N8 dev board; ESP32-S3 Supermini series also supported) |
| Attitude sensor | MPU6050 (I²C) or MPU6500 (SPI), auto-detected at boot — no manual configuration |
| Motor output | Four LEDC hardware PWM channels, 20 kHz / 10-bit |
| Connectivity | Wi-Fi SoftAP + WebSocket (the aircraft is its own hotspot; no router needed) |
| Repository | [github.com/YaoKunS/ESP32-Drone](https://github.com/YaoKunS/ESP32-Drone) (GPL v3, full source and photos) |
| Open hardware | [OSHWHub — YaoKun's profile (oshwhub.com/lindahe)](https://oshwhub.com/lindahe) — the YaoKun Drone series (Spark Plan 2026 open-source project); schematics and PCB files are free to download |
| Component kit | Taobao store "耀坤智联" [shop35432590.taobao.com](https://shop35432590.taobao.com/) — basic parts from ¥9.9 |
| Video tutorials | Bilibili channel "耀坤智联" [space.bilibili.com/3707011699050767](https://space.bilibili.com/3707011699050767) |

## 2. Features

### 2.1 Flight Control

- **Cascaded dual-loop control** — roll and pitch use an outer angle loop feeding an inner angular-rate loop; yaw is a single rate loop. Scheduled at a fixed 500 Hz.
- **X-frame mixing** — the four motors sit at the diagonal corners, so a single motor failure still leaves usable thrust and partial control authority (better fault tolerance than an H-frame).
- **Low-throttle torque scaling** — control authority is scaled linearly with throttle to compensate for the ESC's dead zone and non-linearity, keeping fine corrections responsive around hover.
- **Integral anti-windup** — when a PID output saturates, the integral term decays by a 0.99 factor per cycle, preventing windup during limiting that would otherwise cause a violent snap when authority returns.

### 2.2 Attitude Estimation

- **Three switchable estimators at runtime** — Madgwick (gradient descent), Mahony (Betaflight-style PI-corrected DCM) and a complementary filter. Mahony is the default.
- **Adaptive accelerometer trust** — the deviation of `|a|` from 1 g is turned into a trust factor that dynamically scales Madgwick's β, the complementary filter's α, and Mahony's PI correction. During aggressive manoeuvres or free fall the filter degrades gracefully into pure gyro integration instead of being dragged off by a bogus gravity vector.
- **Automatic IMU identification** — MPU6050 (I²C, WHO_AM_I = 0x68/0x70) and MPU6500 (SPI, WHO_AM_I = 0x70) are detected at boot.
- **Software axis remapping** — four switches (`SwapXY / FlipX / FlipY / FlipZ`) adapt to any module mounting orientation without touching the driver.
- **Automatic level calibration** — while disarmed, 400 ms of detected stillness automatically resets the level reference from the accelerometer and syncs all three estimators.

### 2.3 Safety Protection

- **8 encoded fault reasons** (IMU failure / dt overrun / tilt limit / RC timeout / WS disconnect / KILL / manual disarm / motor hardware fault) all funnel into the single `disarmNow()` exit.
- **Four-condition arming interlock + 800 ms hold** — throttle low, sticks centred, airframe level, no fault. All four must hold continuously, which eliminates accidental arming.
- **Fault latch requires explicit reset** — after a fault you must close the ARM switch *and* return throttle to zero before arming becomes possible again, so a chattering fault cannot cause repeated re-arming.
- **Forced motor stop at low throttle** — at throttle ≤ 0.05 the motors are cut and all PIDs reset, clearing integral accumulated around the hover threshold.
- **Triple NaN guard** — three independent checks (IMU read failure, NaN/Inf attitude, NaN PID output). Any hit disarms immediately, blocking NaN from reaching the motor outputs.
- **Triple-gated motor test** — output capped at 20 %, duration clamped to 100–2000 ms, and the command is refused outright while armed, on motor fault, or with stick input present.

### 2.4 Human-Machine Interface

- **Self-hosted Wi-Fi hotspot** (default `YaoKun_Drone` / `12345678`, IP `192.168.4.1`) — works in the field with no network at all.
- **Zero-dependency single-page UI** — HTML/CSS/JS are all compiled into the firmware (1,651 lines) with no CDN or image assets, so it works offline.
- **Dual virtual sticks** — left stick throttle/yaw, right stick pitch/roll. Pointer Events cover both mouse and touch, and **releasing a stick re-centres the three axes while throttle holds**.
- **Fixed 20 Hz command cadence** — fully decoupled from render frame rate, so a stuttering phone or a throttled background tab cannot cause the firmware to time out the command link.
- **Tri-color LED semantics** — red+green steady = armed; synchronized breathing = disarmed idle; 120 ms fast blink = fault; blue fast blink = actually flying, steady = web page connected, slow blink = idle.
- **14 buzzer patterns** — power-on, ready, long beep on arming, double beep (gyro calibration), triple beep (level calibration), takeoff cue (1–10 repeats), fast fault alarm, three-level battery alarm, and a dedicated KILL tone, plus a global mute. **Full rhythm charts in Chapter 9.**
- **Three-level battery alarm** — 3.70 / 3.50 / 3.30 V thresholds, with LED and buzzer cadence escalating together. Alarm priority is higher than the flight-state LEDs.
- **12 color themes** (light / dark + 10 traditional Chinese colors); the selection is stored in `localStorage` and restored on the next visit.

### 2.5 Parameters & Maintainability

- **Multi-board conditional compilation** — switch PCB by changing one macro in `config.h`; pin definitions are fully isolated.
- **Every parameter tunable online** — 9 PID gains, 7 flight limits, filter mode and buzzer settings, all hot-updated from the web UI and persisted to NVS.
- **Double clamping** — values are clamped on write and clamped again after reading back from NVS, so stale values from older firmware cannot go out of range.
- **Five critical sections guard shared state** — RC commands, config/PID, KILL requests, motor tests and calibration requests, all race-free across the two FreeRTOS contexts.
- **Every command has an ack** — if a parameter did not take effect, or a command was rejected (e.g. calibration while armed), the reason is shown immediately in the UI.

## 3. System Architecture

The firmware is divided into five layers by responsibility. Layers only talk through the interfaces declared in `drone_types.h`; there is no cross-layer reaching. This lets "swap a PCB", "change an algorithm" and "rewrite the whole UI" happen independently.

![Five-layer architecture](Docs/fig/fig01_arch.svg)

*Fig. 3-1　Five-layer architecture: presentation / decision / perception / hardware driver / configuration & storage*

> 💡 **Why bother layering?** The project supports multiple PCB variants (4 implemented, 4 planned), and the only difference between them is the pin definitions in `config.h`. If hardware access were scattered through algorithm code, swapping boards would mean touching the whole project. With every `ledcWrite`, `Wire.begin` and `SPI.transfer` confined to `hardware.ino`, **the layers above have no idea which pin they run on** — that is what makes one codebase cover several boards.

### 3.1 Module Dependencies

![Module dependencies](Docs/fig/fig08_modules.svg)

*Fig. 3-2　Module dependencies: the header contract layer, six `.ino` implementation files, and the concurrency model across two FreeRTOS contexts*

| File | Lines | Responsibility |
|---|---|---|
| `YaoKunDrone.ino` | 598 | Entry point: `setup()` / `loop()`, the control pipeline `controlStep()`, Wi-Fi/Web start-up, voltage alarm and calibration helpers |
| `drone_types.h` | 312 | Shared contract: 5 structs, 4 enums, inline helpers, all cross-module `extern` declarations |
| `config.h` | 376 | Multi-board conditional compilation + every system constant (pins / PWM / PID limits / thresholds) |
| `hardware.ino` | 885 | Four hardware drivers plus the methods of Madgwick / Mahony / Mpu6050 / Mpu6500 / ImuDriver |
| `drone_imu.h` | 199 | Declarations for those five classes + `accelToRollPitchDeg` / `isImuStationary` helpers |
| `drone_pid.h` | 183 | Generic positional PID with anti-windup and output clamping (heavily commented as a tutorial) |
| `rc.ino` | 291 | Critical-section wrappers for RC commands, WebSocket event callbacks, 14 downlink command parsers |
| `control.ino` | 80 | Five PID instances, gain application, dirty-flag hot update |
| `failsafe.ino` | 113 | Safety core: arming conditions, state machine, disarm and kill — fully independent of control math |
| `storage.ino` | 145 | Load/save for the NVS namespace `"drone"`, with second-pass clamping |
| `telemetry.ino` | 114 | Telemetry JSON framing and rate gating, three enum-label translators |
| `drone_led.h` / `drone_buzzer.h` | 147 / 79 | LED mode enum and breathing algorithm / buzzer pattern enum and state-machine interface |
| `drone_web_ui.h` | 1651 | The complete single-page front end, fully embedded, no external assets |

> 💡 **Two Arduino conventions you must know:** ① `.ino` files get auto-generated prototypes and are force-included by the IDE, while `.h` files are not — hence all cross-module declarations live in `drone_types.h`, and each `.ino` only needs to include it. ② `drone_pid.h` uses `clampf()`, which is defined in `drone_types.h`, so that header must be included *after* the utility functions — the usual "dependencies first" rule.

### 3.2 Code Size Distribution

![Code size distribution](Docs/fig/fig12_stats.svg)

*Fig. 3-3　How the 5,173 lines split: one third is the web front end, nearly a quarter is hardware drivers*

## 4. Control Loop

`loop()` is deliberately minimal: refresh LEDs, advance the buzzer state machine, then check whether the control period has elapsed. If not, spin in `delayMicroseconds(50)` to yield CPU to the Wi-Fi tasks; if yes, compensate the time base with an accumulator so any lagging slot is caught up in one go, then enter `controlStep()`.

```bash
// Time base and compensation: a late period never causes permanent drift
if ((int32_t)(now_us - next_us) >= 0) {
    while ((int32_t)(now_us - next_us) >= 0) next_us += kLoopPeriodUs;
    const float dt = (now_us - last_us) * 1e-6f;
    last_us = now_us;
    controlStep(dt);
    // ... voltage alarm 200 ms, blue LED 50 ms, alarm LED override
} else {
    delayMicroseconds(50);
}
```

![Control pipeline](Docs/fig/fig02_flow.svg)

*Fig. 4-1　The `controlStep()` pipeline: KILL → motor test → dt validation → IMU read → attitude solve → tilt protection → RC timeout → arming state machine → PID mixing*

### 4.1 Priority Ordering: Why This Sequence

The order is not arbitrary. Each step exists to guarantee that "if something already went wrong earlier, nothing later should run":

| # | Stage | Why it sits here |
|---|---|---|
| **①** | KILL request | Highest priority. When the user hits emergency disarm, the motors must stop **immediately** even if they are spinning and the IMU has already failed — nothing downstream may block it. |
| **②** | Motor test mode | Once a test starts it must run its full duration; letting flight logic cut in mid-way makes the motor stutter. It therefore pre-empts the control path and returns as soon as it finishes. |
| **③** | dt validity | A bad dt means the scheduler was blocked for a long time, so any control output computed now is untrustworthy — stop and report why. |
| **④** | IMU read | No attitude data means no control. **Note it returns if the read fails** — it never reuses last cycle's attitude, because that would let the aircraft fly "from memory". |
| **⑤⑥** | Attitude NaN check<br>Tilt protection | Both are evaluated right after the solve and before the arming state machine. Practical reason: if the aircraft has already flipped or the attitude has diverged, entering the arming path is meaningless. |
| **⑦** | RC timeout | A stale command means the operator has lost control authority — a clear "must not keep flying" signal, evaluated before arming. |
| **⑧** | Arming state machine | Only after confirming "we should be flying" does the code enter PID and mixing. Telemetry is still sent while disarmed so the UI always shows state. |

> ✅ **An easily overlooked correct design:** every disarm on a safety path calls `sendTelemetry()` before returning. That means **even while the aircraft is faulted, the web UI still shows the reason in real time.** Many flight controllers go silent in protection mode, leaving the user to guess what happened.

### 4.2 Power-On Boot Sequence

![Boot sequence](Docs/fig/fig11_boot.svg)

*Fig. 4-2　`setup()` sequence: serial → motor LEDC → IMU auto-detect → gyro calibration → parameter load → disarm → hotspot start-up*

> ⛔ **Two waits you cannot skip:**<br><br>① **Gyro bias calibration takes about 1–2 s** (500 samples × 2 ms). The airframe must be absolutely still — if it is rotating, the bias is recorded as angular rate, and the aircraft powers up believing it is spinning, so the first throttle input throws it.<br><br>② **A failed IMU probe halts the system**: `begin()` returning false drops `setup()` into `while(true) delay(1000)`. This is deliberate — without attitude data the controller can never be safe, so it refuses to let the user believe "it can fly".

Motor LEDC binding failure is different: it only sets `FS_MOTOR_FAIL` and inhibits arming, **and the system keeps running**. The IMU, estimator and web UI are all fine, so the user can still open the page and read the log to diagnose. A hard halt would destroy that.

## 5. Control Algorithms

![Cascaded PID chain](Docs/fig/fig03_pid.svg)

*Fig. 5-1　Cascaded dual-loop chain: the outer loop emits a desired rate, the inner loop emits normalized torque, which is scaled and fed into X-frame mixing*

### 5.1 Why Two Loops

The fundamental problem with single-loop control: **angle error and angular-rate error have different units and need different gains.** Driving motor torque directly from "angle error × Kp" creates a dilemma:

- Want fast response → raise Kp → motors saturate → a tiny angle error commands full throttle with no headroom;
- Want smooth motion → lower Kp → sluggish response, and any slightly quicker input turns into large oscillation.

Cascading splits the two demands into loops with different units:

- **Outer loop (angle)** — input is "how many degrees off", output is "how fast should we rotate (°/s)". Its ±250 °/s clamp is a **built-in speed safety valve**: the airframe can chase the target attitude at no more than 250 °/s, which is inherently safe.
- **Inner loop (angular rate)** — input is "how many °/s off", output is normalized torque. It solves a tracking problem, so as long as the inner loop holds firmly, attitude transitions become controllable and linear.

> 💡 **This is the classic structure used by Betaflight, PX4 and friends** — not this project's invention. But most tutorials stop at the words "cascaded PID". Here the units, clamp values and tuning rationale of each loop are documented in the code comments so you can follow them line by line.

### 5.2 Why Yaw Uses a Single Loop

There is no outer angle loop for yaw. The aircraft **has no magnetometer and no absolute heading reference**, so yaw angle is a pure gyro integral that drifts without bound. Putting an outer loop on a drifting quantity means integrating an integral, which makes steady-state error worse. Yaw therefore uses a pure rate loop (`g_yaw_rate_pid`): get "how fast to rotate" right and ignore which heading you end up at. The output clamp is ±0.3, more conservative than ±0.5 for roll/pitch, because yaw only has authority over two motors. Since v1.1 the yaw loop carries a small integral (Ki=0.005) that automatically cancels the net torque of motor/prop asymmetry, eliminating the constant slow spin (see 5.5).

### 5.3 X-Frame Mixing Matrix

```bash
yaw_t = yaw_torque * kYawTorqueSign   // yaw sign adaptation factor (see below)
M0 front-left  FL = throttle + roll_torque + pitch_torque - yaw_t
M1 front-right FR = throttle - roll_torque + pitch_torque + yaw_t
M2 rear-right  RR = throttle - roll_torque - pitch_torque - yaw_t
M3 rear-left   RL = throttle + roll_torque - pitch_torque + yaw_t
```

Where the signs come from: roll uses differential thrust across diagonals (FL+RL up, FR+RR down rolls right), pitch uses front/rear differential, and yaw uses counter-rotation on each side. The sum is then clamped to `[0, 1]`.

**Why yaw torque is multiplied by a factor**: the mixer assumes the motor rotation layout "front-left = CCW, front-right = CW, rear-right = CCW, rear-left = CW" (diagonal pairs share rotation). The v1.1 as-built aircraft turned out to have **every motor reversed** (front-left GPIO16 = CW, front-right GPIO6 = CCW, rear-right GPIO5 = CW, rear-left GPIO15 = CCW — diagonals still matching). That layout is legal on its own, but it flips the yaw reaction-torque direction: without flipping the sign the yaw loop becomes **positive feedback** — the nose spins faster and faster after takeoff, and once the aircraft tilts the thrust vector starts circling, i.e. the observed "accelerating spin → spiral climb". The fix is to multiply by `kYawTorqueSign = -1` (`config.h`) after the PID output and before the mixer: at this single point, stick directions, transmitter channel directions and the attitude ball all stay untouched; roll/pitch torque depends only on motor **position**, which did not change, so they are unaffected.

> ⚠️ **Mixing signs must match the physical motor orientation, otherwise the yaw loop goes dead or into positive feedback — the symptom is a constant slow spin (legal diagonal layout, signs globally flipped) or an accelerating spiral climb (the illegal adjacent-same-rotation layout, which no software can fix).** On a first build, always verify each motor's rotation prop-off via the Test page (see Chapter 12 step 4) before deciding whether to swap motor wires or change `kYawTorqueSign`.

### 5.4 Low-Throttle Torque Scaling

```bash
const float torque_scale = clampf(cfg.torque_scale_min + throttle * cfg.torque_scale_slope, 0.0f, 1.0f);
// default = 0.20 + throttle * 2.0
roll_torque  = g_roll_rate_pid.update(...)  * torque_scale;
```

Why it is needed: brushless ESCs have a pronounced dead zone and non-linearity at low throttle. Hover sits around 30 % throttle — exactly the least linear region. Command a 0.1 torque differential there and the actual RPM difference may be only 0.02, so the aircraft barely responds and feels "numb". After scaling (scale ≈ 0.8 at 30 % throttle) the same PID output produces a noticeably larger real RPM difference, and fine corrections become responsive.

The cost is that at high throttle the scale exceeds 1 and is clamped to 1, i.e. no compensation — which is exactly what you want: at high RPM you do not need extra gain, and applying it would invite oscillation.

### 5.5 Default PID Values

| Controller | Default Kp / Ki / Kd | Output clamp | Physical meaning |
|---|---|---|---|
| Angle loop (shared roll/pitch) | 2.0 / 0.001 / 0 | ±250 °/s | Output is a desired angular rate in °/s |
| Rate loop (shared roll/pitch) | 0.025 / 0 / 0.0002 | ±0.5 | Output is normalized, dimensionless torque |
| Yaw rate loop | 0.030 / 0.005 / 0 | ±0.3 | More conservative: only two motors affected |

Note the outer loop's Ki = 0.001 is **very small**, deliberately. Attitude steady-state error is naturally compensated by gravity (a tilted body produces a restoring torque), and a large integral would wind up during sustained tilt and overshoot badly on exit. The roll/pitch rate loops use no integral at all (Ki = 0) because gyro bias is already removed during calibration.

**The yaw loop's Ki = 0.005 is new in v1.1** (previously 0). Yaw is a single-layer rate loop with no outer loop to fall back on, and the combined torque of four motors and props can **never be perfectly symmetric** — that residual needs a constant counter-torque. A pure P controller can only balance a steady disturbance by leaving a residual rate: the aircraft rotates, the error appears, and the P term holds an opposing torque — i.e. a **constant slow spin** (ground effect near the floor amplifies the asymmetry). With the small integral, the I term builds the required counter-torque within about a second and the spin disappears; the PID class's anti-windup (integral ×0.99 decay while saturated) prevents windup. ⚠️ Upgrading from v1.0 with saved parameters: set yaw Ki to 0.005 manually in the web UI (or tap "Restore defaults"); a full-chip flash erases NVS so the new default applies automatically.

## 6. Attitude Estimation

![Attitude estimation flow](Docs/fig/fig04_ahrs.svg)

*Fig. 6-1　Estimation chain: IMU sample → axis remap → trust computation → selected estimator → Euler angles → level offset subtraction*

### 6.1 Accelerometer Trust: The Sanity Switch for the Whole Estimator

This is the most interesting design decision in the attitude pipeline. The accelerometer measures gravity direction and the gyro measures angular rate; they are complementary — but **the accelerometer is only trustworthy when there is no additional acceleration**. During a climb it reads "gravity + thrust acceleration", so its direction no longer represents the horizon. Correcting attitude with it then injects bogus corrections, and the estimate drifts further off.

```bash
const float acc_norm = sqrtf(ax*ax + ay*ay + az*az);
const float dev = fabsf(acc_norm - 1.0f);        // deviation from 1 g
if (kAccTrustLo > kAccTrustHi)
    acc_trust = (kAccTrustLo - dev) / (kAccTrustLo - kAccTrustHi);   // linear map to 0..1
acc_trust = clampf(acc_trust, 0.0f, 1.0f);
```

| Parameter | Value | Meaning |
|---|---|---|
| `kAccTrustHi` | 0.08 | Deviation below 0.08 g is fully trusted, trust = 1 |
| `kAccTrustLo` | 0.35 | Deviation above 0.35 g is fully distrusted, trust = 0 (pure gyro integration) |

The trust value then feeds three places: Madgwick's β interpolation (`β = 0.02 + trust × 0.12`), the complementary filter's α reduction, and Mahony's PI correction term. All three estimators therefore **share one judgement of "whom to believe"** and behave consistently.

### 6.2 Comparing the Three Estimators

| Estimator | Status | Key idea | Best for |
|---|---|---|---|
| **Mahony** | **Default** | Cross product between estimated and measured gravity gives error `e`; correct the rate with `g' = g + Kp·e + ∫Ki·e dt`, then integrate the quaternion. Integral clamped to ±0.35 rad/s and frozen above 180 °/s rotation to prevent divergence. | The default. Moderate cost, few parameters, insensitive to bias, same lineage as Betaflight, plenty of reference material. |
| Madgwick | Optional | Gradient descent: formulate gravity alignment as an objective and step down the gradient with β, adaptively interpolated 0.02–0.14. Falls back to pure gyro when the acceleration magnitude is too small. | When you want to feel the trust factor yourself; for readers learning gradient descent. |
| Complementary | Optional | The simplest: gyro integration blended with accelerometer `atan2` angles via a first-order α = 0.98. α rises with trust (trusting the gyro more). | Best for teaching — the most intuitive formula, and a control group for validating the other two. |

> 💡 **How is attitude continuity preserved when switching?** Swap the estimator naively and the new one starts from an identity quaternion (level) while the aircraft is already tilted tens of degrees — attitude would "snap back to zero" and the control loop would slam full torque. So the code first reads **the old estimator's current Euler angles** and injects them via `setEulerDeg()`; the complementary filter takes over its internal state directly. All three are aligned before continuing. An easily missed but critical detail.

### 6.3 Two Paths to Level Calibration

| Method | Trigger | Behaviour |
|---|---|---|
| **Manual** | Button on the Settings page (disarmed only, sends `CAL,LEVEL`) | Reads the current accelerometer-derived level angles into `g_level_roll_offset_deg` / `g_level_pitch_offset_deg`, saves to NVS immediately, confirms with a triple beep |
| **Automatic** | Disarmed + 400 ms of stillness (accel deviation < 0.10 g and all axes < 1.5 °/s) | Overwrites the estimated attitude with the accelerometer reading and resets the internal state of all three estimators |

The reported attitude is always "estimate − offset", so the offset is stored once and applies forever. A yaw offset variable exists (`g_level_yaw_offset_deg`) but has no UI entry point and stays 0.

> ⛔ **After changing the IMU module you must redo level calibration.** Mounting orientation and bias differ per module, and the offset is bound to "this board + this module". Without clearing it the attitude ball stays tilted and automatic calibration keeps resetting to a wrong reference.

## 7. Safety Mechanisms

![Failsafe architecture](Docs/fig/fig06_failsafe.svg)

*Fig. 7-1　All eight fault sources converge on the single `disarmNow()` exit, plus three extra protections that never become enum values*

### 7.1 The Single Disarm Exit

Whatever path detects a fault, it ends in the same function, `disarmNow(reason)`, which does seven things in a fixed order:

1. Clear the armed flag `g_armed = false`
2. Write 0 to all four motor PWM channels
3. Reset all five PID instances (clear integral and derivative history)
4. Set the arming-inhibit flag `g_arm_inhibit = true`
5. Record the reason in `g_last_fs` and write a status message
6. Beep (using the dedicated tone for KILL, since `BUZ_KILL` already played)
7. Stop any looping alarm tone

> 💡 **Why insist on a single exit?** If every fault branch wrote its own motor-off logic, sooner or later one would forget to clear the integrals or stop the alarm. The symptom would be "motors stopped but the buzzer is still screaming" or "the aircraft lurches the moment it re-arms" — bugs that are extremely hard to trace. With one function, adding a fault type is a single enum value and zero duplicated logic.

> 📌 **Since v1.1 the chip's brownout reset is disabled** (first statement of `setup()` clears `RTC_CNTL_BROWN_OUT_ENA`, same approach as the Flix firmware). Reason: with a 1S Li-ion behind an LDO the dropout margin is thin, and the surge of four motors at full load can pull the 3.3 V rail below the chip's brownout threshold (~2.43 V), causing a hard chip reset — observed as "reboot every time throttle approaches liftoff on the ground, but fine when held in the hand" (critical throttle = peak current). After disabling, the software `FS_` fault codes are unaffected, but **on a genuine undervoltage the chip no longer self-protects by resetting**; only the three-stage buzzer voltage alarm remains (<3.70 V slow / <3.50 V fast / <3.30 V urgent), so mind the voltage and land in time. Real fixes: thicker power wires, an electrolytic cap on the supply, or a dedicated BEC for the flight controller.

### 7.2 The Arming State Machine

![Arming state machine](Docs/fig/fig05_arm.svg)

*Fig. 7-2　Three-state arming machine: DISARMED → ARMING → ARMED, with the four-condition interlock and fault inhibit*

#### The Four Arming Conditions (all required)

| Condition | Requirement | What it prevents |
|---|---|---|
| ① Throttle | ≤ 0.05 (`kArmThrottleMax`) | Arming with throttle applied, which spins the motors instantly |
| ② Sticks | \|roll / pitch / yaw\| ≤ 0.15 (`kStickCenterMax`) | Arming with a command held, which yaws the aircraft at takeoff |
| ③ Attitude | \|roll\| and \|pitch\| ≤ 45° | Arming while inverted or heavily tilted |
| ④ State | No motor hardware fault, not in fault inhibit | Repeated arming attempts after a motor fault |

All four must hold **continuously for 800 ms** (`kArmHoldMs`) before arming actually happens. Breaking any one resets the timer. This targets the most common consumer-drone accident: brushing a switch and taking off.

### 7.3 Recovery After a Fault

> ⚠️ **Note: a fault does not automatically restore the ability to arm.**<br><br>You must first **close the ARM switch** (`arm_request = false`) **and return throttle to zero**; only then is the inhibit flag cleared. Requiring an explicit reset is intentional: right after a fault the airframe and motors are rarely settled, and making the operator perform a deliberate "I have confirmed it is safe" action is far safer than the system deciding on its own to re-enable arming.

### 7.4 Three Protections Outside the Fault Enum

| Protection | Rule |
|---|---|
| **Forced stop at low throttle** | At `throttle ≤ 0.05`, write 0 and reset every PID. The hover threshold is where integral accumulates fastest, so it must be cleared. |
| **Triple-gated motor test** | Output ≤ 20 %; duration clamped to 100–2000 ms with auto-stop; commands are refused with an ack while armed, on motor fault, or with stick input. |
| **Double parameter clamping** | Clamp on write, clamp again after reading from NVS, so stale values from older firmware cannot go out of range. |

## 8. Communication & Web Remote

![Communication timeline](Docs/fig/fig07_comm.svg)

*Fig. 8-1　HTTP page fetch → WebSocket handshake → config push → 20 Hz bidirectional loop → command channels*

### 8.1 The Link

On boot the controller starts SoftAP and registers three routes: `/` returns the embedded single-page app, `/health` returns `OK` (for liveness probing), and `/ws` is the WebSocket endpoint. The browser loads `http://192.168.4.1`, immediately opens a WebSocket, and the firmware pushes the complete configuration on connect. From then on it is a 20 Hz bidirectional loop.

> 💡 **Why WebSocket text frames instead of binary MAVLink?** The whole point of this project is "control it from a browser" — the operator is a person holding a phone, not a ground station. Text JSON buys three practical things: ① serial tools, the browser console and Python scripts can all send and receive directly, so the debugging barrier is zero; ② the front end just calls `JSON.parse`, with no MAVLink library or message registry; ③ no endianness or field-alignment issues, and a human can read it. The cost is slightly more bandwidth (~200 bytes per frame × 20 Hz ≈ 4 KB/s), which Wi-Fi does not even notice.

### 8.2 Uplink: Control Commands (20 Hz)

```bash
C,throttle,roll,pitch,yaw,arm_request
e.g. C,0.320,-0.150,0.080,-0.200,1
     throttle  roll   pitch  yaw    arm request
```

The front end sends on a fixed `setInterval(…, 20)` cadence, independent of how often the user moves. This guarantees **the command frame keeps refreshing even when the sticks are still** — the firmware declares a link loss on `cmd_age > cmd_timeout` (300 ms default). If it only sent on movement, a stationary aircraft would be disarmed 300 ms later.

#### Other Uplink Commands

| Command | Purpose |
|---|---|
| `K` | Emergency stop, maps to `FS_KILL`, highest priority in firmware |
| `GET` | Pull the full configuration (page refresh or "Fetch parameters") |
| `SAVE` | Persist current in-memory parameters to NVS ("Save parameters" in the UI) |
| `PID,ANGLE\|RATE\|YAW,kp,ki,kd` | Hot-update one PID group's gains; sets a dirty flag, applied and PID reset on the next control cycle |
| `CFG,KEY,VALUE` | Change one of the 7 flight limits or switch filter mode; all clamped to kMin/kMax |
| `CAL,LEVEL\|GYRO` | Request level calibration / gyro calibration, disarmed only |
| `MTEST,idx,thr,ms` / `MTEST,STOP` | Start / stop a single-motor test |
| `BUZZER_MUTE,0\|1` | Global buzzer mute (written to NVS immediately) |
| `TAKEOFF_REPEAT,1~10` | Number of takeoff cue repetitions |

### 8.3 Downlink: Telemetry (20 Hz)

```bash
{"type":"tel","t":1234567,"armed":1,"roll":-3.21,"pitch":1.05,"yaw":45.30,
 "dt_ms":2.01,"loop_hz":497.5,"cmd_age":42,"m0":0.310,"m1":0.298,
 "m2":0.295,"m3":0.305,"vbatt":3.842,"fs":0,"cal":"IDLE","msg":"ARMED"}
```

| Field | Meaning |
|---|---|
| `armed` | Whether the aircraft is armed |
| `roll/pitch/yaw` | Attitude in degrees, level offset already subtracted |
| `dt_ms` / `loop_hz` | Actual control period and loop frequency — the key indicators of whether the MCU is running out of headroom |
| `cmd_age` | Age of the last RC command in ms; approaching 300 means stop and investigate |
| `m0~m3` | Actual motor outputs (0–1); use them to verify mixing and detect saturation |
| `vbatt` | Battery voltage; `null` rather than 0 when the read fails |
| `fs` | Fault reason code, see 7.1 |
| `cal` / `msg` | Calibration state and a human-readable status message |

#### Ack Replies

Every control command answers with `{"type":"ack","op":"…","tag":"…","ok":0|1,"msg":"…"}`, where `msg` gives the specific reason: `ARMED` (calibration requested while armed), `MOTOR_FAIL`, `RC_ACTIVE` (motor test requested with stick input), `BAD_IDX`, `QUEUED`, `RUN`, `MUTED`, and so on. The front end renders these as localized messages for 2 seconds, which makes "I changed a parameter and nothing happened" diagnosable.

### 8.4 The Web UI

![Web UI structure](Docs/fig/fig10_webui.svg)

*Fig. 8-2　Four views: Remote (dual sticks + telemetry), Settings (attitude ball + tuning), Test (single motor), Themes (12 color schemes)*

> 💡 **Zero external dependencies is not laziness, it is a requirement.** All 1,651 lines are compiled into the firmware with no CDN, font file or image. Flying in the field there is no internet, and under RF interference a CDN request can hang for minutes — exactly when the aircraft is most likely to have a problem. So the deliberate choice is "one HTML file is the whole app", usable the moment the page arrives.

| View | Contents |
|---|---|
| **Remote** | Left stick (vertical throttle / horizontal yaw), right stick (vertical pitch / horizontal roll), ARM switch, emergency disarm button, throttle and link state, status message, 8 telemetry readouts |
| **Settings** | Canvas attitude ball (with horizon), three-axis readouts, level and gyro calibration buttons with status, buzzer mute and takeoff repeat count, filter dropdown, fetch/save parameters, 9 PID inputs, 7 flight limits |
| **Test** | Motor index selection, throttle and duration, single-motor start/stop |
| **Themes** | A 12-scheme grid (light / dark + 海棠 / 朱红 / 藕荷 / 湖蓝 / 晴蓝 / 藏青 / 葱绿 / 甘青 / 翡翠 / 松花), click to re-skin, stored in `localStorage` |

#### Why the Two Sticks Behave Differently

Releasing the right stick (pitch/roll) immediately zeroes all three axes, returning to "attitude hold". Releasing the left stick does **not** zero the throttle — if it did, the aircraft would drop instantly. Making the two sticks behave differently keeps the meaning of "letting go" unambiguous: attitude re-levels, altitude decays, and the pilot knows exactly what to expect.

## 9. Buzzer & LED System

This flight controller has no screen, so **the buzzer and the tri-color LED are its instrument panel**. The 14 tones and the three-color LED language form a complete feedback vocabulary: without opening the web page or watching a serial console, listening and looking is enough to know what state the aircraft is in and what just went wrong. The implementation principle throughout is **strictly non-blocking** — every sound and light effect is a `millis()`-timestamp state machine polled from the main loop (`buzzerUpdate()` / `ledUpdate()`), with no `delay()` anywhere, so the 500 Hz control loop is never stalled by a beep.

### 9.1 The Full Sound-and-Light Timeline

Start with the complete lifecycle from power-up to landing — every phase tells its story through sound and light together:

![Sound and light timeline](Docs/fig/fig15_flow_av.svg)

*Fig. 9-1　The full sound-and-light timeline: 8 phases combining tones, red/green LEDs and the blue LED, plus fault branches that can interrupt at any point*

> 💡 **Two feedback channels, two separate jobs:** the red/green LEDs express **flight-controller core state** (boot / idle / armed / fault), while the blue LED expresses **link state** (is a web page connected, is it actually flying). So when a light is blinking fast you never have to guess whether it is a fault or flight — the color tells you.

### 9.2 Buzzer: Rhythm Charts for 14 Patterns

The 14 patterns fall into five groups by purpose: status announcements (single), calibration confirmation (multi-beep), takeoff (repeated), waiting prompts (looping), alarms (looping). Every rhythm below matches the real state-machine parameters in `hardware.ino`:

![Buzzer rhythm charts](Docs/fig/fig13_buzzer.svg)

*Fig. 9-2　Rhythm charts for the 14 patterns: solid blocks are "buzzer on", the x-axis is milliseconds; the right column is the trigger scenario*

| Pattern (enum) | Rhythm | Trigger and notes |
|---|---|---|
| `BUZ_POWER_ON` | single, 200 ms | Sounds right after `setup()` finishes hardware init — "power-on self-test passed". |
| `BUZ_READY` | single, 100 ms | After the Wi-Fi hotspot and web server are up, in sync with the blue LED going to slow blink. |
| `BUZ_SHORT_BEEP` | single, 100 ms | Confirms disarming when ARM is closed — you hear it, so the motors are really cut. |
| `BUZ_LONG_BEEP` | single, 400 ms | Plays when the ARM switch has been held ~0.8 s, **from this moment the props are dangerous**. |
| `BUZ_MOTOR_TEST` | single, 50 ms | Warning before each single-motor test, giving you time to get your hands clear. |
| `BUZ_DOUBLE_BEEP` | 100 / off 100 / 100 | Gyro calibration succeeded from the web UI. |
| `BUZ_TRIPLE_BEEP` | 100 ×3, 100 gap | Level calibration succeeded. The two calibrations differ only by beep count — distinguishable with your eyes closed. |
| `BUZ_TAKEOFF` | double-beep × 3 rounds (1–10 configurable) | Fires once on **the first throttle-up after arming** (`g_takeoff_sounded` prevents repeats). |
| `BUZ_QUICK_TICK` | period 200 / on 50, looping | Urges you to finish arming, synced with the red/green slow blink; stops when you let go. |
| `BUZ_FAST_ALARM` | period 160 / on 80, looping | Any of six faults (IMU failure / RC timeout / invalid attitude / excessive tilt / PID NaN / loop overrun). |
| `BUZ_VOLT_ALERT_SLOW` | period 1000 / on 200 | Cell voltage < 3.70 V, level Ⅰ. |
| `BUZ_VOLT_ALERT_MED` | period 500 / on 100 | < 3.50 V, level Ⅱ. |
| `BUZ_VOLT_ALERT_FAST` | period 200 / on 50 | < 3.30 V, level Ⅲ, near over-discharge. |
| `BUZ_KILL` | 100 / off 50 / 100 | Dedicated tone when a `kill` command cuts throttle, **and the only pattern that cannot be overridden** by another sound in `buzzerPlay()`. |

> ⚠️ **A detail worth knowing:** the takeoff pattern is commented as "repeated double beeps", but walking the sub-state machine shows the `phase==0` branch only turns the buzzer off without re-arming the first beep — audibly, round 1 is a full double beep (0 / 200 ms) while rounds 2 and 3 each sound only once (at 600 / 1000 ms). To make every round a proper double beep, change `buzzerSet(false)` in the `phase==0` branch to sound once more before entering `phase==1`. This is a **behaviour/comment mismatch** found while reading the code, recorded honestly.

### 9.3 The Tri-Color LED Language

![Tri-color LED language](Docs/fig/fig14_led.svg)

*Fig. 9-3　The LED language: 5 red/green modes, 3 blue states and 3 voltage-alarm overrides, as brightness-vs-time curves*

The red/green LEDs are driven by `g_led_mode` (LEDC channels 4/5, fully isolated from motor channels 0–3, so they never fight), and the blue LED by a separate piece of logic refreshed every 50 ms in the main loop:

| LED | State | Behaviour and meaning |
|---|---|---|
| **Red + green**<br>LEDC PWM<br>channels 4 / 5 | `LED_BOOT` booting | Alternating 220 ms blink — initializing, not yet idle. |
| **Red + green**<br>LEDC PWM<br>channels 4 / 5 | `LED_DISARMED` idle | Synchronized 1.4 s breathing (sine) — disarmed, **breathing = safe to approach**. |
| **Red + green**<br>LEDC PWM<br>channels 4 / 5 | `LED_ARMING` arming | Synchronized 300 ms slow blink + ticking — waiting for you to confirm. |
| **Red + green**<br>LEDC PWM<br>channels 4 / 5 | `LED_ARMED` armed | Both steady — motors are live, stay clear of the props. |
| **Red + green**<br>LEDC PWM<br>channels 4 / 5 | `LED_FAILSAFE` fault | Synchronized 120 ms fast blink + fast alarm — needs immediate attention. |
| **Blue**<br>plain GPIO<br>50 ms refresh | Slow blink (1 s) | Idle: no web page connected, not armed. |
| **Blue**<br>plain GPIO<br>50 ms refresh | Steady | WebSocket connected — someone has the page open. |
| **Blue**<br>plain GPIO<br>50 ms refresh | Fast blink (200 ms) | **Flying**: armed and throttle > 5 %. "Blue blinking = I am really flying." |
| **Voltage override**<br>highest priority | Level Ⅰ < 3.70 V | Blue slow blink — battery getting low, prepare to land. |
| **Voltage override**<br>highest priority | Level Ⅱ < 3.50 V | Green slow blink — noticeably sagging, land soon. |
| **Voltage override**<br>highest priority | Level Ⅲ < 3.30 V | Red + green + blue fast blink — near over-discharge, land now. |

> 💡 **Why an override layer instead of a new state?** `applyVoltageLed()` rewrites the three LEDs **after** the state LEDs have been refreshed, without touching `g_led_mode`. The alarm "borrows" the LEDs for a few seconds; when voltage recovers or the aircraft lands, the state language resumes unchanged. The two concerns stay fully decoupled — much cleaner than inserting an alarm state into each of the five `LedMode` values.

### 9.4 A Few Implementation Choices

- **Non-blocking state machines** — every pattern is a small "sub-state + deadline" machine advanced by `buzzerUpdate()`. The main loop never waits for a beep to finish; that is a hard requirement for 500 Hz real-time control.
- **Mute blocks sound, not state** — `buzzerSet()` checks the mute flag up front: the state machine keeps advancing while muted, it just does not drive the pin. Unmuting resumes naturally, with no "stuck state while muted".
- **Looping tones are managed separately** — `buzzerStopLoop()` stops only the 5 looping patterns (tick / alarm / three voltage levels) and **never cuts a one-shot beep that just started**. So a disarm beep is never "eaten" by an alarm that follows immediately.
- **Persistence** — mute and takeoff repeat count are written to NVS (`BUZZER_MUTE` / `TAKEOFF_REPEAT`) and restored after a reboot, so you do not re-set them every flight.

### 9.5 Adding Your Own Tone in Three Steps

1. **Register the enum** — add an entry to `BuzzerPattern` in `drone_types.h` (e.g. `BUZ_CAL_FAIL`).
2. **Write the rhythm** — a one-shot only needs a duration in the `buzzerPlay()` switch; complex rhythms (multi-beep / looping) get an `if (g_buzzer_pattern == ...)` branch in `buzzerUpdate()` advanced by `g_buzzer_substate`.
3. **Trigger it** — call `buzzerPlay(your_pattern)` at the relevant business point. Done.

> ✅ **Diagnose by ear:** one 200 ms beep = self-test passed · one 100 ms beep = ready / disarmed · two short = gyro calibrated · three short = level calibrated · long beep = armed (dangerous) · rapid beeps = fault · slow tick speeding up = voltage falling · two urgent beeps = emergency stop.

## 10. Hardware & Pinout

![Pinout and wiring](Docs/fig/fig09_pins.svg)

*Fig. 10-1　Board selection, the complete pin table for the default board, reserved expansion headers and voltage thresholds*

### 10.1 Boards and Switching (4 implemented)

All board differences live in the conditional compilation in `config.h`; switching means editing one `#define` at the top. That file also has a fallback: if none of the `PCB_YaoKun_*` macros is defined, the default board `PCB_YaoKun_LCKFB_ESP32S3` is selected automatically. One caveat: **defining a not-yet-implemented board macro (e.g. `PCB_YaoKun_ESP32Dev`) does not raise an error** — the fallback quietly pulls it back to the default board, so it compiles with default pins and the trailing `#error` branch is effectively unreachable. Only use one of the four implemented macros below.

| Board macro | Hardware | Motor pins |
|---|---|---|
| `PCB_YaoKun_LCKFB_ESP32S3`<br>★ default | YaoKun K1.0, LCKFB ESP32-S3 R8N8 full-function board | 16 / 6 / 5 / 15 |
| `PCB_YaoKun_S3mini` | YaoKun S1.0C, ESP32-S3 Supermini (open-sourced) | 8 / 4 / 3 / 9 |
| `PCB_YaoKun_S3mini_B` | YaoKun S1.0B, internal test build | 10 / 4 / 3 / 11 |
| `PCB_YaoKun_S3mini_A` | YaoKun S1.0A, first internal test build | 8 / 4 / 1 / 9 |
| `PCB_YaoKun_ESP32Dev` | Planned: YaoKun D1.0, for 30-pin ESP32 dev boards |  |

> ⚠️ **The S3mini series is pin-starved**, so the red/green LEDs may not be populated on the actual PCB (the pins are still defined, there is just no LED there). S1.0A/B are internal test revisions whose pins match those specific units — do not copy them blindly onto your own board.

### 10.2 IMU Axis Mapping

Different IMU modules are mounted in different orientations, so four software switches remap in **software**, with no driver change:

| Module | SwapXY | FlipX | FlipY | FlipZ |
|---|---|---|---|---|
| Genuine MPU6050 (4×4 mm) | true | false | false | false |
| Domestic MPU6050 (3×3 mm) | true | false | false | false |
| MPU6500 module | true | **true** | **true** | false |

In the `SwapXY` branch both axis rates are also negated — not a typo: swapping the coordinate system flips handedness, so both must invert to preserve a right-handed frame.

> 💡 **Verification (also written in `config.h`):** rebuild and flash, open the page → Settings → watch the attitude ball. Tilt the aircraft **about 30° to the left**: Roll should read **negative** with the horizon low-left/high-right. **Nose up**: the ball should show sky (pitch positive). **Nose down**: the ball should show ground (pitch negative). Two or three combinations are usually enough to find the right one.

### 10.3 Battery Voltage Sensing

GPIO2 is ADC1_CH2, 12-bit with 11 dB attenuation (measurable to about 3.9 V), combined with a 100K + 100K divider on the PCB (ratio 2.0) to cover a 1S LiPo's 4.2 V full-charge range. Alarms have three levels (the normal state raises none):

| Level | Threshold (1S) | LED | Buzzer |
|---|---|---|---|
| Normal | ≥ 3.70 V | Normal state display | none |
| Low | < 3.70 V | Blue, 1 s slow blink | 1 s period |
| Warning | < 3.50 V | Green, 1 s slow blink | 0.5 s period |
| Critical | < 3.30 V | Red+green, 0.5 s fast blink | 0.2 s period |

The voltage alarm has **higher LED priority than flight state** — low battery is more urgent than "am I armed", and the pilot must see it first.

### 10.4 Reserved Expansion Pins

| Bus | Pins (default board) | Planned use |
|---|---|---|
| SPI | MOSI 18 / CLK 38 / MISO 21 / CS 17 | PMW3901 optical flow; the MPU6500 also uses this bus |
| Second I²C | SCL 4 / SDA 3 | VL53L1X laser rangefinder |
| UART | TX 12 / RX 11 | ELRS receiver (a physical transmitter later) |
| SBUS | GPIO 1 | SBUS receiver input (UART hardware inversion) |

> ⛔ **These four pin groups are only *defined* in the current code — no driver is implemented.** Apart from web commands, the firmware reads nothing from them. This version is controlled from the browser only.

## 11. Build & Flash

### 11.1 Toolchain

| Item | Requirement |
|---|---|
| IDE | **Arduino IDE 2.x** (recommended), or Arduino CLI / PlatformIO |
| Board package | Espressif Systems → **esp32** (≥ 2.0.14, must support ESP32-S3) |
| Board selection | ESP32S3 Dev Module (or the equivalent for your board) |
| Third-party libraries | `ESPAsyncWebServer`, `AsyncTCP` (see below) |
| USB driver | CH340 / CP2102 or whichever bridge your board uses |

> 💡 **About AsyncTCP:** it is a hard dependency of `ESPAsyncWebServer`. The package of that name in the ESP32 library manager targets ESP32 rather than ESP32-S3, so **compiling for S3 may fail**. The reliable route is to clone the `master` branch of `esphome/AsyncTCP` (which already has S3 support) into Arduino's `libraries` folder. If both the library-manager version and a manual copy exist, the manual one wins.

### 11.2 Installing the Libraries

```bash
# Option 1: Arduino IDE library manager
#   search ESPAsyncWebServer -> install
#   search AsyncTCP          -> if compilation fails, use option 2

# Option 2: manual install (recommended for ESP32-S3)
cd <Arduino IDE install dir>/libraries
git clone https://github.com/esphome/AsyncTCP.git
git clone https://github.com/mathieucarbou/ESPAsyncWebServer.git
# If ESPAsyncWebServer also fails, clone the esphome branch:
#   git clone -b esphome https://github.com/mathieucarbou/ESPAsyncWebServer.git
```

### 11.3 Build and Flash Steps

1. **Open the project**　Get the sources first: `git clone https://github.com/YaoKunS/ESP32-Drone.git` (or download the zip from the repo page). Then Arduino IDE → File → Open Project → select `YaoKun_Drone/YaoKun_Drone.ino`. The IDE pulls in every `.ino` / `.h` in the same folder — **do not copy a single file out**.
2. **Select the board**　Tools → Board → ESP32 Arduino → `ESP32S3 Dev Module`; Tools → Upload Speed → 921600 (drop to 115200 if unstable); Tools → Flash Size → `4MB (FS:1MB OTA:0.5MB)` or whatever matches your board.
3. **Confirm the board macro in `config.h`**　Open `config.h` and check that the default `#define PCB_YaoKun_LCKFB_ESP32S3` matches your PCB. If not, change that one line — **no other pin edits needed**.
4. **Connect and flash**　Connect USB → Tools → Port → select the new COM port → Upload. If it will not enter download mode, hold BOOT while clicking Upload and release when it starts.
5. **Open the serial monitor**　Baud rate **115200**. A healthy boot prints `Boot...` → `LEDC attach: 1 1 1 1` → `Detected MPU6050 (I2C)` → `Gyro calibration done` → `Ready. Open http://192.168.4.1/`.

> ⛔ **Two flashing pitfalls you must know:**<br><br>① **A full-chip merged flash wipes NVS.** If you use `esptool merge_bin` to combine bootloader + partitions + app into one 4 MB image and flash from offset 0, the NVS partition at `0x9000` is overwritten and you lose the Wi-Fi name, the IMU detection result and every tuned PID. During development, just use the IDE's Upload button (app partition only).<br><br>② **The board may stay in download mode after flashing.** `ESP-ROM … boot:0x0 DOWNLOAD waiting for download` means the chip is sitting in the bootloader. First connect with `--before no-reset` to confirm it is still reachable, then read back `0x10000` and check that the first byte is `0xE9` to confirm the image is intact. `run` / `--after hard-reset` may keep falling back into download mode (BOOT held low) — use `--after watchdog-reset` instead.

### 11.4 Build Options

```bash
Serial output: 115200 baud, for IMU detection results and LEDC binding status
Flash frequency: 80 MHz (default)
PSRAM: the ESP32-S3 R8N8 board has PSRAM on board; set it to match your hardware
Core count / frequency: defaults are fine; which core the control loop runs on does not affect correctness
```

For high-rate attitude debugging, set `kEnableSerialAttDebug` in `config.h` to `true` and it prints `ATT,roll,pitch,yaw` at 100 Hz. **Keep it `false` for real flight** — serial printing blocks and eats into the control period.

## 12. Usage

### 12.1 First Power-Up (follow the order exactly)

> ⛔ **Safety: remove the propellers for steps 1–4 of a first build.** Wrong motor order or mixing signs with props fitted means a crash. Only fit props after step 4 passes completely.

1. **Place it level and still, then power up**　Put the aircraft on a level table and **keep it absolutely still** for about 2 seconds. If it rotates during gyro bias calibration, the bias is recorded as angular rate and the aircraft powers up believing it is spinning — the first throttle input will throw it. The ready beep means calibration finished.
2. **Confirm the serial output**　The monitor should print the detected IMU type and `Ready. Open http://192.168.4.1/`. If it stops at `IMU init FAILED`, no sensor was detected — check the I²C wiring (SDA=9 / SCL=8) or whether the module is actually an MPU6500.
3. **Connect to the hotspot, open the page, run level calibration**　Phone Wi-Fi → `YaoKun_Drone` (password `48539491`) → browser → `http://192.168.4.1` → Settings → check the attitude ball is level (it should not be tilted) → tap "Level calibration" and listen for the triple beep. If the ball is tilted, switch to the complementary filter and compare readings to decide whether the IMU axis mapping is wrong (see 10.2).
4. **Check every motor's direction (props off)**　Test page → tap M0 / M1 / M2 / M3 in turn, each at 0.12 throttle for 600 ms. Verify position: M0 is **front-left**, M1 **front-right**, M2 **rear-right**, M3 **rear-left**; then verify rotation seen from above: **M0 front-left = clockwise, M1 front-right = counter-clockwise, M2 rear-right = clockwise, M3 rear-left = counter-clockwise** (diagonal pairs share rotation — this is the factory layout that `kYawTorqueSign=-1` adapts to, see 5.3). If a position is wrong, check the pin mapping in `config.h`; if a motor spins the wrong way, swap its two output wires to reverse it. **Note: the four motors must form a "diagonal pairs share rotation" layout — in the illegal adjacent-same-rotation layout the yaw control torque is identically zero and no software can fix it; the hardware must be changed first.**
5. **Fit the props and check their orientation**　Confirm the four motors alternate rotation direction (a prerequisite for X-frame mixing) before fitting props. Each motor's prop must produce thrust in the same direction — the most basic and most commonly botched step.
6. **Low-altitude test flight**　Back to the Remote page → raise the throttle slowly to 20–30 % until it lifts off → **immediately** flip the ARM switch and **hold it for 1 s** until the arming beep → trim throttle to hold altitude → gently push the right stick to check roll/pitch direction (pushing right should roll it right). If a direction is reversed, disarm at once, fix it and retry.

### 12.2 Pre-Flight Checklist

- Battery adequately charged (> 3.7 V), wiring secure, props undamaged and tight
- After power-up, wait for gyro calibration to finish (keep it still)
- The web page connects and the attitude ball is level without drifting
- For arming: throttle at zero, all sticks centred, airframe level
- No obstacles nearby and nobody standing near the takeoff point

### 12.3 Quick Reference

| I want to… | How |
|---|---|
| Arm | Throttle zero + sticks centred + level airframe → flip ARM → **hold 1 s** → the long beep means success |
| Disarm | Close the ARM switch, or tap "Emergency disarm" |
| Emergency stop | Tap "Emergency disarm" → sends `K` → motors cut immediately (highest priority) |
| Recover from a drift | Reduce throttle and let it land → correct attitude → re-arm. Do not fight it with large reverse stick inputs in the air |
| Tune PID | Settings → edit Kp/Ki/Kd → "Apply" → fly and feel → "Save parameters" to persist to NVS |
| Change the estimator | Settings → dropdown → apply (**must be disarmed**) → saved to NVS automatically |
| Restore factory parameters | Erase the NVS partition during flashing, then reflash |

## 13. PID Tuning Guide

### 13.1 Order Matters: Inner Loop First

> ⛔ **Never tune both loops at once.** With the outer loop moving you cannot judge the inner one, and you will not be able to tell whether "slow response" comes from a small outer Kp or an inner loop that cannot keep up. Stabilize the inner loop first, then the outer — the only correct order for cascaded control.

#### Step 1: Tune the Rate Loop (RATE group)

First drop the angle-loop Kp to **about 0.5** on the Settings page so the outer loop is weak and interferes less. Then:

- **Too slow** (RPM lags the stick) → raise Kp
- **Oscillation** (high-frequency airframe shake) → lower Kp, or raise Kd
- Reference range: Kp 0.015–0.040, Kd 0–0.0005

#### Step 2: Tune the Angle Loop (ANGLE group)

Only touch the outer loop once the inner one is stable:

- **Slow to level, feels soft** → raise angle Kp
- **Swings back and forth** → lower angle Kp
- **Stops with a constant tilt offset** → add a little Ki (0.0005–0.002), **never above 0.005**
- Reference range: Kp 1.5–3.0, Ki 0–0.002, Kd normally 0

#### Step 3: Yaw (YAW group)

Yaw usually needs no tuning; the default works. If rotation feels slightly delayed, raise Kp from 0.030 to 0.040. Yaw Ki ships at 0.005 (added in v1.1) to cancel the constant spin from motor/prop torque asymmetry: still spinning slowly with the sticks centered on the ground → try 0.008–0.010; nose oscillating left-right around center → the integral is too large, drop back to 0.003–0.005. Keep in mind Ki compensates the *steady* torque asymmetry and should not exceed 0.01 — yaw angle has no absolute reference (pure gyro integral), and a larger integral merely accumulates that drift into continuous rotation.

### 13.2 Tuning the Safety Limits

| Parameter | Default | Range | Advice |
|---|---|---|---|
| Max tilt angle | 30° | 5–80° | Beginners: keep 20–30°. This is the maximum permitted tilt and directly determines how far the aircraft can lean |
| Max yaw rate | 180 °/s | 30–360 | 120–180 is plenty for beginners; higher just makes you dizzy |
| Tilt protection angle | 80° | 20–85° | Lower is safer. 60–70° already protects well — do not raise it just to "lean harder" |
| RC timeout | 300 ms | 100–2000 | In weak-signal environments raise to 500–800; when signal is good do not go below 200 or a single dropped packet disarms you |
| Telemetry period | 50 ms | 20–500 | 20 Hz default is enough; 20 ms (50 Hz) noticeably loads the Wi-Fi link |
| Torque scaling | 0.20 / 2.0 | 0–1 / 0–4 | If hover feels numb, raise min or slope; if it oscillates, lower them |

> 💡 **Change one parameter at a time.** Test and record after each change; change three and you cannot tell which one did what. All parameters persist in NVS, and are lost on reboot unless you tap "Save parameters" — but **while tuning on the bench, save every time** to survive an accidental power loss.

### 13.3 Telemetry Fields Worth Watching

| Field | Normal range | What an anomaly means |
|---|---|---|
| `loop_hz` | 480–500 | Persistently below 450 means something is blocking (serial printing, network congestion) and control resolution has degraded |
| `dt_ms` | about 2.0 | Occasional jumps to 5–10 mean a one-off block (WS push, flash write). Sustained > 5 triggers dt protection |
| `cmd_age` | < 50 | Approaching 300 will disarm suddenly — the page is not connected or Wi-Fi is dropping packets |
| `m0~m3` | 0.25–0.45 at hover | Pinned near 0 or 1 means insufficient torque or a bad offset; large spread between motors means a balance or CG problem |
| `fs` | 0 | Non-zero: read the fault code, see 7.1 |

## 14. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Serial stops at `IMU init FAILED` | IMU not detected | Verify SDA=9 / SCL=8 wiring; confirm whether the module is MPU6050 (I²C) or MPU6500 (SPI); check for reversed I²C lines |
| Attitude ball tilted / Roll reading wrong | Wrong IMU axis mapping, or level calibration not done | Run level calibration first; if that fails, cycle `SwapXY/FlipX/FlipY` combinations per 10.2 and reflash |
| Aircraft thinks it is tilted at power-up | It moved during gyro calibration | Place it level and still, power-cycle, and wait for the ready beep before touching it |
| Push right, aircraft goes left | Motors swapped or mixing signs wrong | Props off, check each motor index and position on the Test page; distinguish a **left/right** swap (mirrored) from a **rotation** swap (one motor reversed) |
| ARM does nothing / drops out immediately | Arming conditions not met | Throttle to minimum, all sticks centred, airframe level, hold the switch still for over 1 s |
| Disarms by itself about 1 s after arming | RC timeout | Watch `cmd_age`. Near 300 ms means the page link is unstable — confirm you are still on the hotspot, disable battery saver, move closer |
| Disarms mid-flight | Tilt limit / WS disconnect / RC timeout | Read `fs`: 4 = TILT_LIMIT (hit something or insufficient thrust), 2 = WS_DISCONNECT (phone out of range or screen off), 1 = CMD_TIMEOUT (link problem) |
| `192.168.4.1` will not open | Not on the hotspot / serial blocked | Confirm you joined `YaoKun_Drone` (not a same-named router SSID); **some phones warn "this Wi-Fi has no internet" and disconnect automatically — choose "keep connection" or dismiss the prompt** |
| Page intermittent / telemetry stutter | Channel interference from multiple units | Power only one aircraft (multiple units share one SSID and contend for the same channel); put the phone in airplane mode with data off; move closer or use 5 GHz |
| Parameter changes have no effect | "Apply" or "Save" not pressed | "Apply" = effective immediately (this session only); "Save parameters" = written to NVS (survives reboot). Both are needed |
| Calibration button does nothing | Aircraft is armed | Calibration is only allowed while disarmed — close the ARM switch first |
| Motors do not spin / only one spins | ESC unpowered, or throttle too low | Verify ESC BEC power; note this firmware **forces a stop at throttle ≤ 0.05**, so below 5 % throttle the motors never turn |
| Serial works but the page will not open | Old firmware / incomplete build | Confirm you rebuilt and reflashed; the web UI expects a recent firmware version |

> 💡 **The three-step diagnostic:** ① serial — do you see `Detected MPU6050/6500` and `LEDC attach: 1 1 1 1`; ② web telemetry — is the attitude sane, is `loop_hz` near 500, is `fs` zero; ③ props off, single-motor test. Nine out of ten problems land in one of three buckets: sensor, link, or mixing.

## 15. Design Trade-offs & Known Limits

### 15.1 What This Codebase Gets Right

| Design point | Value |
|---|---|
| Single disarm exit | Adding a fault type is one enum value; there is no "some branch forgot to cut the motors" hole |
| Telemetry on the safety path | The UI shows the reason in real time even during a fault — no guessing |
| Five critical sections | Shared state is race-free across two FreeRTOS contexts, and the sections are short enough not to disturb 500 Hz timing |
| Accelerometer trust | Turns "when the accelerometer must not be believed" into an explicit, computable quantity instead of a fixed weight and luck |
| Continuous filter switching | Avoids the attitude jump that slams full torque — a hidden bug common in similar projects |
| Double parameter clamping | Clamp on write plus clamp on read-back, so stale values from older firmware cannot go out of range |
| Fixed-cadence commands | UI stutter cannot affect the command rate — the foundation of remote-control stability |
| Very high comment density | Every constant has a source, every non-obvious place explains why; readable as a flight-control textbook |

### 15.2 Honest Limitations

| Limitation | Impact |
|---|---|
| **No position hold** | IMU only, no position or vertical reference. Release the sticks and it keeps its horizontal velocity; release throttle and it descends. Not a bug — a physical boundary set by the sensor suite. |
| **Yaw drifts slowly** | No magnetometer, so yaw is a pure gyro integral and heading drifts over a long flight. Stillness while disarmed keeps correcting it; in flight nothing does. |
| **Browser control only** | The SBUS / ELRS pins are defined but no driver exists; this version has no physical transmitter support. |
| No black box | After a fault there is no flight-data replay to find the cause. NVS stores parameters, not logs. |
| No ground-station protocol | Text JSON over WebSocket rather than MAVLink, so QGC and friends cannot connect directly. The price of "control it from a browser". |
| No OTA update | Reflashing requires USB. |
| Small yaw authority | Output clamped to ±0.3, more conservative than roll/pitch; fast yaw may feel weak. |
| No final modularization pass | `controlStep()` (~250 lines) still lives in the main file, and `hardware.ino` is 885 lines covering four driver classes. Readable today, but split them before adding much more. |

## 16. Roadmap

The PCB and pinout already leave room for the upgrades below, ordered by return on effort:

| Priority | Item | Notes |
|---|---|---|
| **High** | VL53L1X rangefinder<br>+ PMW3901 optical flow | Uses SPI + the second I²C bus to achieve "GPS-free hover" — the single most worthwhile upgrade, and the direct fix for the no-position-hold limitation. Optical flow measures horizontal displacement, the rangefinder measures height, and together they estimate horizontal velocity for position correction. |
| **High** | Barometric altitude hold | SPL06-01 or BMP280 on the shared IMU bus. Barometric height + Kalman filter + dual-loop PID closes the vertical axis, which is the only path to "let go of the sticks and it hovers". |
| Medium | SBUS receiver support | GPIO1 is reserved (UART hardware inversion). A physical transmitter removes the browser dependency and improves bandwidth and latency. Note SBUS runs at 100 Hz, far below the 20 Hz assumption behind the current timeout thresholds — adjust them accordingly. |
| Medium | QMC5883L magnetometer | Corrects yaw drift and upgrades yaw from a single rate loop to a cascaded loop. Beware magnetic interference (motor current, speaker magnets); hard- and soft-iron calibration are required. |
| Medium | Flight log / black box | Ring-buffer attitude, motor outputs and fault state into SPI flash, exportable over the web UI, so incidents can be analysed with data instead of guesswork. |
| Low | MAVLink protocol | Would bring QGC / Mission Planner compatibility, at the cost of a parser library and message registry that conflict with the current zero-dependency single-page design. A real trade-off. |
| Low | Code split | Extract `controlStep()` into its own module and split `hardware.ino` by driver type. 5,173 lines is still readable, but further growth will not be. |

> ✅ **If you only do one thing:** implement altitude hold (rangefinder or barometer + dual-loop PID). It changes the aircraft qualitatively — from "manual flying" to "you can let go of the sticks". It is the divide between "it flies" and "it is pleasant to fly".

## 17. Comparison with Other Open-Source Flight Controllers

The tables below compare YaoKun Drone with **Flix**, **ESP-FC** and **ESP-Drone**. **The original assessment tables were generated by DeepSeek and are provided for reference only**; objective figures about this project (line count, number of boards, loop rate) have been verified against the actual source — e.g. 5,173 lines including 1,651 lines of front end rather than ~2,500, and 4 implemented boards plus 4 planned rather than 8.

### 17.1 Core Architecture

| Dimension | YaoKun Drone | Flix | ESP-FC | ESP-Drone |
|---|---|---|---|---|
| Framework | Arduino (modular files) | Arduino (<2,000 lines) | Native ESP-IDF / Arduino | ESP-IDF + FreeRTOS |
| Control loop rate | 500 Hz | 1,000 Hz | 1–2 kHz | 500 Hz |
| OS | Bare-metal superloop | Bare metal | Bare metal | FreeRTOS |
| Protocol | WebSocket + HTTP | MAVLink (QGC) | Betaflight Configurator | CRTP + cfclient |
| Parameter configuration | Web page, direct | MAVLink parameter service | Betaflight Configurator / CLI | cfclient host tool |

### 17.2 Attitude Estimation & Sensor Fusion

| Dimension | YaoKun Drone | Flix | ESP-FC | ESP-Drone |
|---|---|---|---|---|
| Estimator | **Madgwick / Mahony / complementary** (3 options, hot-switchable) | Complementary filter | Quaternion + complementary | Crazyflie EKF |
| Filter types | Low-pass + adaptive accelerometer trust | LPF | LPF, dynamic notch, RPM filter | Standard |
| Gyro support | MPU6050/6500 (auto I²C/SPI detect) | MPU9250/6500/6050 | MPU6050/9250, ICM20602, BMI160 | MPU6050 |
| Adaptive algorithm | ✅ Dynamic accelerometer trust | ❌ | ✅ Dynamic notch | ❌ |
| Filter tuning | Web UI | CLI | Betaflight GUI | cfclient |

### 17.3 PID Control & Flight Performance

| Dimension | YaoKun Drone | Flix | ESP-FC | ESP-Drone |
|---|---|---|---|---|
| Control architecture | **Cascaded PID** (angle → rate) | 3-axis two-stage cascade | Betaflight-style PID | Crazyflie PID |
| Yaw control | Dedicated yaw rate PID | Integrated in 3-axis PID | Standard PID | Standard PID |
| Torque scaling / anti-windup | ✅ Low-throttle scaling + integral anti-windup | ❌ | Betaflight standard | Standard |
| Integral anti-windup | ✅ | ✅ | ✅ | ✅ |
| Flight modes | Self-level (altitude hold extensible) | ACRO / ANGLE | ACRO / ANGLE / AIRMODE | Self-level / alt-hold / position hold |

### 17.4 Learning Curve & Educational Value

| Dimension | YaoKun Drone | Flix | ESP-FC | ESP-Drone |
|---|---|---|---|---|
| Line count | **5,173** (incl. 1,651 front end), modular | <2,000, modular | Larger (mimics Betaflight) | Medium (FreeRTOS) |
| Learning difficulty | **★★☆☆☆** (Arduino-friendly) | ★★☆☆☆ | ★★★★☆ | ★★★★☆ |
| Chinese documentation | ✅ Complete (comments + tutorials) | Community translations | Limited | ✅ Official Chinese docs |
| Intended audience | Embedded beginners / hobbyists / teachers / student competitions | Minimalist fans | Advanced FPV pilots | Professional developers |
| Teaching value | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★★★☆ |

### 17.5 Feature Completeness

| Feature | YaoKun Drone | Flix | ESP-FC | ESP-Drone |
|---|---|---|---|---|
| Wi-Fi control | ✅ Web page (no app) | ✅ MAVLink + QGC | ✅ ESP-NOW, SBUS, etc. | ✅ App / gamepad |
| Phone app control | Via web page (nothing to install) | QGC ground station | ❌ | ✅ Dedicated app |
| Ground station | Custom web page | QGroundControl | Betaflight Configurator | cfclient |
| Black box | ❌ (SD card extensible) | ✅ Log analysis | ✅ | ✅ |
| Motor test | ✅ In the web UI | Unclear | ✅ Betaflight sliders | ✅ Host-tool debugging |
| Sensor calibration | ✅ Gyro / level | ✅ IMU calibration | ✅ Betaflight GUI | ✅ cfclient |
| Altitude / position hold | Extensible (I²C/SPI reserved) | Planned | ❌ | ✅ |

> 💡 **Summary:** the differentiator of YaoKun Drone is **"small but complete"** — commercial-grade flight-controller architecture while keeping Arduino's low barrier to entry. It is not only a drone you can fly, but a living textbook: every module demonstrates how an embedded system should be architected. It suits students learning embedded systems, developers looking for an open-source flight-controller project, teachers needing a teaching case, and researchers who want to validate an algorithm quickly.

## 18. Links & Support

| Channel | Address and notes |
|---|---|
| **GitHub repository** | [https://github.com/YaoKunS/ESP32-Drone](https://github.com/YaoKunS/ESP32-Drone)<br>All firmware source, web UI and photos. Clone with `git clone https://github.com/YaoKunS/ESP32-Drone.git`. GPL v3 — Stars, forks and issues are welcome. |
| **OSHWHub (open hardware)** | [https://oshwhub.com/lindahe](https://oshwhub.com/lindahe)<br>YaoKun's profile — the YaoKun Drone PCB series (Spark Plan 2026 open-source project). Schematics and PCB project files are free to download, and you can order fabrication directly from JLCPCB. |
| **Bilibili video tutorials** | [https://space.bilibili.com/3707011699050767](https://space.bilibili.com/3707011699050767)<br>The "耀坤智联" channel — build guides, flight demos and a full tutorial series. |
| **Taobao component kit** | [https://shop35432590.taobao.com/](https://shop35432590.taobao.com/)<br>The "耀坤智联" store — the component kit for this project, with basic parts from ¥9.9. |

> ✅ **Support the project:** if this has been useful, a Star on GitHub or a follow on Bilibili ("耀坤智联") is the most direct way to support the author.

---

<sub>YaoKunSmart · document compiled from the source in this repository · firmware 5,173 lines · [中文版](README.md)</sub>
