# Electrical Interface Map

This document defines the electrical interfaces for the robot arm at the contract level: signals, voltage domains, connector usage, and expected behavior. Exact MCU GPIO assignments can be filled in once the control-board layout is finalized.

## 1. System-Level Signals

| Signal | Direction | Voltage | Source | Sink | Notes |
|:---|:---:|:---:|:---|:---|:---|
| AC mains in | In | Mains | Wall outlet | IEC inlet | Routed through fuse and E-stop |
| 24V motor rail | Out | 24V DC | PSU | Motor drivers | Main actuator power |
| 5V logic rail | Out | 5V DC | Buck converter | STM32, logic ICs, sensors | Isolated from motor rail where practical |
| Pi 5V rail | Out | 5V DC | Dedicated buck converter | Raspberry Pi | Kept separate from noisy motor logic rail |
| Servo 5V rail | Out | 5V DC | Third buck from `24V_MOTOR` | MG996R gripper only | Isolated from `5V_LOGIC` / `5V_PI` for stall-current spikes |
| STEP signals | Out | 3.3V -> 5V | STM32 | 74HC245 -> drivers | One per joint |
| DIR signals | Out | 3.3V -> 5V | STM32 | 74HC245 -> drivers | One per joint |
| ENABLE signals | Out | 3.3V -> 5V | STM32 | 74HC245 -> drivers | One per joint |
| ALARM / FAULT | In | Driver output | Drivers | STM32 | Active-low fault input assumed |
| Hall home inputs | In | 5V or open-collector | A3144 sensors | STM32 | One per joint |
| I2C SCL/SDA | Bi-dir | 3.3V or 5V via pullups | STM32 | TCA9548A / AS5600 | Shared bus, muxed if needed |
| UART TX/RX | Bi-dir | 3.3V | Raspberry Pi / STM32 | STM32 / Raspberry Pi | High-level command channel |

## 2. Driver Logic Map

### 2a. J1-J4 Closed-Loop Driver Control

| Joint | Motor / Driver | Control Signals | Feedback Signals | Notes |
|:---|:---|:---|:---|:---|
| J1 | NEMA 23 / CL57T | STEP, DIR, ENABLE | ALARM | Closed-loop correction lives in the driver |
| J2 | NEMA 23 / CL57T | STEP, DIR, ENABLE | ALARM | Same interface as J1 |
| J3 | NEMA 17 / CL42T | STEP, DIR, ENABLE | ALARM | Same electrical contract, lower current |
| J4 | NEMA 17 / CL42T | STEP, DIR, ENABLE | ALARM | Same electrical contract, lower current |

### 2b. J5-J6 Open-Loop Driver Control

| Joint | Motor / Driver | Control Signals | Feedback Signals | Notes |
|:---|:---|:---|:---|:---|
| J5 | NEMA 14 / TMC2209 | STEP, DIR, ENABLE | Optional AS5600 + Hall home | Quiet wrist joint, open-loop by default |
| J6 | NEMA 14 / TMC2209 | STEP, DIR, ENABLE | Optional AS5600 + Hall home | Quiet tool-roll joint, open-loop by default |

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

| Pin | Signal | Function |
|:---:|:---|:---|
| 1 | VCC | Sensor supply |
| 2 | GND | Sensor ground |
| 3 | HOME | A3144 Hall output |
| 4 | SCL | I2C clock for AS5600 / mux |
| 5 | SDA | I2C data for AS5600 / mux |
| 6 | SHIELD | Cable shield / drain |

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
| MCU / dev board for diagrams | **STM32 Nucleo-F401RE** (`STM32F401RET6`) | Schematics and [firmware/pinout.md](../../firmware/pinout.md) match `platformio.ini` (`nucleo_f401re`) |

## 6. Open Items

- Final STM32 GPIO numbers (CubeMX / Nucleo pinmux vs firmware `pinout.md`).
- Exact connector family for the control box and joint harnesses.
- Whether J5 and J6 get AS5600 feedback immediately or stay open-loop for phase 1.
