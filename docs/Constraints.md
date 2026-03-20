<!--
This file contains all the constraints we have defined regarding the project.
Constraints are hard limits (things that cannot be violated) or soft targets (design goals that
should be achieved but could be relaxed if engineering trade-offs demand it).
-->

# 6-DOF Robot Arm — Project Constraints

This document catalogs the constraints that govern design decisions across all subsystems. Constraints are grouped by type and labeled as either **HARD** (non-negotiable, violating this breaks the system) or **SOFT** (a design target that may be relaxed with justification).

For the calculations behind these limits, see [`Calculations.md`](./Calculations.md). For the design decisions that respond to these constraints, see [`Design_Choices.md`](./Design_Choices.md).

---

## 1. Mechanical Constraints

### 1a. Torque — HARD

| Joint | Required Holding Torque (worst-case) | Motor Native Torque | Required Gear Ratio | Chosen Ratio | Safety Margin |
|:---:|:---:|:---:|:---:|:---:|:---:|
| J1 (Base) | ~7.95 Nm | 2.0 Nm (NEMA 23) | ≥ 4× (static), ≥ 10× (dynamic) | 20:1 cycloidal | **4.0×** |
| J2 (Shoulder) | ~7.95 Nm | 2.0 Nm (NEMA 23) | ≥ 10× (dynamic) | 20:1 cycloidal | **4.0×** |
| J3 (Elbow) | ~3.27 Nm | 0.80 Nm (NEMA 17) | ≥ 4.1× | **15:1 cycloidal** | **3.67×** |
| J4 (Forearm) | ~0.015 Nm (inertial) | 0.42 Nm (NEMA 17) | ≥ 1× | **10:1 cycloidal** | **280×** |
| J5/J6 (Wrist) | < 0.15 Nm | 0.14 Nm (NEMA 14) | ~1× | Direct | Small margin |

- **Motor shafts must NEVER carry structural (axial/radial bending) loads** — all structural forces pass through bearings. Violating this will damage or destroy motor shaft bearings.
- **Cycloidal drives require PTFE grease** — unlubricated operation will melt the PETG disk via friction heat within minutes of sustained use.

### 1b. Payload — SOFT

- **Target**: 0.5 – 1.0 kg at the end-effector, arm fully extended
- **Limit**: > 1.5 kg approaches the safe limit of J2 (40 Nm effective with 4.0× margin). J3 (12 Nm, 3.67× margin at ~3.27 Nm requirement) becomes the binding structural limit above ~1.5 kg.
- Payload targets will be verified via physical load testing in Phase 1

### 1c. Reach — SOFT

- **Target**: ~630mm shoulder-to-wrist (fully extended)
- Derived from link length estimates: L_upper ≈ 280mm, L_forearm ≈ 250mm, L_wrist ≈ 100mm
- Final values locked in when CAD dimensions are confirmed

### 1d. Repeatability — SOFT

- **Target**: ±0.5 – 1.0 mm at the end-effector
- **Theoretical step resolution**: ~62 µm at full extension (J1 microstepping; see `Calculations.md §3b`)
- **Practical limit**: Set by PETG print tolerances (~0.2mm) and cycloidal play (~0.1°). Belt-stretch is no longer a factor as all four main joints now use cycloidal drives.
- A repeatability better than ±0.3 mm is not achievable without replacing PETG structure with aluminum and cycloidal drives with harmonic drives

### 1e. Temperature — HARD

- **PETG glass transition temperature: ~80°C**. Motor mounting surfaces and structural joints must not exceed this continuously.
- NEMA 23 motors running at 4A can surface-heat to 60–70°C. Adequate thermal standoff (air gap, non-continuous duty cycle) must be maintained between motor bodies and PETG housings.
- **Do not use PLA** anywhere in the load-bearing structure — its 60°C Tg provides zero margin.

---

## 2. Electrical Constraints

### 2a. Power Supply — HARD

