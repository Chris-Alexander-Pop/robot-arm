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
| Link 1 (Upper Arm) | PETG structure + NEMA 17 motor + J3 cycloidal housing | ~900 g |
| Link 2 (Forearm) | PETG structure + NEMA 17 + J4 cycloidal housing | ~550 g |
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

### 1e. Elbow Joint (J3) — Cycloidal Drive Calculation

J3 only needs to lift Link 2 (forearm) + Link 3 (wrist) + payload. Using preliminary masses from §1b:

$$\tau_{J3} = (0.5 \times 9.81 \times 0.35) + (0.55 \times 9.81 \times 0.125) + (0.30 \times 9.81 \times 0.30)$$

$$\tau_{J3} = 1.72 + 0.67 + 0.88 = \textbf{3.27 Nm}$$

The NEMA 17 (80 Ncm = 0.80 Nm) + **15:1 cycloidal drive** gives:

$$\tau_{output} = 0.80 \times 15 = \textbf{12.0 Nm}$$

This gives a safety factor of:

$$SF_{J3} = \frac{12.0}{3.27} = \textbf{3.67\times}$$

> **J3 binding constraint is resolved.** The belt drive approach (~2.4 Nm) was insufficient. The 15:1 cycloidal provides 3.67× headroom, matching the safety margin philosophy of J1/J2.

### 1f. Forearm Joint (J4) — Cycloidal Drive Calculation

J4 (forearm twist) rotates the distal assembly about the forearm's own axis. The dominant torque is from **inertia during acceleration**, not gravity — the axis of rotation passes through the forearm, so gravity generates no moment. The constraint is therefore inertial:

$$\tau_{J4\_inertia} = I_{distal} \times \alpha$$

Where $I_{distal}$ is the moment of inertia of Link 3 + payload about the J4 axis, and $\alpha$ is the desired angular acceleration.

- Estimated $I_{distal}$: 300g mass at ~0.1m radius → $I \approx 0.003 \text{ kg·m}^2$
- Design acceleration: $\alpha = 5 \text{ rad/s}^2$ (a reasonably fast wrist rotation)

$$\tau_{J4} = 0.003 \times 5 = \textbf{0.015 Nm}$$

The NEMA 17 (42 Ncm = 0.42 Nm) + **10:1 cycloidal drive** gives:

$$\tau_{output} = 0.42 \times 10 = \textbf{4.2 Nm}$$

This is **280× the required torque** — the 10:1 cycloidal is chosen not for torque headroom but to:
- **Maintain angular resolution**: 10:1 × 1/32 microstepping gives 0.0056°/step at the J4 output
- **Provide damping**: The gear reduction slows input speed → less inertial overshoot during direction reversals
- **Unify manufacturing**: Same dowel pins, same PETG disk profile as J1/J2/J3 (just with 10 pins instead of 15/20)

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
| Raspberry Pi 4 | 5V | 3.0A (budget peak) | Dedicated LM2596 buck (size for CPU + USB peripherals) |
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

## 5. Cycloidal Drive Vibration Analysis

### 5a. Eccentric Imbalance Force

Each cycloidal drive's eccentric cam (a 608ZZ bearing, mass $m_{ecc} \approx 20g = 0.020\text{ kg}$, offset $e = 1.0\text{ mm} = 0.001\text{ m}$ from the motor shaft centreline) produces a rotating centrifugal force:

$$F_{imbalance} = m_{ecc} \times e \times \omega^2$$

| Motor Input Speed | ω (rad/s) | F_imbalance | Assessment |
|:---:|:---:|:---:|:---|
| 300 RPM | 31.4 | **20 mN** | Negligible — below perception threshold |
| 600 RPM | 62.8 | **79 mN** | Minor — felt as slight buzz in PETG links |
| 1000 RPM | 104.7 | **220 mN** | Noticeable — degrades end-effector accuracy during motion |
| 2000 RPM | 209.4 | **880 mN** | Significant — must be mitigated at any ratio |

> **Context**: A 20:1 cycloidal drive outputs 1 revolution per 20 motor revolutions. To move a joint at 30°/s, the motor must spin at $30 \times 20 / 360 \times 60 \approx$ **100 RPM** — well in the low-concern zone for normal pick-and-place trajectories. Rapid slewing (180°/s output) would put motor at ~600 RPM — still acceptable with counterweighting.

### 5b. Counterweight Effectiveness

A counterweight of mass $m_{cw} = m_{ecc}$ placed at the same radius $e$ on the **opposite** side (180°) of the motor shaft completely cancels the imbalance:

$$F_{net} = m_{ecc} \times e \times \omega^2 - m_{cw} \times e \times \omega^2 = 0$$

