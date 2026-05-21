<!--
This file defines the target application(s) the robot arm is designed and built for.
It is the single source of truth for application-level requirements. Subsystem documents
(Scope, Constraints, todos, architecture) reference this file rather than restating it.
-->

# Application & End Use

The 6-DOF arm targets a **headline application** plus a **method** that wraps around it. Both were chosen to be (a) achievable within the existing mechanical / electrical envelope, (b) signal-dense for robotics / mechatronics / industrial automation roles, and (c) mutually reinforcing — the method validates the application, the application gives the method something concrete to do.

---

## 1. Headline Application — Autonomous 3D-Print Bed Tending

The arm sits next to one or more FDM 3D printers, waits for a print job to finish, removes the completed part from the bed, drops it in a parts bin, and signals the printer to start the next queued job. The goal is **hands-off operation of the same printers that are producing the arm's own structural parts** — closing the design loop (the arm builds the arm).

### 1a. Why this application
- **On-brand with the build.** The project is already 3D-print-intensive; the application emerged from a real workflow pain.
- **Lives inside the mechanical envelope.** Print tending is mostly free-space motion plus a single top-down grasp — no contact-rich manipulation, no precision assembly. The sim-to-real transfer gap is small in this regime.
- **Industrial framing.** This is a real product category (Formic, Prusa Pro AFS, 3DQue Quinly) — interview-friendly.
- **Forgiving precision floor.** A ±2 mm placement tolerance is achievable with FDM-printed cycloidal joints; tighter precision applications (e.g., PCB assembly) would not be.

### 1b. Functional requirements
| # | Requirement | Type |
|:--|:--|:--|
| F1 | Detect "print complete" event for one or more managed printers | HARD |
| F2 | Locate the finished part on the print bed within ±5 mm | HARD |
| F3 | Lift the part off the bed and place it in a designated bin | HARD |
| F4 | Issue a "start next job" command to the printer queue | HARD |
| F5 | Recover gracefully from missed picks (retry, then fault and notify) | SOFT |
| F6 | Operate adjacent to a heated print bed (up to ~60 °C ambient near the arm base, ~80 °C bed surface during cool-down) without thermal damage to PETG structure | HARD |
| F7 | End-to-end cycle time < 60 s from print-complete signal to next job start | SOFT |

### 1c. Derived hardware requirements
These flow from the functional requirements above and are referenced by [`Constraints.md`](Constraints.md):

| Spec | Value | Driver |
|:--|:--|:--|
| Reach (over print bed) | ≥ 300 mm horizontal, ≥ 250 mm vertical | F2, F3 — must cover a typical 220×220 mm bed plus a 100 mm bin offset |
| Payload | ≥ 200 g at full extension | F3 — covers most hobbyist parts; small structural parts for this project are < 100 g |
| Placement repeatability | ±2 mm | F2 — part-bed registration tolerance, not assembly tolerance |
| End-effector | Single quick-swap mount, default parallel-jaw gripper (servo-driven). Suction variant for flat-topped parts. | F3 |
| Thermal margin at base | Continuous operation at 40 °C ambient, transient survival at 60 °C | F6 — PETG Tg is 80 °C, see `Constraints.md §1e` |
| Safety | Soft E-stop on missed-pick fault (F5); hardware E-stop independent of software (see `Constraints.md §2e`) | F5 |

### 1d. Explicit non-goals
To keep scope honest:
- **Not** a generalized "any-print, any-bed" tender. v1 assumes a known bed pose, a known parts bin pose, and one printer.
- **Not** vision-based "is the print done" classification. v1 uses OctoPrint's HTTP API for the completion signal; CV is reserved for part localization on the bed.
- **Not** multi-printer orchestration. v1 manages one printer; multi-printer queuing is post-resume work.
- **Not** support-material removal or finishing.

---

## 2. Method — Sim-to-Real via Digital Twin (+ Model-Based Control Design)

