# Mechanical Design & Kinematics

<!--
This document details the mechanical architecture, actuator selection, power transmission strategy,
structural materials, and kinematic approach for the 6-DOF robot arm.
-->

## 1. Design Goals & Payload Estimate

The arm is designed for **desktop-scale pick-and-place operations**, targeting an estimated payload of **0.5 – 1.0 kg** at the end-effector. The total arm reach is targeted at **~500mm** (from shoulder to wrist) based on the link-length breakdown below. These targets will be validated in simulation before final hardware procurement.

**Gear-ratio authority.** Firmware uses `kJ1MotorRevsPerJointRev = 19.0F` and `kJ2MotorRevsPerJointRev = 15.0F` in `firmware/stm32_core/lib/drivers/src/stepper_driver.cpp`. Physical pin/lobe counts are `[NEEDS MEASUREMENT]`. Effective-output-torque products that assumed twenty-to-one reduction are superseded.

---

## 2. Actuator Selection

The motor stack is split into three tiers based on joint position and torque requirement. Mounting heavier motors closer to the base is a deliberate strategy to minimize the **moving moment of inertia** — the further mass sits from the base pivot, the more energy is wasted just swinging the arm itself.

| Joint | Description | Motor | Driver | Torque | Gearing | Eff. Output Torque |
|:---|:---|:---|:---|:---|:---|:---|
| J1 | Base Rotation | NEMA 23 (2.0 Nm) | CL57T Closed-Loop | 2.0 Nm | Cycloidal 19:1 (firmware) | `[NEEDS MEASUREMENT]` |
| J2 | Shoulder Pitch | NEMA 23 (2.0 Nm) | CL57T Closed-Loop | 2.0 Nm | Cycloidal 15:1 (firmware) | `[NEEDS MEASUREMENT]` |
| J3 | Elbow Pitch | NEMA 17 (80 Ncm) | CL42T Closed-Loop | 0.80 Nm | Cycloidal 15:1 | **~12 Nm** |
| J4 | Forearm Twist | NEMA 17 (42 Ncm) | CL42T Closed-Loop | 0.42 Nm | Cycloidal 10:1 | **~4.2 Nm** |
| J5 | Wrist Pitch | NEMA 14 (14 Ncm) | TMC2209 Open-Loop | 0.14 Nm | Direct | **0.14 Nm** |
| J6 | Tool Roll | NEMA 14 (14 Ncm) | TMC2209 Open-Loop | 0.14 Nm | Direct | **0.14 Nm** |

> **Note on Closed-Loop Kits**: J1–J4 use integrated closed-loop stepper kits (`1-CL57T-S20-V41` for J1/J2, `1-CL42T-S08-V41` for J3, `1-CL42T-S04-V41` for J4) which include the motor, matched driver, and encoder wiring. The built-in encoder closes the loop at the driver level — no external AS5600 required on these joints.

---

## 3. Gearing & Power Transmission

All four driven joints (J1–J4) use **cycloidal drives**. This eliminates the torque shortfall that belt-only drives had at J3, unifies the manufacturing process (same print profiles, same components), and removes belt-stretch as a source of positional error.

### 3a. Cycloidal Drive Design (All J1–J4)

A cycloidal drive is a compact, high-ratio speed reducer that achieves a large gear ratio in a single short stage. Its key properties:
- **3D Printable**: Ring pins are hardened steel dowel pins (3–5mm); the disk and housing are PETG. No metal machining.
- **High Ratio in Compact Form**: 10:1–30:1 in a package shorter than the motor itself.
- **Near-Zero Backlash**: The disk is always in contact with multiple pins simultaneously — no gear mesh "slop".
- **Load Shared Across Many Pins**: Force is distributed across ~half the pins at any moment, drastically reducing peak contact stress vs. a single-tooth mesh.

**Ratio selection per joint** (CAD intent, not counted on hardware). Firmware J1/J2 constants are 19.0 and 15.0; physical `[NEEDS MEASUREMENT]`.

| Joint | Pins (N) CAD intent | Disk Lobes (N-1) CAD intent | Ratio in controller | Rationale |
|:---:|:---:|:---:|:---:|:---|
| J1 | `[NEEDS MEASUREMENT]` | `[NEEDS MEASUREMENT]` | 19:1 firmware | Highest torque demand |
| J2 | `[NEEDS MEASUREMENT]` | `[NEEDS MEASUREMENT]` | 15:1 firmware | Not identical to J1 in firmware; do not assume a shared housing ratio |
| J3 | 15 | 14 | 15:1 | Resolves belt shortfall; smaller housing fits NEMA 17 envelope |
| J4 | 10 | 9 | 10:1 | Low torque demand; smallest housing, minimizes added mass |

