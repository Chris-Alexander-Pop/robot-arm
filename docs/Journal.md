<!-- 
Use this file to journal development with wins, problems encountered, and whatever else... 
-->

Mar 26: cycloidal disk

May 17: **Scoping decision — picked the headline application and method.**

Up to this point the project had no concrete end goal beyond "build a 6-DOF arm." This was hurting on two fronts: (1) hard to justify design trade-offs without a workload to reference, and (2) hard to communicate what the project actually *does* on a resume.

Picked **autonomous 3D-print bed tending** as the headline application, with **sim-to-real via a Gazebo digital twin** as the method that wraps around it. Rationale captured in [`Application.md`](Application.md); key points:

- Print tending lives comfortably inside the existing mechanical envelope (free-space motion + top-down grasp, ±2 mm placement tolerance) — no redesign required.
- The application emerged naturally from the build itself: the arm tends the printers that produce its own structural parts. Clean narrative.
- Sim-to-real is constrained to **L1 (digital twin) + L2 (planner-in-the-loop)** — explicitly not RL/domain-randomization. The free-space pick task doesn't justify a learned policy, and the scope discipline keeps the project shippable.
- Delivery is sequenced into **independently demo-able checkpoints A→F** so that if any milestone slips, the previous one is still a complete artifact (video / GIF / write-up).

Updated `README.md`, `Scope.md`, and `Constraints.md` to reference `Application.md` rather than restating the goal. Reorganized the three subsystem `todo.md` files around checkpoints A–F.

Next concrete step: Checkpoint B — load preliminary J1+J2 URDF into RViz and confirm joint sliders drive both the sim and the real arm.

May 17 (later): **Added Simulink / Simscape / Embedded Coder to the toolchain.**

Rationale: hand-tuning the joint PID on the real arm is slow, risky, and unrepeatable (gear backlash and friction drift). Designing against a Simscape Multibody plant model lets the controller converge offline; Embedded Coder then carries the *exact* tuned controller bit-for-bit to the STM32. This is also the standard workflow at automotive / aerospace OEMs and is a strong resume signal for those teams.

Scope decisions:

- Simulink work lives in a new top-level `simulink/` directory, parallel to `simulation/`. Three simulators total — Python FK/IK, Simscape (Simulink), and Gazebo — each catching a different class of bug. See [`Scope.md §2e`](Scope.md#2e-model-based-design--codegen-simulink) for the layering rationale.
- **Planned (not in repo yet):** Generated C from Embedded Coder would live under `simulink/codegen/output/` so the firmware build does not require a MATLAB license.
- **Planned:** The codegen → firmware boundary is a single hand-written adapter (`joint_controller_generated.cpp`); see [`Constraints.md §3c`](Constraints.md) for the contract. Firmware today uses hand-coded control only.
- Scope-disciplined: Simulink is for **joint-level control design + codegen**, not for replacing MoveIt or the system-level Python tooling. Anti-patterns documented in [`implementation/simulink_workflow.md §6`](implementation/simulink_workflow.md).
- The Simulink milestone slots into Checkpoint C (`Application.md §3`): same trajectory in Gazebo, Simscape, and on the real arm — three-way agreement within ±5° per joint is the exit criterion.

Updated `Scope.md`, `Constraints.md` (new §3c), `Application.md` (new M6–M8 + revised Checkpoint table), `firmware/todo.md`, `simulation/todo.md`, root `README.md`, and `docs/implementation/firmware_architecture.md`. Created `docs/implementation/simulink_workflow.md` as the central reference.

Next concrete step on the Simulink side: write `simulink/scripts/setup_workspace.m` and `joint_params.mat` from the existing constants in `Constraints.md` / `Calculations.md` so any future model has a single source of parameter truth.