- **Bus voltage**: Exactly **24V DC**. The CL57T and CL42T drivers require 20–50V. Below 20V, torque rolloff at moderate speeds is severe. Above 50V, drivers are damaged.
- **Maximum continuous current**: **14.6A** (Mean Well LRS-350-24 rated output). The theoretical simultaneous peak draw of all 6 motors is 13.8A — within limit, but only barely.
- **Software motion constraint**: J1 and J2 must **never** be commanded to full-current simultaneously during normal operation. The motion planner must ensure sequential or reduced-current operation to stay below 14.6A.
- **Logic power isolation**: Logic 5V supply (Raspberry Pi, STM32) must be derived from a **separate buck converter** output, not tapped from a motor driver's logic supply. Motor switching noise must not contaminate the logic rail.

### 2b. Logic Level — HARD

- **STM32 I/O**: 3.3V logic levels
- **CL57T / CL42T industrial drivers**: Require 5V logic on STEP/DIR inputs
- A **74HC245 level shifter is mandatory** in the signal path between the STM32 and the industrial drivers. Feeding 3.3V directly to a driver expecting 5V will result in unreliable triggering or complete failure to step.
- **TMC2209** (NEMA 14 wrist drivers): Accept 3.3V logic natively — no level shifting required

### 2c. I2C Addressing — HARD

- All **AS5600 encoder ICs** share the fixed I2C address `0x36` (non-configurable in hardware)
- A **TCA9548A I2C multiplexer** (address `0x70`) is **mandatory** to address multiple AS5600s from a single STM32 I2C bus
- Maximum encoders per bus without additional muxing: **8** (TCA9548A has 8 channels) — sufficient for 6 joints

### 2d. Wiring — HARD

- Motor phase wires (NEMA 23): minimum **18 AWG silicone** — continuous 4A, silicone jacket for heat tolerance and flex life
- Logic/encoder signal wires: **22 AWG shielded** — unshielded signal wires routed near motor phase wires will pick up PWM switching noise and corrupt I2C encoder reads
- All wires entering screw-terminal blocks (PSU, drivers): must be **ferrule-crimped** — bare stranded wire cold-flows under the screw and can loosen, causing intermittent high-resistance connections under vibration
- The base rotation joint (J1) cable pass-through must use a **drag chain** — unsupported cables will fatigue and break within hundreds of rotation cycles

### 2e. Safety — HARD

- **E-Stop is a hardware interlock**: The E-Stop button must cut AC mains power to the PSU directly. A software-only E-Stop is insufficient — if the STM32 or Pi crashes, software E-Stop is ineffective.
- **Fused AC inlet**: The IEC C14 panel connector must include a fuse sized to the PSU's input current rating
- The CL57T ALARM output pin (active low on fault) **must** be wired to a STM32 interrupt GPIO — unmonitored driver faults can cause a runaway motor

---

## 3. Firmware Constraints

### 3a. Control Loop Timing — HARD

- **Minimum loop rate**: ≥ **1 kHz** (1ms period) for the PID position update loop
- **Step pulse generation**: Must be hardware-timer-based (STM32 TIMx peripherals in output-compare mode). Software-generated pulses (via GPIO toggle in a loop) will have unacceptable jitter at high step rates.
- **Maximum UART latency**: The STM32 must parse and begin executing a `SET_JOINTS` command within **< 5ms** of receipt. Buffered serial with DMA is preferred.
- **Watchdog timeout**: If no valid command packet is received within **500ms**, all joints must immediately be set to zero velocity (hold position). This handles Pi crashes or cable disconnection safely.

### 3b. Communication Protocol — HARD

- All UART packets use the **binary framed format**: `[0xAA 0x55][CMD][PAYLOAD][XOR_CHECKSUM]`
- ASCII-based communication (e.g., `G-code` style text) must **not** be used — parsing overhead exceeds acceptable latency budget
- Checksum validation is mandatory; malformed or corrupted packets must be silently discarded (not acted on and not crashed on)
- **Baud rate default**: 115200. May be increased to 921600 if USB-serial is used and trajectory update rate demands it.

