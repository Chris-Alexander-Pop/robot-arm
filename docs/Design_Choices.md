<!--
This file holds the design choices made for this project and their engineering reasoning.
Decisions are recorded here so the rationale is preserved across development phases.
-->

# Design Choices & Engineering Rationale

This document records the key design decisions for the 6-DOF robot arm, focusing on **why** each choice was made over the alternatives. Decisions are organized by subsystem.

---

## 1. Mechanical & Actuation

### 1a. Cycloidal Drives at J1–J4

**Decision**: Use **3D-printable cycloidal drives** (PETG disk + hardened steel dowel ring pins) at all four driven joints (J1/J2 at 20:1, J3 at 15:1, J4 at 10:1).

**Alternatives Considered**:

| Option | Pros | Cons | Chosen? |
|:---|:---|:---|:---:|
| Planetary Gearbox (metal, off-shelf) | Very robust, no printing | Expensive (~$80–$150 per joint), heavy | ✗ |
| Harmonic Drive | Near-zero backlash, compact | Extremely expensive (~$300+) | ✗ |
| Worm Gear (3D printed) | Self-locking, cheap | High friction, low efficiency, no backdrivability | ✗ |
| Timing Belt only (J3) | Cheap, no extra housing | 3:1 belt = 2.4 Nm — **below the 3.27 Nm requirement** | ✗ |
| **Cycloidal Drive (3D printed + steel pins)** | Printable, 10–20:1 in one stage, high torque density, near-zero backlash, uniform BOM | Requires PTFE grease, careful tolerances, eccentric vibration must be managed | **✓** |

**Rationale**: The cycloidal drive achieves a high ratio in a package smaller than a fist. The ring gear pins are hardened steel M4 dowels — these cannot shear; only the PETG disk is consumable. Extending cycloidals to J3 and J4 resolves the torque shortfall (see `Calculations.md §1e`) and **unifies the entire drive system**: the same dowel pins, the same print profiles, and the same assembly procedure across all four joints, simplifying fabrication and spares.

---

### 1b. NEMA 23 for J1 & J2, Not Larger (e.g. NEMA 34)

**Decision**: Use the **2.0 Nm NEMA 23** closed-loop kits as the base/shoulder motors.

**Rationale**: After gearing, the effective output torque at J2 is 2.0 Nm × 20 = **40 Nm** — sufficient for the 0.5–1.0 kg payload target with a 5× safety factor. Going to NEMA 34 would provide more torque but:
- Larger, heavier, physically larger envelope (harder to integrate into the cycloidal housing)
- Far more expensive
- Drivers would need to operate at 5–8A, requiring a larger PSU
- The 2.0 Nm motor with 20:1 gearing is already over-torqued relative to the PETG structural strength limit

---

### 1c. NEMA 14 "Pancake" Motors for J5 & J6 (Wrist/Tool)

**Decision**: Use the **14HS10-0404S** NEMA 14, 26mm-long pancake steppers for joints 5 and 6.

**Rationale**: At the wrist, every gram of motor mass multiplies into additional required torque at J2 (lever arm effect). A NEMA 17 at the wrist (280g) would add ~1.5 Nm to the J2 load. The 14HS10-0404S weighs **130g** — half the NEMA 17 — and the wrist torque requirement is only ~0.15 Nm (well within its 0.14 Nm rating at low speeds, augmented by any small gearing if needed).

The wrist joints do not need the high torque of a closed-loop system, so the open-loop TMC2209 driver is sufficient. The TMC2209's **StealthChop** mode eliminates audible stepping noise, which would otherwise be very noticeable at the wrist where motor mounts are near the user.

---

### 1d. Cycloidal Drive at J3 (Elbow) — 15:1

**Decision**: Replace the originally considered GT2 belt drive at J3 with a **15:1 cycloidal drive** on the NEMA 17 (80 Ncm) motor.

**Why not belt?**
The worst-case torque at J3 is **3.27 Nm**. A 3:1 GT2 belt drive (the practical maximum before pulleys become geometrically oversized) yields only `0.80 Nm × 3 = 2.40 Nm` — a shortfall of ~27%. Increasing to 4:1 is possible but results in a 64-tooth driven pulley that is physically larger than the NEMA 17 motor body, making packaging very difficult inside the upper arm link.

**Why 15:1 specifically?**
- Yields `0.80 × 15 = 12.0 Nm` — a 3.67× safety factor over the 3.27 Nm requirement
- 15 pins is a smaller, lighter housing than the 20-pin J1/J2 design, appropriate for the NEMA 17
- 15:1 keeps motor input speed at a moderate RPM for typical joint velocities (below the estimated 1116 RPM structural resonance threshold)

**Trade-off accepted**: The cycloidal housing adds ~50–80g to Link 1 (upper arm), slightly increasing J2 load. At the 20:1 reduction on J2, the 40 Nm effective output absorbs this easily.

