# Firmware Architecture (STM32)

<!--
This document details the low-level firmware running on the STM32 microcontroller:
its responsibilities, control loop design, communication protocol, and safety logic.
-->

## 1. Overview & Role

The STM32 is the **real-time spinal cord** of the robot. Its singular mandate is deterministic, low-latency motor control. It does not perform trajectory planning or inverse kinematics — those are the Raspberry Pi's job. The STM32 simply receives target joint states and executes them as fast and precisely as physics allows.

**Why a dedicated microcontroller instead of doing everything on the Pi?**
A Linux-based system (including the Raspberry Pi running ROS 2) is not a real-time OS. The scheduler can preempt any process for milliseconds at a time. A stepper motor requires pulse trains generated with microsecond-level precision — a 1ms jitter in pulse timing produces audible stuttering, vibration, and degraded position accuracy. The STM32 solves this with hardware timers that generate STEP pulses entirely in hardware, irrespective of software load.

---

## 2. Responsibilities

### 2a. Step Pulse Generation (STEP/DIR)

The STM32 uses **hardware timer peripherals** (TIM1, TIM2, etc.) in output-compare mode to generate STEP pulses. The pulse frequency directly maps to motor speed:

```
steps_per_second = (target_velocity_deg_per_sec / 1.8) * microstep_divisor
pulse_period_us  = 1,000,000 / steps_per_second
```

*Example*: At 1/16 microstepping, moving at 90°/s = 800 steps/rev × (90/360) × 16 = **3200 pulses/sec** → 312.5 µs per pulse.

The timer auto-reloads at this period. No `delay()` calls, no busy-waiting. The CPU is free to run the PID loop and handle UART simultaneously.

### 2b. Closed-Loop PID Control

Although the CL57T and CL42T drivers handle their own internal closed-loop correction at the driver level, the STM32 can still run a **supervisory position control loop** at the system level when the firmware needs behavior above the driver's built-in correction:

1. **Setpoint**: Target joint angle received from the Raspberry Pi (degrees).
2. **Process Variable**: Current actual joint angle. For J1–J4 this comes from the closed-loop driver itself (over RS-485 or the driver's internal step register). For J5–J6 it comes from the homed step counter, or from an AS5600 if one has been fitted as an optional add-on.
3. **Error**: `e(t) = setpoint - actual`
4. **PID Output**: Adjusts the step pulse frequency (velocity) fed to the driver.

```
u(t) = Kp * e(t) + Ki * ∫e(t)dt + Kd * de(t)/dt
```

The control loop runs at **≥ 1 kHz** (every 1ms) using a SysTick or dedicated timer interrupt. The PID gains (Kp, Ki, Kd) are **stored in flash** for each joint independently and can be tuned over the serial command interface without reflashing.

This makes the PID layer useful for:
- smoothing setpoint changes from ROS / MoveIt,
- compensating for load-dependent lag on joints that expose external position feedback,
- tuning homing and slow approach moves,
- and providing a consistent control abstraction if a joint is later converted from closed-loop driver feedback to external encoder feedback.

It is not meant to replace the driver's own internal correction loop on J1-J4.

> **For the open-loop TMC2209 wrist joints (J5/J6)**: The PID loop is effectively open-loop (encoder feedback not available by default). The STM32 simply commands position by counting pulses from the homed reference. If drift becomes a problem, AS5600 encoders can be added at the wrist.

### 2c. Encoder Reading (I2C — optional path, J5/J6 only)

> The CL57T / CL42T closed-loop kits on **J1–J4** report their position via the driver itself; the STM32 does not poll an external AS5600 for those joints. The I2C / mux code path only exists for **optional** wrist feedback on J5 / J6.

When an AS5600 encoder is connected to a wrist joint:

1. If both J5 and J6 are fitted with AS5600 boards, the STM32 first writes the target channel to the **TCA9548A** I2C multiplexer (address `0x70`). With only one wrist AS5600, the mux is omitted and the AS5600 is addressed directly.
2. STM32 reads the 12-bit angle register from the **AS5600** (address `0x36`).
3. Converts raw count (0–4095) to degrees: `angle_deg = (raw / 4096.0) * 360.0`
4. Feeds the result into the wrist joint's PID loop as the process variable.

When no AS5600 is fitted, the wrist joints run pure open-loop from the homed step counter and skip this code path entirely.

### 2d. Homing Sequence

On power-up or on receipt of a `HOME` command:

1. For each joint (in sequence, from base outward):
   - Enable motor at low velocity in the negative direction
   - Poll the **A3144 Hall effect sensor** pin for a falling edge (magnet trigger)
   - On trigger: immediately halt motor, zero the step counter, set `joint_angle = 0.0`
2. Move each joint to a known safe **rest pose** (e.g., straight up)
3. Send `HOMED_OK` packet to Raspberry Pi

### 2e. Safety Interlocks

The STM32 monitors the following and will **halt all motors instantly** on any fault:

| Fault Condition | Detection Method | Action |
|:---|:---|:---|
| Overcurrent / Driver Fault | CL57T/CL42T ALARM output pin (interrupt) | Disable all ENABLE lines, send `FAULT` packet to Pi |
| E-Stop Pressed | Physical AC power cut (hardware) | All motor power lost immediately |
| Joint Limit Exceeded | Software angular limit check in PID loop | Halt joint, clamp setpoint, send `LIMIT` packet |
| Serial Timeout | Watchdog timer — if no command received in ~500ms | Set all joints to hold (zero velocity) |
| I2C Bus Hang | Timeout in poll loop, reset TCA9548A | Log error, continue with last known position |

---

## 3. Communication Protocol (UART/USB — STM32 ↔ Pi)

A lightweight **binary packet protocol** is used for high-throughput, low-latency communication. ASCII protocols are avoided due to parsing overhead.

### Packet Format

```
[HEADER: 0xAA 0x55] [CMD: 1 byte] [PAYLOAD: N bytes] [CHECKSUM: 1 byte]
```

- **Header**: Fixed 2-byte sync word to detect packet boundaries after noise/corruption
- **CMD**: Command type (see table below)
- **PAYLOAD**: Variable based on CMD. Joint target packets carry 6× float16 angles = 12 bytes
- **CHECKSUM**: XOR of all bytes from CMD through end of PAYLOAD

### Command Table

| CMD Byte | Name | Direction | Payload | Description |
|:---:|:---|:---:|:---|:---|
| `0x01` | `SET_JOINTS` | Pi → STM32 | 6× float16 angles (12B) | Set target angles for all 6 joints |
| `0x02` | `GET_STATUS` | Pi → STM32 | None | Request current joint states |
| `0x03` | `STATUS_RESP` | STM32 → Pi | 6× float16 actual angles + 1B fault flags (13B) | Response to GET_STATUS |
| `0x04` | `HOME` | Pi → STM32 | None | Begin homing sequence |
| `0x05` | `HOMED_OK` | STM32 → Pi | None | Homing complete confirmation |
| `0x06` | `FAULT` | STM32 → Pi | 1B fault code | Driver fault or limit breach |
| `0x07` | `SET_PID` | Pi → STM32 | 1B joint index + 3× float32 (Kp, Ki, Kd) (13B) | Tune PID gains on the fly |
| `0x08` | `ESTOP` | Pi → STM32 | None | Software e-stop from Pi |

### Baud Rate

**115200 baud** is the default. At this rate, a full `SET_JOINTS` command (16 bytes total with header/checksum) takes ~1.4ms to transmit — well within the 10–20ms trajectory update interval of MoveIt 2. For finer trajectory streaming, the baud rate can be increased to **921600** if USB-serial is used instead of UART.

---

## 4. Firmware Module Structure (PlatformIO / C++)

```
firmware/
├── src/
│   ├── main.cpp              # Initialization, main loop
│   ├── stepper.cpp/.h        # Timer-based pulse generation per joint
│   ├── pid.cpp/.h            # Generic PID controller struct
│   ├── encoder.cpp/.h        # Optional AS5600 reads (J5/J6 only) - mux used only if both fitted
│   ├── homing.cpp/.h         # Homing state machine
│   ├── comms.cpp/.h          # UART packet parser and builder
│   └── safety.cpp/.h         # Fault detection and e-stop logic
├── include/
│   └── config.h              # Pin definitions, joint limits, PID defaults
└── platformio.ini            # STM32 target, build flags
```

All **joint-specific parameters** (pin numbers, angular limits, home directions, PID gains) are in `config.h` so they can be updated without touching control logic files.
