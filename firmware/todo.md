# Firmware Roadmap

This document outlines the low-level real-time control code for the STM32 microcontroller.

The backlog is organized around the **staged delivery checkpoints A–F** defined in [`docs/Application.md`](../docs/Application.md#3-staged-delivery-plan). Each checkpoint lists the firmware work required to unlock it. Items marked *Phase 1* are needed before any real-arm demo; *Phase 2* items are unlocked once the full 6-axis hardware is built.

**Learning path**: Tier 1 exercise stubs (`JointController::Step`, `StepperDriver` under `stm32_core/lib/`) and the fuller product backlog described below intentionally overlap — the boxes here are broader themes (architecture, tooling, ROS), while onboarding steps live in **[`firmware/CONTRIBUTING.md`](CONTRIBUTING.md)** with concrete filenames.

---

## Checkpoint A — "It moves" *(Phase 1, J1 + J2 only)*

*Exit criterion: STM32 drives J1 and J2 to commanded positions via simple serial commands; encoder/driver feedback reports back actual angle.*

### A.1 Pre-hardware / analysis
- [ ] **Electrical Interface Map**: Define pinouts, logic levels, connector assignments, and fault-line behavior for J1 + J2.
- [ ] **Current Budget Check**: Verify peak, average, and startup current against the PSU and logic buck converters.
- [ ] **Fault-State Matrix**: Specify firmware responses for E-stop, driver alarm, serial timeout, encoder failure, and I2C bus hang.
- [ ] **Protocol Mocking**: Add host-side tests or stubs that validate packet framing and command parsing without real hardware.

### A.2 Core firmware (J1 + J2)
- [ ] **Serial Protocol**: Define and implement the binary UART/USB command protocol (`[0xAA 0x55][CMD][PAYLOAD][XOR]`) between RPi and STM32.
- [ ] **Stepper Core**: Implement precise timer-based pulse generation for CL57T drivers on J1 and J2.
- [ ] **State Machine**: Implement IDLE / MOVING / HOMING / ERROR states for the 2-DOF subset.
- [ ] **Driver Validation**: Confirm STM32 reliably drives both NEMA 23 (via CL57T) on J1 and J2 at varying speeds, with the 74HC245 level shifter in the loop.
- [ ] **Homing Logic**: Draft homing routines using Hall sensors or physical hard-stops for J1 + J2.

---

## Checkpoint B — "It moves in sim" *(no firmware-side work)*

This checkpoint is purely software/URDF — firmware remains at Checkpoint A. Listed here only so the cross-reference is explicit.

---

## Checkpoint C — "It plans" *(Phase 1)*

*Exit criterion: MoveIt-planned trajectory streams to the STM32 and executes on J1 + J2 with < 1° per-joint error vs. the Gazebo dry-run; the same trajectory also runs in Simscape against the per-joint Simulink controller with three-way agreement (Gazebo + Simscape + real) within ±5° per joint.*

- [ ] **`FollowJointTrajectory` ingestion**: STM32 accepts and interpolates trajectory waypoints at the 50–100 Hz update rate that MoveIt streams (see `docs/Constraints.md §4b`).
- [ ] **Supervisory PID Position Loop (hand-coded baseline)**: Implement on top of the CL57T's internal closed loop; corrects integrated position error reported back to the Pi. This is the Checkpoint A controller and stays as the fallback path.
- [ ] **Generated-Controller Adapter (`joint_controller_generated.cpp`)**: Wraps the Embedded-Coder-generated joint PID functions (from `simulink/codegen/output/`) into the firmware's `JointController::Step` hook. See [`docs/implementation/simulink_workflow.md §3a`](../docs/implementation/simulink_workflow.md). Must compile cleanly **without** MATLAB on the build host.
- [ ] **Codegen Integration Smoke Test**: Run the generated controller against a unit-test fixture and assert it agrees bit-for-bit with the Simulink reference output (Software-in-the-Loop equivalence, `Application.md §2c M7`).
- [ ] **Per-trajectory Safety Envelope**: Reject incoming setpoints that violate joint angle limits (`docs/Constraints.md §3d`) — must fail safely, not clip silently. Adapter re-applies the clamp on controller output as defense-in-depth (`docs/Constraints.md §3c`).
- [ ] **Telemetry Stream**: Publish per-joint actual angle + driver fault status back to the Pi at ≥ 50 Hz for sim-to-real comparison (`Application.md §2c M4`).

---

## Checkpoint D — "It sees" *(Phase 2; vision is host-side, firmware only needs to be ready)*

*Exit criterion: Firmware reliably executes pick / place trajectories handed down by the vision-driven task node.*

- [ ] **Gripper Channel**: Add a `SET_GRIPPER` command path (servo PWM for MG996R, or solenoid GPIO for the suction variant).
- [ ] **Move-Then-Settle Reporting**: STM32 emits a `MOVE_COMPLETE` packet only when all joints are within tolerance and stationary — required by the pick step to avoid grasping while moving.

---

## Checkpoint E — "It tends" *(Phase 2)*

*Exit criterion: STM32 supports the full print-tending cycle without firmware-side intervention.*

- [ ] **Multi-Axis Sync**: Synchronize movement across all 6 joints for smooth trajectories (Bresenham's or equivalent multi-axis interpolation).
- [ ] **Global Error Handling**: Cross-axis safety — if any joint stalls or faults, halt all joints and emit `FAULT`.
- [ ] **Safety Interlocks**: Hardware E-Stop interrupt handler (must override software state); current sensing on the 24V bus.
- [ ] **EEPROM Storage**: Save calibration offsets, soft limits, and PID tunings to non-volatile memory for all 6 axes.
- [ ] **Tuning & Optimization**: Re-tune PID loops in Simulink against the updated all-joint plant models; regenerate via Embedded Coder; the firmware only consumes the regenerated source (no hand-tuned gain edits in firmware).
- [ ] **Gravity-Compensation Feed-Forward (J2)**: Integrate the Simulink-designed feed-forward path for J2 to handle full-extension payload during the pick step.
- [ ] **Communication Stress Test**: Validate high-speed command streaming during a full tending cycle without dropped packets.

---

## Checkpoint F — "Sim predicted it" *(Phase 2 stretch; firmware support only)*

- [ ] **Timestamped Telemetry**: Pair every reported joint angle with its STM32 timestamp so the sim-vs-real comparison plot has unambiguous time alignment.