---

### 1e. Cycloidal Drive at J4 (Forearm Twist) — 10:1

**Decision**: Use a **10:1 cycloidal drive** on the NEMA 17 (42 Ncm) motor at J4.

**Is a gearbox even necessary at J4?** The gravity torque at J4 is near-zero (the rotation axis is roughly parallel to the forearm, so the distal mass creates no gravitational moment). However, a cycloidal drive at J4 is still chosen for three reasons:

1. **Resolution**: Direct drive at 1/32 microstepping gives 0.056°/step. Through a 10:1 cycloidal, that becomes 0.0056°/step — 10× finer, enabling precise tool-roll positioning for orientation-sensitive pick-and-place.
2. **Inertial damping**: The gear reduction lowers effective reflected inertia at the output, reducing oscillation after a direction reversal — important for a lightweight wrist assembly that has little natural damping.
3. **Manufacturing uniformity**: Using the same ring pins, grease, and assembly procedure as J1/J2/J3 reduces the number of unique part types in the BOM.

**Ratio choice**: 10:1 (10 pins, 9-lobe disk) is the smallest practical cycloidal ratio. Smaller ratios have poor conjugate tooth geometry. This is the minimum necessary to gain the resolution and damping benefits without adding unnecessary housing mass.

---

### 1f. Vibration Mitigation Strategy

**Decision**: Implement a tiered vibration mitigation strategy across all cycloidal drives.

**Why is vibration a concern?** Each cycloidal drive's eccentric cam (a 608ZZ bearing offset ~1mm from the motor shaft centreline) creates a rotating centrifugal imbalance force $F = m \cdot e \cdot \omega^2$. At 600 RPM motor input, this is ~79 mN per drive. With 4 drives on the arm, unmitigated vibration accumulates and is amplified at the end-effector.

**Mitigation decisions (in priority order):**

| Measure | Implementation | Cost | Joints |
|:---|:---|:---:|:---|
| **Counterweight disk** | Second 608ZZ at 180° on rear motor shaft | ~$2/joint | J1–J4 (mandatory) |
| **Twin-disk cycloidal** | Two disks 180° out of phase; cancels torque ripple | Doubles disk print time | J1, J2 (high duty cycle) |
| **S-curve trajectories** | MoveIt 2 jerk-limited profiles (`joint_limits.yaml`) | Software only | All joints |
| **1/32 microstepping** | CL42T DIP switch (J3/J4); CL57T already supports | No cost | J3, J4 |
| **PTFE grease** | Standard lubrication requirement | Already in BOM | J1–J4 |

**Rationale**: The counterweight is the most impactful and cheapest fix — it statically balances the rotating assembly and eliminates the imbalance force at **all speeds**. The twin-disk addition at J1/J2 addresses torque ripple, which is a secondary vibration source distinct from the imbalance. S-curves prevent the firmware from exciting the estimated 18.6 Hz PETG link resonance during acceleration phases. Together, these measures should reduce vibration to a level imperceptible during normal pick-and-place.

---

### 1g. PETG over PLA or ABS

**Decision**: Use **PETG filament** as the primary structural material.

**Rationale**:
- **vs. PLA**: PLA has a glass transition temperature of ~60°C. A stepper motor running at 50% duty cycle in an enclosure can heat motor mounts to 55–70°C, causing PLA to soften and deform under load. PETG's 80°C Tg provides safe headroom.
- **vs. ABS**: ABS warps severely during printing (requires enclosure, specific bed temperatures) and releases unpleasant fumes. PETG prints reliably on an open printer with a PEI-coated bed.
- **vs. ASA/Nylon**: Stronger materials, but require specialized hardware; PETG is optimal for the shop-printable constraint.

---

## 2. Electronics & Control

### 2a. Closed-Loop Stepper Kits (CL57T / CL42T) over Open-Loop

**Decision**: Use **integrated closed-loop stepper kits** for J1–J4 instead of open-loop drivers (TB6600, A4988, etc.).

**Alternatives Considered**:

| Option | Pros | Cons |
|:---|:---|:---|
| Open-loop steppers | Cheap, simple | Skip steps under load → arm loses position without knowing |
| Open-loop + separate AS5600 encoder | Position monitoring | STM32 must implement full correction loop; complexity |
| **Closed-loop kit (CL57T/CL42T)** | Encoder+correction built into driver IC | Costs ~2–3× more than an open-loop driver |
| Servo motors (Dynamixel) | Very capable, compact | Expensive ($50–$200 per joint), proprietary protocol |

**Rationale**: The CL57T and CL42T drivers perform closed-loop correction **within the driver itself** — if the motor skips a step, the driver detects the encoder mismatch and corrects it without any STM32 involvement. This dramatically simplifies firmware and eliminates missed-step failures, which are the #1 reliability problem with open-loop stepper arms. The cost premium over TB6600 drivers is ~$40 per joint — well worth it for a reliable, repeatable arm.

