<!--
This file stores calculations / estimates made regarding the project.
It serves as the mathematical reasoning and validation behind engineering decisions.
-->

# Robot Arm — Engineering Calculations & Validation

This document contains the quantitative analysis that underpins hardware selection, gearing ratios, power budgeting, and link sizing decisions. All values should be treated as **design estimates** until validated via CAD and physical testing.

---

## 1. Torque Requirements

### 1a. Fundamental Formula

The critical constraint of a robot arm is the **static holding torque** required at each joint when the arm is at its worst-case configuration — fully extended, horizontally.

$$\tau_{joint} = \sum_{i} (M_i \times g \times L_i)$$

Where:
- $M_i$ = mass of link $i$ and all links distal to it (kg)
- $g$ = 9.81 m/s²
- $L_i$ = distance from the joint to the center of mass of segment $i$ (m)

### 1b. Link Mass Estimates (Preliminary)

These are estimates based on similar open-source arms (AR4, BCN3D Moveo). They will be updated with exact values once the PETG parts are modeled in CAD with material densities assigned.

| Link | Component | Estimated Mass |
|:---|:---|:---:|
| Link 1 (Upper Arm) | PETG structure + NEMA 17 motor + J3 cycloidal | ~850 g |
| Link 2 (Forearm) | PETG structure + NEMA 17 + GT2 belt assembly | ~500 g |
| Link 3 (Wrist Assembly) | PETG + 2× NEMA 14 + end-effector | ~300 g |
| Payload (design target) | Object being manipulated | ~500 g |

### 1c. Shoulder Joint (J2) — Worst-Case Calculation

J2 (Shoulder Pitch) is the **critical joint** — it must lift the entire extended arm. Using link centers of mass at the midpoint of each link:

Assume link lengths: L_upper = 280mm, L_forearm = 250mm, L_wrist = 100mm

$$\tau_{J2} = (M_{payload} \times g \times L_{total}) + (M_{link1} \times g \times L_{cm1}) + (M_{link2} \times g \times L_{cm2}) + (M_{link3} \times g \times L_{cm3})$$

$$\tau_{J2} = (0.5 \times 9.81 \times 0.63) + (0.85 \times 9.81 \times 0.14) + (0.50 \times 9.81 \times 0.405) + (0.30 \times 9.81 \times 0.58)$$

$$\tau_{J2} = 3.09 + 1.17 + 1.98 + 1.71 = \textbf{7.95 Nm}$$

> **Raw NEMA 23 holding torque: ~2.0 Nm.** This confirms that a gearbox is **mandatory** at J1 and J2.

### 1d. Required Gear Ratio at J2

To safely hold 7.95 Nm with a 2.0 Nm motor, the minimum gear ratio is:

$$R_{min} = \frac{\tau_{required}}{\tau_{motor}} = \frac{7.95}{2.0} = 3.98\times$$

However, a robot arm doesn't just hold still — it must also **accelerate the load**. Applying a **dynamic safety factor of 2.5×** (common for smooth, non-jerky trajectories):

$$R_{design} = R_{min} \times SF = 3.98 \times 2.5 = \approx 10\times$$

The chosen **cycloidal drive** provides a **20:1 to 30:1** ratio, giving a substantial safety margin. At 20:1, the effective output torque is:

$$\tau_{output} = 2.0 \text{ Nm} \times 20 = \textbf{40 Nm}$$

This provides **5× headroom** over the calculated worst-case, which accounts for: dynamic accelerations, friction within the cycloidal drive, link mass growth as the CAD is fleshed out, and future payload increases.

### 1e. Elbow Joint (J3) — Calculation

J3 only needs to lift Link 2 (forearm) + Link 3 (wrist) + payload:

$$\tau_{J3} = (0.5 \times 9.81 \times 0.35) + (0.50 \times 9.81 \times 0.125) + (0.30 \times 9.81 \times 0.30)$$

$$\tau_{J3} = 1.72 + 0.61 + 0.88 = \textbf{3.21 Nm}$$

The NEMA 17 (80 Ncm = 0.80 Nm) + a **3:1 belt reduction** (16T → 48T pulleys) gives:

$$\tau_{output} = 0.80 \times 3 = 2.40 \text{ Nm}$$

This is slightly under. Options:
1. Increase belt reduction to 4:1 (16T → 64T)
2. Add a second-stage cycloidal (~5:1) at J3
3. Reduce link mass targets in CAD

> **This is a known tension point** — final resolution requires accurate CAD masses.

---

## 2. Power Budget

### 2a. Motor Current Draw

Closed-loop stepper drivers set motor current via DIP switch or software. The values below are configured in the driver, not the motor spec:

| Joint | Motor | Driver | Configured Current | Power @ 24V |
|:---|:---|:---|:---:|:---:|
| J1 (Base) | NEMA 23 2.0Nm | CL57T | 4.0A | 96W |
| J2 (Shoulder) | NEMA 23 2.0Nm | CL57T | 4.0A | 96W |
| J3 (Elbow) | NEMA 17 80Ncm | CL42T | 3.0A | 72W |
| J4 (Forearm) | NEMA 17 42Ncm | CL42T | 2.0A | 48W |
| J5 (Wrist P) | NEMA 14 14Ncm | TMC2209 | 0.4A | 9.6W |
| J6 (Tool R) | NEMA 14 14Ncm | TMC2209 | 0.4A | 9.6W |
| **Simultaneous Max** | | | **13.8A** | **331W** |

