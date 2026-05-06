# Electrical Interface Map

This document defines the electrical interfaces for the robot arm at the contract level: signals, voltage domains, connector usage, and expected behavior. Exact MCU GPIO assignments can be filled in once the control-board layout is finalized.

## 1. System-Level Signals

| Signal | Direction | Voltage | Source | Sink | Notes |
|:---|:---:|:---:|:---|:---|:---|
| AC mains in | In | Mains | Wall outlet | IEC inlet | Routed through fuse and E-stop |
| 24V motor rail | Out | 24V DC | PSU | Motor drivers | Main actuator power |
| 5V logic rail | Out | 5V DC | Buck converter | STM32, logic ICs, sensors | Isolated from motor rail where practical |
| Pi 5V rail | Out | 5V DC | Dedicated buck converter | Raspberry Pi 4 | Kept separate from noisy motor logic rail; budget **≥3 A** peaks |
| Servo 5V rail | Out | 5V DC | Third buck from `24V_MOTOR` | MG996R gripper only | Isolated from `5V_LOGIC` / `5V_PI` for stall-current spikes |
| STEP signals | Out | **3.3V MCU** → **5V** | STM32 GPIO | 74HC245 → drivers | MCU pins are **3.3 V logic**; industrial drivers use **5 V** after the **74HC245** |
| DIR signals | Out | **3.3V MCU** → **5V** | STM32 GPIO | 74HC245 → drivers | Same level shifting as STEP |
| ENABLE signals | Out | **3.3V MCU** → **5V** | STM32 GPIO | 74HC245 → drivers | Same level shifting as STEP |
| ALARM / FAULT | In | Driver output | Drivers | STM32 | Active-low fault input assumed |
| Hall home inputs | In | 5V or open-collector | A3144 sensors | STM32 | One per joint |
| I2C SCL/SDA | Bi-dir | 3.3V or 5V via pullups | STM32 | Optional AS5600 (J5/J6 only) | Bus only populated if a wrist AS5600 is fitted; TCA9548A only if both are fitted |
| UART TX/RX | Bi-dir | 3.3V | Raspberry Pi / STM32 | STM32 / Raspberry Pi | High-level command channel |

## 2. Driver Logic Map

### 2a. J1-J4 Closed-Loop Driver Control

The CL57T / CL42T kits ship with a factory motor encoder that the **driver itself** reads. The STM32 only sees STEP/DIR/ENABLE/ALARM — there is **no external AS5600, no I2C bus, and no magnet** for these joints.

| Joint | Motor / Driver | Control Signals | Feedback Signals | Notes |
|:---|:---|:---|:---|:---|
| J1 | NEMA 23 / CL57T | STEP, DIR, ENABLE | ALARM + Hall home (A3144) | Closed-loop correction lives in the driver |
| J2 | NEMA 23 / CL57T | STEP, DIR, ENABLE | ALARM + Hall home (A3144) | Same interface as J1 |
| J3 | NEMA 17 / CL42T | STEP, DIR, ENABLE | ALARM + Hall home (A3144) | Same electrical contract, lower current |
| J4 | NEMA 17 / CL42T | STEP, DIR, ENABLE | ALARM + Hall home (A3144) | Same electrical contract, lower current |

### 2b. J5-J6 Open-Loop Driver Control

| Joint | Motor / Driver | Control Signals | Feedback Signals | Notes |
|:---|:---|:---|:---|:---|
| J5 | NEMA 14 / TMC2209 | STEP, DIR, ENABLE | Hall home (A3144); AS5600 optional Phase 2 only | Quiet wrist joint, open-loop by default |
| J6 | NEMA 14 / TMC2209 | STEP, DIR, ENABLE | Hall home (A3144); AS5600 optional Phase 2 only | Quiet tool-roll joint, open-loop by default |

> All six joints carry an A3144 Hall sensor for boot-time homing. Only J5 / J6 are even *candidates* for an AS5600, and only if open-loop drift turns out to matter in practice.

## 3. Connector Contract

### 3a. Driver Control Header

A common 6-pin control header can be used for each motor channel:

| Pin | Signal | Function |
|:---:|:---|:---|
| 1 | STEP | Step pulse from STM32 |
| 2 | DIR | Direction control from STM32 |
| 3 | ENABLE | Driver enable line from STM32 |
| 4 | ALARM | Fault output back to STM32 |
| 5 | GND | Shared reference |
| 6 | +5V_REF | Logic reference or pullup source |