This is a **static balance** — valid at all speeds. A second 608ZZ bearing (or a small 3D-printed annular weight) press-fit to the rear of the motor shaft at 180° offset achieves this for ~$2 in hardware.

### 5c. Twin-Disk (Dual-Phase) Effectiveness

The twin-disk design uses two cycloidal disks keyed 180° out of phase on the **same eccentric**. Since the eccentric offset vector points in the same direction for both disks, their imbalance forces do NOT cancel (unlike the counterweight). However, the benefit is:
- **Torque ripple** from lobe-pin engagement is cancelled (one disk is always mid-engagement when the other is at a transition)
- **Axial thrust** from the disk-pin contact is balanced (symmetric)
- **Load doubled**: Two disks share the output torque, halving peak contact stress per pin

For full vibration elimination, twin-disk + counterweight should both be used at J1/J2.

### 5d. Resonant Frequency Estimate (PETG Link)

A PETG link behaves as a cantilever beam. Its first natural frequency determines what excitation frequencies to avoid:

$$f_n = \frac{1}{2\pi} \sqrt{\frac{3EI}{mL^3}}$$

For the upper arm link (L ≈ 0.28m, PETG E ≈ 2.0 GPa, estimated I ≈ 1.5×10⁻⁸ m⁴ for a hollow rectangular section, m ≈ 0.30 kg):

$$f_n \approx \frac{1}{2\pi} \sqrt{\frac{3 \times 2.0 \times 10^9 \times 1.5 \times 10^{-8}}{0.30 \times 0.28^3}} \approx \frac{1}{2\pi} \sqrt{\frac{90}{6.59 \times 10^{-3}}} \approx \frac{1}{2\pi} \times 116.8 \approx \textbf{18.6 Hz}$$

This corresponds to a motor input speed of:

$$n_{resonant} = 18.6 \text{ Hz} \times 60 \text{ s/min} = \textbf{\approx 1116 RPM}$$

> **Practical implication**: Keep motor input speeds below ~800 RPM to maintain a comfortable margin below the estimated structural resonance. S-curve velocity profiles that limit jerk will also avoid exciting this mode during acceleration ramps.

---

## 6. PETG Print Strength Validation

### 6a. Shear Stress on Heat-Set Insert

A typical M3×5mm brass heat-set insert in PETG, subjected to a pull-out or shear load from a motor mounting bolt:

- **Insert shear area**: π × D × L = π × 3mm × 5mm ≈ 47 mm²
- **PETG shear strength**: ~25–35 MPa (conservative: 25 MPa)
- **Max shear force**: 47 mm² × 25 N/mm² = **1,175 N ≈ 120 kgf**

A NEMA 23 mounting bolt generates at most a few N·m of tightening torque → bolt tension ~500–800N. The heat-set insert has **~1.5–2.3× safety factor** in shear. Using 4× heat-set inserts per motor mount (as is standard) brings the combined pull-out force well above any realistic load.

> **Conclusion**: Heat-set inserts in PETG for motor mounting are mechanically sound for this application.

---

## 7. Summary — Engineering Decision Outcomes

| Decision | Calculation-Driven Rationale |
|:---|:---|
| Cycloidal drives at J1/J2 (20:1) | Required 10× dynamic minimum; 20:1 gives 4× safety margin on 7.95 Nm load |
| Cycloidal drive at J3 (15:1) | Belt at 3:1 yielded 2.4 Nm < 3.27 Nm required; 15:1 cycloidal gives 12.0 Nm (3.67× margin) |
| Cycloidal drive at J4 (10:1) | Torque demand is ~0.015 Nm; 10:1 chosen for resolution, damping, and manufacturing uniformity |
| NEMA 23 at J1/J2 (2.0 Nm) | Only motor class with sufficient torque before gearing |
| NEMA 17 at J3 (80 Ncm) | 15:1 cycloidal brings 0.80 Nm → 12 Nm; well above 3.27 Nm requirement |
| NEMA 17 at J4 (42 Ncm) | 10:1 cycloidal brings 0.42 Nm → 4.2 Nm; far exceeds inertial demand |
| NEMA 14 at J5/J6 | Torque needs are <0.14 Nm; weight dominates → lightest possible |
| Counterweight on all cycloidal drives | Eliminates eccentric imbalance force at all speeds (static balance) |
| Twin-disk design at J1/J2 | Cancels torque ripple and axial thrust; doubles load capacity |
| S-curve velocity profiles (software) | Avoids exciting ~18.6 Hz PETG link resonance during acceleration ramps |
| Mean Well LRS-350-24 PSU | Covers 13.8A theoretical max with margin |
| PETG for structure | 80°C Tg, excellent layer adhesion, validated heat-set shear strength |
| 1/32 microstepping (J3/J4) | Further reduces per-step torque impulse → lower vibration excitation at mid-arm joints |
