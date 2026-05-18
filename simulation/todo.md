# Simulation Roadmap

This document outlines the **system-level** kinematic models and physics simulation environments (Python FK/IK + Gazebo) — the **sim** half of the sim-to-real pipeline defined in [`docs/Application.md §2`](../docs/Application.md#2-method--sim-to-real-via-digital-twin).

> **Joint-level dynamics (motor + cycloidal + link) and per-joint controller design live in [`/simulink/`](../simulink/README.md)** — see [`docs/implementation/simulink_workflow.md`](../docs/implementation/simulink_workflow.md) for that split.

The backlog is organized around the **staged delivery checkpoints A–F**. Simulation work is front-loaded toward Checkpoints B–C (digital twin + planner-in-the-loop) since those define the entire method.

---

## Checkpoint A — "It moves" *(no simulation work)*

---

## Checkpoint B — "It moves in sim"

*Exit criterion: A URDF of J1 + J2 loads in RViz and matches the physical arm pose when driven by the same joint commands.*

- [ ] **DH Parameter Estimation**: Draft Denavit-Hartenberg parameters for J1 + J2 from preliminary CAD dimensions (`dh_solver.py`).
- [ ] **Forward Kinematics (FK)**: Pure Python FK solver to validate coordinate transforms (numpy only — no ROS dependency, fast to iterate).
- [ ] **URDF — Primitive Geometry**: Build the J1 + J2 URDF using cylinders/boxes to model the joint hierarchy. Real meshes come later.
- [ ] **RViz2 Visualization**: ROS 2 launch file that loads the primitive URDF, publishes `/tf`, and accepts `/joint_states` from either the GUI slider node or the real hardware bridge.
- [ ] **Dimensional Validation**: Measure the real assembled arm vs. the URDF; iterate until link lengths agree within ±2 mm (`docs/Application.md §2c M1`).

---

## Checkpoint C — "It plans"

*Exit criterion: MoveIt produces a trajectory in sim; the same trajectory executes on real hardware with per-joint error matching the dry-run within ±5°.*

- [ ] **Inverse Kinematics (IK) Prototyping**: Implement and test analytical IK on the 2-DOF subset to sanity-check MoveIt's TRAC-IK output.
- [ ] **Visual Assets**: Export J1 + J2 CAD parts to STL, decimate, link as visual meshes in the URDF (collision meshes remain primitive for speed).
- [ ] **Inertia Tensors**: Extract mass + COM + inertia from CAD for J1 + J2 links. Required for any future Gazebo physics — Gazebo will silently accept zero-inertia URDFs and produce nonsense (`docs/Constraints.md §4d`).
- [ ] **Sim vs. Real Telemetry Logger**: A Python script that records the planned trajectory, the actual STM32-reported angles, **and the Simscape-predicted angles** (exported from Simulink as CSV), computes per-joint error across all three, and renders a plot. This is the artifact that proves Checkpoint C (`Application.md §2c M8`).

---

## Checkpoint D — "It sees" *(camera-side sim is optional)*

- [ ] **Camera Frame in URDF**: Add the overhead camera as a fixed link with a known transform to the print bed frame, so MoveIt understands where vision data is anchored.
- [ ] *(Optional)* **Simulated Camera in Gazebo**: Use Gazebo's camera plugin to render a synthetic bed view — enables dry-running the part-localization pipeline before the real camera is mounted.

---

## Checkpoint E — "It tends" *(physics fidelity matters now)*

*Exit criterion: A Gazebo simulation runs the full tending state machine and behaves qualitatively like the real arm.*

- [ ] **URDF Refinement (all 6 joints)**: Extract actual physical properties (mass, COM, inertia tensors) from final CAD for J3–J6 and update the URDF.
- [ ] **All-Link Visual Assets**: Export, decimate, and link STLs for all six links.
- [ ] **Gazebo / Ignition Setup**: Configure a physics world with joint friction, gravity, and collision models. Include a static printer + bed + bin in the scene.
- [ ] **ros2_control Gazebo Plugin**: Implement `ros2_control` hardware interfaces in Gazebo so the same control stack runs in sim and on hardware unchanged.
- [ ] **Stepper Dynamics Approximation**: Model torque rolloff vs. speed and microstepping behavior at least crudely — pure-velocity joints will not predict cycloidal compliance failures.

---

## Checkpoint F — "Sim predicted it" *(stretch)*

*Exit criterion: One command runs the tending sequence in Gazebo, then on the real arm, and produces a side-by-side video + a per-joint error report.*

- [ ] **Digital Twin Validation Suite**: Standard battery of trajectories run in both sim and real, with auto-generated comparison plots. Becomes the regression test for any URDF or controller change.
- [ ] **URDF Tuning Loop**: When sim and real disagree by > 5° on any joint, automatically flag which URDF parameter (link length, inertia, joint damping) is the most likely culprit. This operationalizes the "simulation is the specification" philosophy from `docs/Scope.md §1`.

---

## Out-of-scope

- Domain-randomized RL training pipelines (`Application.md §2b` — L4 is explicitly out of scope for v1).
- Photorealistic rendering or Isaac Sim integration (Gazebo is sufficient for L1 + L2 transfer).