Every motion executed on the real arm is first planned and validated against a Gazebo digital twin of the same URDF, and the per-joint controllers driving that motion are designed and code-generated in Simulink against a Simscape Multibody plant model of the joint. The method is **deliberately classical** (URDF + MoveIt + ros2_control + Simulink/Embedded Coder), not learned policies — see [scope discipline](#2b-scope-discipline) below.

The two simulators play complementary roles:

- **Gazebo** answers system-level questions: "Does the whole arm execute the planned trajectory without collision or kinematic surprise?"
- **Simscape (Simulink)** answers joint-level questions: "Does this controller drive *one joint* correctly given the motor, cycloidal backlash, and link inertia dynamics?"

Both feed into the same firmware. The Simulink workflow produces the C source for the joint PID loop via Embedded Coder, which is linked into the PlatformIO firmware build — see [`implementation/simulink_workflow.md`](implementation/simulink_workflow.md).

### 2a. Why this method
- **Reuses existing infrastructure.** The `software/ros2_ws/src/robot_description/` and `simulation/` scaffolds already exist; sim-to-real is the natural next step rather than a new domain.
- **Lets the simulation be the specification.** When sim and real disagree, the URDF is corrected until they match — this is already the project's stated philosophy (`Scope.md §1`).
- **Builds the muscle that top robotics employers care about** without requiring an RL training run that wouldn't transfer for a print-tending task anyway.

### 2b. Scope discipline
The sim-to-real spectrum, with this project's chosen level marked:

| Level | Description | Decision |
|:--|:--|:--|
| L1 — Digital twin | URDF in Gazebo matches real arm; same trajectory runs in both | ✅ **Required** |
| L2 — Planner-in-the-loop | MoveIt plans in sim, executes on real arm with encoder feedback | ✅ **Required (headline)** |
| L3 — Imitation learning | Record teleop demos, train behavior cloning, deploy | ⏸ **Out of scope for v1** |
| L4 — RL with domain randomization | Train PPO/SAC in sim, deploy via DR | ❌ **Explicitly out of scope** — wrong tool for free-space pick-and-place; reserved for a future contact-rich follow-on application |

### 2c. Functional requirements
| # | Requirement | Type |
|:--|:--|:--|
| M1 | URDF link geometry matches CAD-measured dimensions within ±2 mm | HARD |
| M2 | URDF link masses + inertia tensors populated from CAD (no zero-inertia links) | HARD |
| M3 | Same MoveIt trajectory executes in Gazebo and on the real arm with the same goal pose | HARD |
| M4 | Joint position error (sim trajectory vs. real trajectory) reported per joint after each test run | SOFT |
| M5 | Sim-only "dry run" of any tending sequence available as a pre-flight check before real execution | SOFT |
| M6 | Per-joint PID controller designed against a Simscape Multibody plant model, tuned with PID Tuner | HARD |
| M7 | Embedded Coder generates the C source for the controller; same controller runs bit-for-bit in Simulink and on the STM32 (validated via Software-in-the-Loop equivalence test) | HARD |
| M8 | Three-way agreement at Checkpoint C: trajectory in Gazebo, trajectory in Simscape, trajectory on real hardware all within ±5° per joint | SOFT |

---

## 3. Staged Delivery Plan

Every checkpoint is independently demo-able. If the project runs out of time mid-stage, the previous checkpoint is still a complete artifact that can be filmed and put on a resume.

| Checkpoint | Deliverable | Independent Demo Value |
|:--|:--|:--|
| **A — "It moves"** | Both J1 + J2 driven by STM32 firmware via hand-coded PID and simple position commands over serial | Baseline mechanical demo (already complete in prototype form) |
| **B — "It moves in sim"** | URDF loads in RViz; joint sliders match real arm pose. URDF also imported into Simscape Multibody to validate the plant-model pipeline | Honest claim of ROS 2 + URDF + Simulink familiarity |
| **C — "It plans"** | MoveIt configured; click a goal in RViz, real arm goes there. Per-joint controller designed in Simulink, code-generated by Embedded Coder, integrated into the firmware; three-way (Gazebo + Simscape + real) agreement within ±5° per joint | Planner-in-the-loop demo + model-based design demo — two strong artifacts |
| **D — "It sees"** | Fixed overhead camera; ArUco marker or simple CNN localizes a part on the bed; arm picks it up from a known pose | Vision pipeline working end-to-end |
| **E — "It tends"** | OctoPrint signal triggers full cycle: detect → pick → drop in bin → start next job. Gravity-comp feed-forward added to J2 in Simulink for full-extension payload pick | Headline demo — the application as designed |
| **F — Stretch: "Sim predicted it"** | Same tending sequence runs in Gazebo first; side-by-side video shows sim and real agreeing. Optionally: ROS Toolbox co-simulation with Simulink as the plant | The sim-to-real cherry on top |

### 3a. Scope-creep traps to avoid
- Do **not** build a "is the print done" CNN. OctoPrint's HTTP API returns this in one call.
- Do **not** try to handle every print geometry in v1. Constrain to parts with a known footprint (or an ArUco sticker on top) and a known bed pose.
- Do **not** write a custom IK solver. Use MoveIt's TRAC-IK plugin.
- Do **not** build joints 3–6 before checkpoint C closes end-to-end on J1 + J2. A 2-DOF arm that tends prints beats a 6-DOF arm that doesn't move.

---

## 4. Relationship to other documents

- [`Scope.md`](Scope.md) — Project-wide context. References this file for the end application.
- [`Constraints.md`](Constraints.md) — Hard/soft engineering limits. Section 0 references the derived requirements in [§1c above](#1c-derived-hardware-requirements). §3c documents the Simulink → firmware codegen contract.
- [`implementation/simulink_workflow.md`](implementation/simulink_workflow.md) — Full Simulink workflow: plant modeling, controller design, Embedded Coder integration, anti-patterns.
- [`firmware/todo.md`](../firmware/todo.md), [`software/todo.md`](../software/todo.md), [`simulation/todo.md`](../simulation/todo.md), [`simulink/README.md`](../simulink/README.md) — Subsystem backlogs organized around the [staged checkpoints](#3-staged-delivery-plan) above.
- [`Journal.md`](Journal.md) — Decision log entry capturing why this application + method were chosen.