### 3c. Joint Angle Limits — HARD

- Each joint has a defined software angular limit (stored in `config.h`). The STM32 must clamp all setpoints to these limits and reject commands that would exceed them.
- Joint limits are derived from the physical range of motion in the CAD model. **Preliminary limits** (to be confirmed vs. CAD):

| Joint | Min (deg) | Max (deg) |
|:---:|:---:|:---:|
| J1 (Base) | -170 | +170 |
| J2 (Shoulder) | -90 | +135 |
| J3 (Elbow) | 0 | +150 |
| J4 (Forearm) | -180 | +180 |
| J5 (Wrist Pitch) | -90 | +90 |
| J6 (Tool Roll) | -180 | +180 |

> These limits are preliminary placeholders — they must be validated against the final CAD geometry before use.

### 3d. Homing — HARD

- The arm **must complete a full homing sequence** before any position commands are accepted. Attempting to move to absolute joint angles without a known zero reference will yield unpredictable positions.
- Homing must proceed from **base outward** (J1 → J2 → ... → J6) to avoid self-collision during the homing sweep.

---

## 4. Software Constraints

### 4a. Real-Time Separation — HARD

- **The Raspberry Pi must not perform any real-time motor control** (direct STEP/DIR signaling). Linux scheduling jitter makes microsecond-level timing impossible without an RTOS. All real-time control remains on the STM32.
- The Pi's role is strictly: trajectory planning, IK solving, command streaming to STM32, telemetry publishing, and user interfaces.

### 4b. ROS 2 / MoveIt — SOFT

- **ROS 2 Humble** is the target distribution (LTS, supported through May 2027). Newer distributions may work but are untested.
- MoveIt 2 must be configured with an **SRDF** defining the `"arm"` planning group (J1–J6) and the end-effector (`"gripper"`). Without this, collision checking and planning groups will not function.
- **Trajectory update rate to STM32**: MoveIt's `FollowJointTrajectory` action interpolates trajectories at a configurable rate. Target: **50–100 Hz** (10–20ms per command packet) — achievable at 115200 baud.

### 4c. Docker — SOFT

- All ROS 2 nodes run in Docker containers. Direct host-system ROS 2 installation is not standard for this project.
- The container must mount `/dev/ttyUSB0` (or equivalent STM32 serial port) from the host — this must be reflected in `docker-compose.yml` device mappings.

### 4d. URDF Accuracy — HARD

- The URDF must reflect the **actual physical link lengths and masses** determined from CAD before being used for MoveIt collision planning. An inaccurate URDF will produce collision avoidance failures (real arm hits things the planner thinks it can clear) or trajectory tracking errors.
- Link inertia tensors must be populated (from CAD) for Gazebo to simulate realistic dynamics. Gazebo will accept zero-inertia URDFs but the physics will be garbage.

---

## 5. Physical / Fabrication Constraints

### 5a. Print Tolerances — HARD

- **Cycloidal disk to ring pin clearance**: ±0.1–0.15mm of intentional clearance is required for the disk to roll freely inside the ring. Tighter and it seizes; looser and backlash increases.
- **Heat-set insert holes**: Must be sized to the insert's knurled OD (typically 4.2–4.5mm for M3×5 inserts in PETG). Standard M3 clearance holes (3.2mm) are not appropriate for heat-sets.
- **Motor mount bolt pattern**: NEMA 23 uses a 47.14mm bolt circle (4× M5); NEMA 17 uses 31mm (4× M3); NEMA 14 uses 26mm (4× M3). These patterns must be modeled exactly in CAD and printed with < 0.3mm dimensional accuracy to align the bolt holes.

### 5b. Vibration — HARD

Cycloidal drives introduce a rotating eccentric imbalance. The following constraints govern its management:

- **Counterweight disk is mandatory on all 4 cycloidal drives (J1–J4)**: A second 608ZZ bearing (or equivalent mass) must be mounted on the rear motor shaft, 180° opposite the eccentric cam, before the arm is operated. Omitting this produces repeating vibration that:
  - Fatigues PETG link structures at their resonant frequency (~18.6 Hz estimated)
  - Degrades end-effector positioning accuracy during motion
  - Loosens fasteners over time (even with Loctite)

- **Motor input speed must stay below ~800 RPM** during normal trajectories. This keeps excitation safely below the estimated 1116 RPM structural resonance of the upper arm link. MoveIt velocity scalings must enforce this limit via `joint_limits.yaml`.

- **S-curve (jerk-limited) trajectory profiles are mandatory** — do not use trapezoidal velocity profiles. Trapezoidal profiles produce instantaneous step changes in acceleration that excite the arm's resonant modes. Configure in MoveIt's trajectory parameterization settings.

- **Twin-disk cycloidal construction is required at J1 and J2**: Two disks stacked 180° out of phase eliminate torque ripple and balance axial disk-pin forces. This is in addition to, not a replacement for, the shaft counterweight.

- **Microstepping ≥ 1/32 is required for J3 and J4** (CL42T DIP switch): Reduces per-step torque impulse, lowering the amplitude of high-frequency forcing from stepper commutation.

### 5c. Center of Mass — SOFT

- The assembled arm should be **rear-heavy at the base** — the cycloidal housing and base link should be the heaviest component, keeping the center of mass close to the base.
- If the forearm and wrist assembly is significantly heavier than expected, J2 torque requirements will increase beyond the current margin.

### 5c. Cable Routing — HARD

- All 6 motor cable bundles must be routed through the arm structure before final assembly — cables cannot be routed externally along the arm body (they will snag during operation, create moment arms, and fatigue).
- The base rotation joint (J1) must have ≥ 340° of unrestricted cable twist. A cable loop or swivel at the base is required.

---

## 6. Financial Constraints

### 6a. Phase 1 Budget — HARD

- **Cap**: ~**$400 CAD** for the Phase 1 prototyping bill of materials
- This buys: 1× NEMA 23 kit, 1× NEMA 17 kit, 1× NEMA 14, 1× TMC2209 pack, PSU, bearings, cycloidal pins, belts, fasteners, and basic electrical components
- Phase 1 deliberately excludes: second NEMA 23, second NEMA 17, wrist motors (2nd), servo/FSR, wire sleeving, aviation connectors, permanent perfboards

### 6b. Phase 2 Budget — SOFT

- **Target**: ~**$700–800 CAD** total (Phase 1 + incremental Phase 2 components)
- Phase 2 adds: second NEMA 23 kit, complete wiring with sleeving/connectors, GX16 aviation connectors, protoboard hat, MG996R gripper, optional FSR

### 6c. Cost-Reduction Priorities

Components where cost has been deliberately minimized vs. professional alternatives:
| Component | Chosen (Cost) | Professional Alternative (Cost) | Savings |
|:---|:---:|:---:|:---:|
| J1/J2 gearboxes | Cycloidal PETG + pins (~$15 each) | Planetary gearbox (~$120 each) | ~$210 |
| J3 gearbox | Cycloidal PETG + pins (~$15) | Planetary gearbox (~$80) | ~$65 |
| J4 gearbox | Cycloidal PETG + pins (~$10) | Planetary gearbox (~$60) | ~$50 |
| J1–J4 position sensing | Built into CL57T/CL42T kit | Separate encoder + open-loop driver | N/A (kit is better) |
| J5/J6 motors | NEMA 14 pancake (~$13 each) | Dynamixel XL430 (~$50 each) | ~$74 |
| Structural material | PETG (~$21/kg) | Aluminum extrusion + machined hubs | ~$200+ |