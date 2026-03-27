# Electrical Design

<!--
This document details the complete electrical architecture:
power system, compute hierarchy, motor drivers, sensors, wiring, and safety systems.
-->

## 1. Power System

### 1a. Main Power Supply — Mean Well LRS-350-24

The selected PSU is the **Mean Well LRS-350-24** (350W, 24V, 14.6A). This choice is driven by the combined peak current draw of all 6 motors:

| Motor | Driver | Peak Current | Count | Subtotal |
|:---|:---|:---|:---:|:---|
| NEMA 23 (2.0 Nm) | CL57T | ~4.0A | 2 | 8.0A |
| NEMA 17 (80 Ncm) | CL42T | ~3.0A | 1 | 3.0A |
| NEMA 17 (42 Ncm) | CL42T | ~2.0A | 1 | 2.0A |
| NEMA 14 (Pancake) | TMC2209 | ~0.4A | 2 | 0.8A |
| **Total (Theoretical Max)** | | | | **~13.8A** |

The LRS-350-24 provides **14.6A**, giving a small but safe margin. Critically, **software motion constraints** are enforced to ensure J1 and J2 (the two highest-current joints) are never both at peak torque simultaneously, keeping the real-world draw well below the PSU limit.

> **Why 24V?** Stepper motor back-EMF scales with RPM. A higher bus voltage allows the driver to force current through the motor windings faster, maintaining torque at higher speeds. Running at 12V would cause torque rolloff at speeds required for smooth arm motion.

### 1b. Logic Power — LM2596 Buck Converters

The 24V rail is **stepped down** to lower voltages for logic components:

| Rail | Voltage | Consumers |
|:---|:---|:---|
| Logic 5V | 5V | STM32 (via USB or 5V pin), 74HC245 level shifters, Hall sensors |
| Logic 3.3V | 3.3V | STM32 internal (regulated on-board from 5V) |
| Pi Power | 5V/3A | Raspberry Pi 3 (via dedicated buck converter for isolation) |

The LM2596-based buck converter modules are compact and inexpensive. A **separate** buck converter powers the Pi to isolate it from the motor noise on the main logic 5V rail.

### 1c. Safety — E-Stop & IEC C14 Inlet

- **E-Stop Button**: Latching mushroom-head button wired **in series** with the PSU's AC input. Pressing it immediately cuts all power to the arm — zero software dependency on safety.
- **IEC C14 Inlet with Fuse**: A panel-mounted C14 connector with an integrated fuseholder. The fuse is sized to the PSU's rated input current as the last-resort overcurrent protection.

---

## 2. Compute Hierarchy

The system uses a **two-tier compute architecture** to separate real-time motor control from high-level planning:

```
[ Raspberry Pi 3 ]          <-- High-Level (ROS 2, MoveIt 2, IK/FK)
        |
    USB / UART
        |
    [ STM32 ]               <-- Low-Level (Real-time PWM, PID, I2C encoders)
        |
  +-----------+-----------+
  |           |           |
[CL57T x2] [CL42T x2] [TMC2209 x2]
  |           |           |
NEMA23x2   NEMA17x2   NEMA14x2
```

- The **Raspberry Pi 3** runs ROS 2 in Docker containers. It handles trajectory planning, inverse kinematics, and user-facing interfaces. It does **not** perform any real-time motor control.
- The **STM32** is a hard-real-time microcontroller. It runs a >1 kHz control loop, generates STEP/DIR pulses, reads encoder feedback, and executes PID corrections.

---

## 3. Motor Drivers

### 3a. CL57T — Closed-Loop Driver (J1 & J2, NEMA 23)

The CL57T is a dedicated closed-loop stepper driver. It accepts a STEP/DIR signal from the STM32 and uses the bundled encoder feedback to internally correct for missed steps **without** requiring the STM32 to handle the correction loop. Key settings:

- **Current Range**: 0–8.0A (configured via DIP switches for the 2.0 Nm motor)
- **Input Voltage**: 20–50V DC (24V rail is ideal)
- **Step Resolution**: Configurable from full-step to 1/256 microstepping
- **Alarm Output**: Pulls low on fault (stall, overcurrent) — wired to STM32 interrupt pin

### 3b. CL42T — Closed-Loop Driver (J3 & J4, NEMA 17)

Same closed-loop principle as CL57T but sized for NEMA 17 current ranges:

- **J3** (80 Ncm motor): CL42T configured up to 3.0A
- **J4** (42 Ncm motor): CL42T configured up to 2.0A

### 3c. TMC2209 — Open-Loop Driver (J5 & J6, NEMA 14)

The NEMA 14 wrist motors are driven by TMC2209 modules, the standard for silent, smooth stepper control:

- **StealthChop2**: Near-silent operation at speeds below the threshold (important for lab environment)
- **SpreadCycle**: Falls back to high-efficiency full-power mode for demanding moves
- **Current**: Configured for 0.4A (NEMA 14 rated current) via VREF trimmer or UART
- **No Encoder Required**: The wrist joints carry minimal load, and microstepping resolution at 1/16 or 1/32 is sufficient. If precision becomes an issue, external AS5600 encoders can be added.

