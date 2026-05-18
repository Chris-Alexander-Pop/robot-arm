# Simulink Workflow — Model-Based Design & Embedded Codegen

<!--
This document defines how Simulink is used on this project: what it produces,
how those outputs cross the boundary into firmware, and the conventions that
keep the model / firmware / URDF in sync.
-->

## 1. Role

Simulink is the **model-based design** path for joint-level control. It complements rather than replaces the other three workspaces:

| Workspace | Role | Primary deliverable |
|:--|:--|:--|
| `firmware/` (PlatformIO, C++) | Real-time execution on STM32 | Compiled binary running at ≥ 1 kHz |
| `software/` (ROS 2, MoveIt, Python) | High-level planning + application logic | `print_tending_node`, MoveIt config |
| `simulation/` (Python, Gazebo) | Free-space kinematic and physics simulation | URDF + Gazebo digital twin |
| **`simulink/` (MATLAB, Simulink, Simscape, Embedded Coder)** | **Plant modeling + controller design + C code generation** | **Generated C source for the joint PID, validated against a plant model** |

Specifically, Simulink owns:

1. **Plant modeling** — Simscape Multibody model of each joint's electromechanical dynamics, used for offline controller design and design-time torque analysis.
2. **Controller design + tuning** — PID (and optional gravity-comp / feed-forward) designed against the plant model using PID Tuner.
3. **Embedded C generation** — Embedded Coder produces production-quality C source for the controller, which is linked into the firmware build.
4. *(Stretch)* **ROS 2 co-simulation** — ROS Toolbox bridges Simulink plant model to ROS 2 topics so controller iteration does not require Gazebo.

---

## 2. Why model-based design at all?

Hand-tuning a PID on the real arm is slow (motor cooldown between trials), risky (overshoot can damage 3D-printed structure), and unrepeatable (gear backlash and friction drift). Designing against a Simscape plant model gives a fast, safe, repeatable loop. Embedded Coder then carries the *exact* tuned controller — bit-for-bit — to the STM32, eliminating the "tuned in MATLAB, retyped into C" error class.

This is also the workflow used at automotive / aerospace OEMs (Bosch, Tesla, Lockheed, etc.); learning it on a personal project is high-signal for those teams.

---

## 3. Boundaries

### 3a. Simulink → Firmware
- Embedded Coder generates C source into `simulink/codegen/output/` (committed to the repo).
- The generated source is copied/symlinked into `firmware/stm32_core/lib/control/generated/`.
- A hand-written adapter at `firmware/stm32_core/lib/control/src/joint_controller_generated.cpp` wires the generated function into the existing `JointController::Step` hook.
- The firmware build (`pio run`) does **not** require MATLAB. Only regenerating the controller code does.

### 3b. URDF / parameters
- The URDF under `software/ros2_ws/src/robot_description/` is the single source of truth for arm geometry and inertia.
- Simulink imports it via `smimport` (see `simulink/scripts/load_urdf.m`); the imported model is hand-augmented with motor and gearbox dynamics.
- Joint parameters (motor constants, ratios, friction coefficients, controller gains) live in `simulink/scripts/joint_params.mat`. Changes to the URDF or `docs/Constraints.md` require a paired update to `joint_params.mat` and a controller re-tune.

### 3c. Constraint contract
The generated code must satisfy the hard constraints in `docs/Constraints.md`:

| Constraint | Implication for Simulink |
|:--|:--|
| §3a — Loop rate ≥ 1 kHz | Solver: `Fixed-step, Discrete`, step size = 1 ms. No continuous-time blocks. |
| §3a — Step pulses are hardware-timer-based | Simulink computes the velocity setpoint; the STM32 timer peripheral generates the pulses. The controller's output is a *velocity command*, not raw pulses. |
| §3d — Joint angle limits enforced in firmware | Controller saturation limits are configured but the firmware adapter applies the absolute limit-clamp as a defense-in-depth layer. |
| §5b — Microstepping ≥ 1/32 for J3/J4 | Plant model accounts for the resulting per-step torque granularity when validating vibration. |

---

## 4. Mapping to delivery checkpoints

Per [`../Application.md §3`](../Application.md#3-staged-delivery-plan), Simulink work is sequenced so it never blocks a demo:

| Checkpoint | Simulink milestone |
|:--|:--|
| **A — It moves** | Hand-coded PID in firmware; no Simulink dependency. Establishes the baseline. |
| **B — It moves in sim** | URDF imported into Simscape Multibody to validate import workflow; plant model for J1 built. |
| **C — It plans** | Controller for J1 designed against the plant, tuned with PID Tuner, code-generated, integrated into firmware; sim-vs-real-vs-Simulink three-way comparison run. **This is the resume-headline Simulink milestone.** |
| **D — It sees** | No Simulink-specific work. |
| **E — It tends** | All-joint controllers regenerated; gravity-comp feed-forward added to J2 to handle full-extension payload during pick. |
| **F — Sim predicted it** *(stretch)* | ROS Toolbox co-simulation: Simulink plant model substitutes for Gazebo during controller iteration. |

---

## 5. Why three simulators?

Each simulator answers a different question:

| Simulator | Question |
|:--|:--|
| **Simscape Multibody (Simulink)** | "Does this controller drive *one joint* correctly given motor + gearbox dynamics?" |
| **Gazebo (ROS 2)** | "Does the *whole arm* execute MoveIt trajectories without collision or kinematic surprise?" |
| **Python FK/IK (simulation/)** | "Do my DH parameters and kinematic transforms agree with first principles?" |

They are layered: Python is fastest to iterate but lowest fidelity; Simscape is highest fidelity for control design; Gazebo is the system-level integration sandbox. Each catches a different class of bug, and disagreement between any two of them is a flag to investigate.

---

## 6. Anti-patterns to avoid

- **Continuous-time blocks in controllers.** Embedded Coder will either refuse or generate code that misbehaves at the 1 kHz boundary. Always use discrete-time equivalents.
- **Hard-coded gains in the model.** Use workspace variables so they can be tuned without touching the model XML, and so the diff in `git` after a re-tune is a single `joint_params.mat` change rather than a noisy `.slx` diff.
- **Editing the generated C by hand.** Always overwritten on regeneration. If a fix is needed, fix it in the model; if the integration boundary is wrong, fix the adapter.
- **Letting URDF and Simulink plant drift.** Always re-import via `smimport` after a URDF change; never edit imported Simscape Multibody geometry by hand.
- **Designing controllers Simulink can simulate but Embedded Coder can't generate.** Check the Code Generation Advisor before tuning — discover blocker blocks early.