**Key Components (per drive unit):**
- **Eccentric cam**: One `608ZZ` bearing press-fit onto the motor shaft, offset by **1.0 mm** from centre
- **Counterweight disk**: A second `608ZZ` (or equivalent mass) press-fit on the rear shaft at 180° offset — cancels eccentric imbalance and reduces vibration (see §3c)
- **Cycloidal disk**: 3D-printed PETG with epitrochoidal tooth profile; toleranced at +0.1 to +0.15 mm clearance to ring pins
- **Ring gear pins**: Hardened steel M4 dowel pins (4mm ⌀) press-fit into the housing — they cannot shear; only the disk is consumable
- **Output bearings**: `6806` or `6808` thin-section bearings at J1/J2; `6804` or `6806` at J3/J4 (smaller motor, smaller housing)
- **Output pin carrier**: A second set of output pins (same M4 dowels) in the output flange that engage the drive holes in the cycloidal disk — transmit rotation without transmitting the eccentric motion

> **Lubrication is mandatory**: Pack synthetic PTFE grease into every cycloidal assembly before first use. Dry running will melt and destroy the PETG disk within minutes of sustained load.

---

### 3b. Vibration in Cycloidal Drives & Mitigation

The rotating eccentric cam creates a **centrifugal imbalance force** that, if unmitigated, causes the whole arm to vibrate — particularly at J3 and J4 which are cantilevered further from the base.

#### Source of Vibration

The eccentric cam assembly (608ZZ bearing, ~20g, offset 1.0 mm from the shaft centreline) acts as an unbalanced rotating mass:

$$F_{imbalance} = m_{ecc} \times e \times \omega^2$$

- At **300 RPM** → ω = 31.4 rad/s → F ≈ **20 mN** (negligible)
- At **1000 RPM** → ω = 104.7 rad/s → F ≈ **220 mN** (noticeable, especially at end-effector)
- At **2000 RPM** → ω = 209.4 rad/s → F ≈ **880 mN** (significant — unacceptable for precision tasks)

High motor speeds on the *input* side of a high-ratio cycloidal drive are common (output moves slowly, but motor spins fast). Normal operational input speeds of 300–600 RPM are expected for smooth trajectories. RPM at the firmware J1/J2 ratios for a given output rate is `[NEEDS MEASUREMENT]`.

#### Mitigation Strategies (Priority Order)

1. **Counterweight Disk (Mandatory)**: A second bearing or custom-machined/printed disk of matching mass is mounted on the **rear** of the motor shaft, diametrically opposite (180° offset from) the eccentric cam. This statically balances the rotating assembly, eliminating the imbalance force entirely at all speeds. **Must be implemented on all four cycloidal drives.**

2. **Twin-Disk (Dual-Phase) Cycloidal (Strongly Recommended for J1/J2)**: Use two cycloidal disks stacked axially, keyed 180° out of phase on the same eccentric. Their eccentric forces cancel each other continuously. This also doubles the load capacity and smooths the torque ripple from the individual disk lobes engaging pins. J1 and J2 should use this design given their higher duty cycles and the fact that any base vibration is amplified up the full arm length.

3. **S-Curve Velocity Profiles (Software — Mandatory)**: MoveIt 2 trajectory parameterization must use **S-curve** (jerk-limited) profiles rather than trapezoidal profiles. Trapezoidal profiles produce instantaneous acceleration changes that stimulate resonant modes of the cycloidal drive and PETG links. S-curves limit the rate of change of acceleration (jerk), preventing resonance excitation. Configure in `ompl_planning.yaml` and the `joint_limits.yaml` jerk parameters.

4. **Microstepping (1/32 or Higher)**: Higher microstepping reduces the per-step torque impulse from the stepper motor. Each individual step is smaller, reducing the amplitude of periodic forcing at the stepping frequency. The CL42T and CL57T support up to 1/256 microstepping — the default of 1/16 is a minimum; 1/32 is recommended for J3/J4.

5. **PTFE Grease (Mandatory, also Dampens)**: A well-greased cycloidal drive is significantly quieter and more vibration-damped than a dry one. The grease film between the disk and ring pins acts as a viscous damper that absorbs high-frequency vibration energy.

---

## 4. Structural Materials & Manufacturing

### 4a. Primary Structure: 3D-Printed PETG
PETG is chosen over PLA or ABS for the following reasons:

| Property | PLA | PETG | ABS |
|:---|:---|:---|:---|
| Glass Transition Temp | ~60°C | **~80°C** | ~100°C |
| Layer Adhesion | Good | **Excellent** | Fair |
| Warp Resistance | Good | **Excellent** | Poor |
| Impact Resistance | Brittle | **Good flex** | Good |
| Post-print difficulty | Easy | **Easy** | Hard (fumes, warp) |

Motors generate significant heat. PETG's 80°C glass transition temperature provides headroom that PLA simply does not.

### 4b. Fasteners & Inserts
- **M3 / M4 / M5 Alloy Steel Bolts & Nuts**: Standard metric hardware for all structural connections
- **M3×5mm Brass Heat-Set Inserts**: Press-in with a soldering iron; creates a permanent metallic threaded boss inside PETG that withstands high-torque motor mounting without stripping
- **Blue Loctite**: Applied to all motor mount fasteners to prevent vibration-induced loosening

### 4c. Bearings
- **J1/J2 Output (Cycloidal)**: `6806` or `6808` thin-section bearing — carries the full structural (axial bending) load at the base and shoulder joints
- **J3/J4 Output (Cycloidal)**: `6804` or `6806` thin-section bearing — appropriate for the smaller NEMA 17 housing profile
- **Eccentric Cam (All Cycloidal)**: `608ZZ` bearings — one per disk stage as the eccentric cam rolling element
- **Counterweight (All Cycloidal)**: Second `608ZZ` (or printed mass) on the rear motor shaft, 180° offset from the eccentric cam
- **Motor shafts must NEVER carry structural loads** — motor shafts are only coupled for torque transmission. All bending/axial forces must pass through the output bearing.

---

## 5. End-Effector

The end-effector is a self-contained sub-assembly that bolts to the **J6 tool flange**. It is mechanically and electrically independent from the joint motors, which keeps tool changes from disturbing the arm wiring.

- **Gripper Actuator — `MG996R` metal-gear micro servo**.
  - **Mounting**: inside the gripper body itself (3D-printed claw / parallel-jaw assembly) that bolts to the J6 tool flange. The MG996R is **not** mounted in the arm wrist — J6 already has its own NEMA 14 stepper for tool roll. The gripper servo only opens / closes the jaws.
  - **Power**: dedicated `5V_SERVO` rail from a third LM2596 buck off `24V_MOTOR` (see [electrical_design.md §1b](electrical_design.md)). It is **not** sharing the `5V_LOGIC` or `5V_PI` rails — stall spikes from the servo would otherwise glitch the MCU.
  - **Signal**: a single ~50 Hz PWM line from one STM32 timer pin (e.g. `PA8 / TIM1_CH1`). 3.3 V drive is sufficient for a hobby servo's high-impedance signal pin, so no level shifter is required.
  - **Wiring across the wrist**: all EOAT signals (`5V_SERVO`, `GND_M`, `PWM`, optional `FSR`) cross the wrist on a single 4-pin tool-flange connector. See [`hardware/wireviz/60_end_effector.yml`](../../hardware/wireviz/60_end_effector.yml).
- **Optional Force Feedback**: a **Force Sensitive Resistor (FSR)** on the gripper fingertip pad, fed through a 10k voltage divider into a STM32 ADC pin. Enables a grip-force PID to prevent crushing delicate objects. Also lives inside the gripper body, not the arm.

---

## 6. Denavit-Hartenberg (DH) Parameters

The arm's kinematics are parameterized using the **Modified DH convention**. These parameters define the geometric relationship between each consecutive joint frame and are used directly in both the Python IK/FK solver and the URDF model.

| Joint | a (mm) | α (deg) | d (mm) | θ (variable) | Notes |
|:---:|:---:|:---:|:---:|:---:|:---|
| J1 | 0 | 90 | d1 | θ1 | Base rotation; d1 = height of base |
| J2 | L1 | 0 | 0 | θ2 | Shoulder pitch; L1 = upper arm length |
| J3 | L2 | 90 | 0 | θ3 | Elbow pitch; L2 = forearm length |
| J4 | 0 | -90 | d4 | θ4 | Forearm twist |
| J5 | 0 | 90 | 0 | θ5 | Wrist pitch |
| J6 | 0 | 0 | d6 | θ6 | Tool roll; d6 = flange offset |

> **L1, L2, d1, d4, d6** are physical link lengths determined by the CAD model. These will be updated once the first prototype joints are dimensioned.

---

## 7. Source CAD & export workflow

Authoritative mechanical CAD is in **`mechanical/sw-models/`** (SolidWorks). To keep the repo and reviews usable (without relying on Windows-only binary formats alone), see **[CAD source files & export conventions](cad_exports.md)** for which formats to export per part, assembly, and drawing, and how to name them.