### 3d. 74HC245 — Logic Level Shifter

The STM32 outputs **3.3V logic** signals (STEP/DIR). The CL57T and CL42T industrial drivers expect **5V logic** levels. The 74HC245 octal bus transceiver shifts all 3.3V STEP/DIR signals to 5V:

- 1 chip handles 4 signal pairs (8 lines) — one per joint per direction
- Powered by the 5V logic rail; DIR pin ties to the 24V signal enable

---

## 4. Sensors & Feedback

### 4a. Built-in Encoder Feedback (J1–J4)

The closed-loop kits (CL57T and CL42T) include a **motor-mounted encoder**. The driver reads this internally, closing the current loop without STM32 involvement. The STM32 can still query the driver's actual position register over RS-485 for position telemetry if needed.

### 4b. AS5600 Magnetic Encoders (J5–J6 Optional / Secondary)

For open-loop wrist joints or secondary absolute confirmation:
- **Principle**: A small diametrically magnetized magnet glued to the motor shaft rotates above the AS5600 IC. The IC measures field angle and outputs 12-bit position (0–4095 counts per revolution) over I2C.
- **Address Conflict**: All AS5600s share the fixed I2C address `0x36`. An **I2C multiplexer (TCA9548A)** is required to address them individually.
- **Resolution**: 12 bits = 4096 steps/rev → **0.088°/count** raw; with microstepping this allows extremely fine angular measurements.

### 4c. A3144 Hall Effect Sensors — Homing

On each joint, an **A3144 Hall effect sensor** and a small neodymium magnet establish the **absolute zero (home) position** at startup:
- The motor sweeps slowly until the Hall sensor triggers → STM32 records this as the joint's `0°` reference.
- This eliminates reliance on limit microswitches, which have mechanical wear.

---

## 5. Wiring & Signal Integrity

### 5a. Wire Gauge Selection

| Circuit | AWG | Rationale |
|:---|:---|:---|
| Motor phase wires (NEMA 23) | 18 AWG | Up to 4A continuous; silicone jacket for flex and heat |
| Motor phase wires (NEMA 17/14) | 22 AWG | Lower current; smaller and lighter |
| Logic signals (STEP/DIR, I2C) | 22 AWG shielded | Shielding prevents motor PWM noise from corrupting encoder data |
| Power distribution bus | 16 AWG | Main 24V bus from PSU to driver terminal blocks |

### 5b. Connectors

- **GX16 Aviation Connectors**: Locking metal connectors on the control box panel for motor cables. Rated for high vibration.
- **Wire Ferrules (Crimped)**: All wires inserted into screw-terminal blocks (drivers, PSU) are terminated with ferrules. Bare stranded wire under a screw terminal can cold-flow and loosen over time — ferrules prevent this.
- **PET Sleeving & Drag Chain**: Motor cables are bundled in braided PET sleeving and routed through a drag chain at the base rotation joint to protect them from fatigue during continuous J1 sweeps.

### 5c. STM32 Breakout / Protoboard

To avoid point-to-point breadboard wiring in the final assembly, a **custom perfboard "hat"** is constructed for the STM32:
- Pin headers for each driver's STEP/DIR/ENABLE lines
- Screw terminals for 5V logic power and ground
- Pads for the 74HC245 level shifter IC
- I2C header for TCA9548A multiplexer
- UART header for Raspberry Pi connection

---

## 6. Validation Strategy Without Hardware

If no physical components are on hand, the electrical side can still be validated at the level that matters most right now: power, interfaces, fault handling, and wiring assumptions.

### 6a. What Can Be Checked Now

- **Power budget math**: Recheck PSU headroom, per-rail current, fuse sizing, and startup margin.
- **Interface contracts**: Define exact STEP/DIR/ENABLE polarity, logic levels, fault-pin behavior, and connector pinouts.
- **Signal integrity assumptions**: Confirm that 3.3V to 5V shifting, pullups, and cable lengths are reasonable for the chosen wiring scheme.
- **Safety behavior**: Document what should happen on E-stop, driver alarm, serial timeout, and encoder failure.
- **Harness documentation**: Freeze a wiring table so the physical build later matches the software assumptions.

### 6b. What Is Not Worth Simulating Yet

- Detailed stepper winding physics for every motor.
- Closed-loop behavior of the CL57T/CL42T drivers unless vendor models are available.
- Full EMI/noise behavior of the real harness.

Those are only useful once the actual parts and cable lengths exist. Before that, they add complexity without improving the design much.

### 6c. Recommended No-Hardware Work Products

1. A pin-by-pin electrical map for the STM32, level shifter, drivers, sensors, and power rails.
2. A current budget table that includes peak, average, and fault-state draw.
3. A fault-state matrix showing how the firmware and hardware should respond to each failure.
4. A bench bring-up checklist for when the first PSU, driver, and motor are available.

The corresponding docs are:

- [Electrical interface map](electrical_interface_map.md)
- [Electrical schematic plan](electrical_schematic_plan.md)
- [Electrical bring-up checklist](electrical_bringup_checklist.md)

For this project, that means the immediate electrical deliverable is a **clean interface specification**, not a full circuit simulator.