### 3b. Sensor Header

A common 6-pin sensor header is wired the same on every joint, even when the I2C lines are unused, so a single cable BOM and connector applies arm-wide. **SCL / SDA are populated only on J5 and J6, and only if an AS5600 is actually fitted.** On J1–J4 those two pins are present in the header but left disconnected at both ends.

| Pin | Signal | Function | Populated on |
|:---:|:---|:---|:---|
| 1 | VCC | Sensor supply (P5V_LOGIC) | J1–J6 |
| 2 | GND | Sensor ground | J1–J6 |
| 3 | HOME | A3144 Hall output (boot homing) | J1–J6 |
| 4 | SCL | I2C clock — AS5600 only | J5/J6 if AS5600 fitted |
| 5 | SDA | I2C data — AS5600 only | J5/J6 if AS5600 fitted |
| 6 | SHIELD | Cable shield / drain | J1–J6 |

## 4. Signal Rules

- STEP, DIR, and ENABLE should be treated as deterministic digital outputs, not analog signals.
- ALARM and HOME inputs should default to a safe state if the line is unplugged.
- I2C pullups should live on the control side of the harness, not inside a random sensor module.
- Motor power and logic power should not share a connector unless the connector is explicitly rated for the combined load.
- Each joint should be wired so that a missing encoder or sensor cannot energize the motor unexpectedly.

## 5. Frozen electrical decisions (for schematic + harness)

These choices unblock KiCad net naming and firmware pin mapping reviews:

| Topic | Decision | Notes |
|:---|:---|:---|
| ALARM / fault lines | **Six independent active-low inputs** to the MCU (`DRV_ALARM_J1` … `DRV_ALARM_J6`) | Pull-ups on the controller board; optional RC at the driver if the manual recommends noise suppression |
| MG996R gripper power | **`5V_SERVO` rail** from a **third** LM2596 (or UBEC) fed from `24V_MOTOR` | Do **not** share `5V_LOGIC` or `5V_PI`; star ground at the PSU return |
| Closed-loop driver logic level | **5 V TTL mode** on each CL57T / CL42T (DIP / jumper per vendor manual) | Matches `74HC245` outputs; verify against the exact driver revision (`V41` kits) |
| MCU / dev board for diagrams | **STM32 Nucleo-F401RE** (`STM32F401RET6`) | Schematics and [firmware/pinout.md](../../firmware/pinout.md) match `platformio.ini` (`nucleo_f401re`). **Power**: board expects **5 V** (e.g. ST-Link **Micro-B USB** from PC or bench supply); the MCU runs at **3.3 V** internally — see [electrical_design.md](electrical_design.md) §1b note. |
| PC programming cable | **USB A – Micro-B** | **ST-Link / VCP** on the Nucleo (not the Pi’s USB-C power cord). Blue molded cables are common; rating/color does not change USB 2.0 behavior. |

## 6. End-Effector / Gripper

The MG996R servo **is** the gripper actuator. Mechanically it lives **inside the gripper assembly** that bolts to the **J6 tool flange**, not in the arm wrist itself. Treat the EOAT as its own electrical sub-system that crosses the wrist on a single 4-pin tool-flange connector:

| Pin | Signal | Voltage | Notes |
|:---:|:---|:---|:---|
| 1 | `5V_SERVO` | 5 V | Dedicated rail from the third LM2596 buck (`24V_MOTOR` to 5 V). **Not** shared with `5V_LOGIC` or `5V_PI`. |
| 2 | `GND_M` | — | Star-grounded at the PSU return alongside the motor rail. |
| 3 | `GRIPPER_PWM` | 3.3 V | One STM32 timer pin (e.g. `PA8 / TIM1_CH1`) feeding a ~50 Hz hobby-servo frame. No level shifter required. |
| 4 | `GRIPPER_FSR` | 0–3.3 V | Optional Force Sensitive Resistor on the gripper fingertip → STM32 ADC pin via a 10k divider. |

See [`hardware/wireviz/60_end_effector.yml`](../../hardware/wireviz/60_end_effector.yml) for the harness diagram.

## 7. Open Items

- Final STM32 GPIO numbers (CubeMX / Nucleo pinmux vs firmware `pinout.md`).
- Exact connector family for the control box and joint harnesses.
- Whether J5 and J6 get AS5600 feedback at all (default plan: skip until measured drift forces the issue).