> **PSU Selection**: The **Mean Well LRS-350-24** (350W, 14.6A) covers the theoretical maximum with a small buffer. In practice, J1 and J2 are never simultaneously at full torque during normal pick-and-place motions, so the real-world average draw is approximately **150–200W**.

### 2b. Logic Power

| Component | Voltage | Current | Source |
|:---|:---:|:---:|:---|
| Raspberry Pi 3 | 5V | 2.5A | Dedicated LM2596 buck |
| STM32 | 5V (USB) | ~0.5A | Shared LM2596 |
| 4× Closed-loop drivers (logic) | 5V | ~0.2A | Shared LM2596 |
| 2× TMC2209 | 3.3V | ~0.1A | STM32 3.3V rail |
| Hall sensors × 6 | 5V | ~0.05A | Shared LM2596 |
| **Logic Total** | | **~3.35A @ 5V** | = ~17W from 24V rail |

Total system budget: **~350W peak**, well within the PSU rating.

---

## 3. Microstepping Resolution

### 3a. Angular Resolution per Step

Stepper motors have **1.8°** per full step (200 steps/revolution). With microstepping:

$$\theta_{step} = \frac{1.8°}{M}$$

Where $M$ is the microstep divisor (2, 4, 8, 16, 32...).

Before the gearbox, at 1/16 microstepping:
$$\theta_{step\_raw} = \frac{1.8°}{16} = 0.1125° \text{ per microstep}$$

After a 20:1 cycloidal drive, the output resolution becomes:
$$\theta_{output} = \frac{0.1125°}{20} = \textbf{0.0056°} \text{ = 5.6 millidegrees per microstep}$$

This is approximately **64,000 microsteps per output revolution** — exceptional resolution for a desktop arm.

### 3b. Linear End-Effector Resolution

At full arm extension (L ≈ 630mm = 0.63m), the smallest angular step at J1 translates to a linear displacement at the end-effector of:

$$\Delta x = L \times \sin(\Delta\theta) \approx L \times \Delta\theta_{rad}$$

$$\Delta x = 0.63 \times \frac{0.0056 \times \pi}{180} = 0.63 \times 0.0000977 = \textbf{0.062 mm}$$

Theoretical end-effector resolution at J1: **~62 micrometers**. Practical positioning repeatability will be dominated by mechanical compliance (cycloidal drive flex, PETG print tolerances, belt stretch), not step resolution. Realistic repeatability target: **±0.5mm to ±1.0mm**.

---

## 4. Cycloidal Drive Geometry

### 4a. Tooth Count & Ratio

The cycloidal drive ratio is determined by:

$$R = N_{pins}$$

Where $N_{pins}$ is the number of ring gear pins. A typical desktop-scale design uses:

- **24 pins** → 24:1 ratio
- **20 pins** → 20:1 ratio (chosen for compactness with M4-sized dowel pins)

The cycloidal disk has **19 lobes** (one fewer than the 20 pins), causing one complete rolling cycle to produce exactly 1/20th of an output revolution.

### 4b. Eccentric Offset

The eccentric cam (608ZZ bearing press-fit on the motor shaft) has an offset of typically **1.0–1.5mm**. This offset determines the amplitude of the epitrochoidal path:

$$e = 1.0 \text{ mm (nominal for desktop scale)}$$

Larger eccentricity → higher load capacity but more vibration and imbalance. At 1.0mm eccentricity with a 6201 counterweight on the rear shaft, vibration is manageable.

---

## 5. PETG Print Strength Validation

### 5a. Shear Stress on Heat-Set Insert

A typical M3×5mm brass heat-set insert in PETG, subjected to a pull-out or shear load from a motor mounting bolt:

- **Insert shear area**: π × D × L = π × 3mm × 5mm ≈ 47 mm²
- **PETG shear strength**: ~25–35 MPa (conservative: 25 MPa)
- **Max shear force**: 47 mm² × 25 N/mm² = **1,175 N ≈ 120 kgf**

A NEMA 23 mounting bolt generates at most a few N·m of tightening torque → bolt tension ~500–800N. The heat-set insert has **~1.5–2.3× safety factor** in shear. Using 4× heat-set inserts per motor mount (as is standard) brings the combined pull-out force well above any realistic load.

> **Conclusion**: Heat-set inserts in PETG for motor mounting are mechanically sound for this application.

---

## 6. Summary — Engineering Decision Outcomes

| Decision | Calculation-Driven Rationale |
|:---|:---|
| Cycloidal drives at J1/J2 | Required 10× minimum, 20× chosen for 5× safety margin |
| NEMA 23 at J1/J2 (2.0 Nm) | Only motor class with sufficient torque before gearing |
| NEMA 17 at J3 (80 Ncm) | Belt reduction needed; J3 torque requirement is borderline |
| NEMA 14 at J5/J6 | Torque needs are <0.14 Nm; weight dominates → lightest possible |
| Mean Well LRS-350-24 PSU | Covers 13.8A theoretical max with margin |
| PETG for structure | 80°C Tg, excellent layer adhesion, validated heat-set shear strength |
| 1/16 microstepping | 5.6 millidegree resolution post-gearbox; >100× finer than repeatability target |