---

### 2b. STM32 + Raspberry Pi Hierarchy over Single-Board Solution

**Decision**: Use a **two-tier compute architecture** (STM32 for real-time, RPi 4 for planning) rather than trying to do everything on one device.

**Why not Raspberry Pi alone?**
Linux is not a real-time OS. Even with `PREEMPT_RT` patches, the Pi's scheduler can introduce millisecond-scale jitter in motor control loops. Stepper motors require microsecond-precise pulse timing. A 1ms timing jitter causes audible vibration and velocity ripple.

**Why not Arduino/ESP32 alone?**
An Arduino Mega or ESP32 can generate precise step pulses, but has insufficient RAM and compute to run ROS 2, trajectory planning, or IK solving. The ESP32's RTOS is capable, but the software ecosystem (ROS 2, MoveIt) is built for Linux.

**Conclusion**: The STM32 handles microsecond-level timing in hardware (no OS, hardware timers), while the Pi handles the math-heavy planning layer in software. This is the same architecture used by professional robot arms (e.g., the UR5 uses a dedicated FPGA + a PC for this split).

---

### 2c. 74HC245 Logic Level Shifter (Not Resistor Divider)

**Decision**: Use the **74HC245 octal bus transceiver** for 3.3V → 5V level shifting, not a simple resistor voltage divider.

**Rationale**: A resistor divider (e.g., 1kΩ/2kΩ) works for **downward** shifting (5V → 3.3V). For upward shifting (3.3V → 5V), a resistor divider cannot amplify voltage — you need an active buffer. The 74HC245 has a propagation delay of ~8ns, well within the STM32's minimum STEP pulse width. It handles all 8 STEP/DIR signals simultaneously for just ~$3.

---

### 2d. Mean Well LRS-350-24 PSU (Not 24V Laptop Adapter)

**Decision**: Use the **Mean Well LRS-350-24** industrial enclosed switching PSU, not a commodity 24V adapter.

**Rationale**:
- Laptop-style 24V adapters typically max out at 3–5A — insufficient for the 13.8A peak requirement
- The LRS series has proper **EMI filtering** — critical for preventing motor switching noise from propagating back into the logic supply and corrupting I2C encoder reads
- The **IEC C14 panel connector** allows a proper fused inlet with an E-stop in the AC line — critical for safe operation
- Mean Well has published MTBF figures and certifications; unbranded adapters do not

---

## 3. Software Architecture

### 3a. ROS 2 (Humble) over Custom Communication Protocol

**Decision**: Use **ROS 2 with MoveIt 2** for all high-level control, rather than writing a custom control stack.

**Rationale**: The ROS 2 ecosystem provides:
- **MoveIt 2**: Battle-tested motion planning with OMPL, collision avoidance, and IK solvers
- **`ros2_control`**: A standardized hardware abstraction layer — swap simulation/real hardware by changing one parameter
- **`robot_state_publisher`**: Automatic URDF-based transform tree for RViz and collision checking
- **Gazebo integration**: The simulation stack plugs directly into the control stack with no code changes

Writing a custom equivalent would take months and would lack the testing coverage of industry-standard packages.

**Trade-off**: ROS 2 on a Raspberry Pi remains computationally heavy. Running in Docker adds overhead. The **Pi 4** improves headroom over older Pi generations for MoveIt / planning; if latency becomes an issue, migrating to a Pi 5 or a small x86 NUC is straightforward — the ROS 2 code is identical.

---

### 3b. Docker for ROS 2 Deployment

**Decision**: Run all ROS 2 nodes in **Docker containers** managed by `docker-compose`.

**Rationale**:
- ROS 2 Humble requires Ubuntu 22.04. The Pi may run a different OS version for other tasks — Docker keeps the ROS environment isolated.
- The `docker-compose.yml` file is a single source of truth for the entire software environment, making setup reproducible from a fresh SD card in minutes.
- Container volumes mount the `/software/` directory, so code changes are reflected immediately without rebuilding the image.

---

## 4. Decisions Deferred (TBD)

| Decision | Options | Blocking Factor |
|:---|:---|:---|
| Wrist encoder (AS5600 on J5/J6) | Add or skip | Test open-loop accuracy first; add if drift is a problem |
| Collision avoidance method | OctoMap (camera) vs. static scene | Camera integration TBD |
| User interface | Web dashboard vs. RViz markers vs. joystick | Depends on final workflow preference |
| Gripper type | Parallel jaw vs. vacuum vs. magnetic | Depends on target objects for pick-and-place |
| J4 ratio refinement | Stay at 10:1 or reduce to 8:1 | Depends on measured CAD mass — smaller ratio = lighter housing |