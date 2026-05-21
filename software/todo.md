# Software Roadmap

This document outlines the high-level control logic and ROS 2 workspace running on the Raspberry Pi.

The backlog is organized around the **staged delivery checkpoints A–F** defined in [`docs/Application.md`](../docs/Application.md#3-staged-delivery-plan). Each checkpoint lists the software work required to unlock it.

---

## Checkpoint A — "It moves" *(minimal Pi involvement)*

*Exit criterion: A simple host-side Python script can issue `SET_JOINTS` packets and watch J1 / J2 move.*

- [ ] **Serial Sender (no ROS yet)**: A small Python utility that opens the STM32 serial port, frames a `SET_JOINTS` packet, and reads back telemetry. Used to validate the firmware before any ROS plumbing.

---

## Checkpoint B — "It moves in sim"

*Exit criterion: URDF loads in RViz; joint sliders drive the real arm in lockstep with the visualized model.*

- [ ] **ROS 2 Workspace Setup**: Package structure for `robot_core` and `robot_description` (skeleton already present).
- [ ] **URDF — J1 + J2 subset**: Pull link lengths from CAD; populate masses + inertia tensors so Gazebo physics is meaningful later (`docs/Constraints.md §4d`).
- [ ] **Hardware Bridge Node**: ROS 2 node that bridges `/joint_commands` → STM32 serial, and STM32 telemetry → `/joint_states`. Test with the 2-DOF subset.
- [ ] **Joint State Publisher GUI**: Wire it up so dragging sliders moves both the RViz model and the real arm — this is the visible demo.
- [ ] **Safety & Heartbeat**: Software watchdog that monitors connection health between Pi and STM32; emits a halt command on timeout.

---

## Checkpoint C — "It plans"

*Exit criterion: Click a goal pose in RViz, MoveIt plans, real arm executes within tolerance of the Gazebo dry-run.*

- [x] **MoveIt 2 Configuration Generation**: Use the MoveIt Setup Assistant with the preliminary URDF to generate move groups and kinematics plugins.
- [x] **MoveIt 2 Package Scaffold**: Added `robot_arm_moveit` with launch, configuration, and smoke tests.
- [ ] **MoveIt 2 Tuning**: Tune trajectory execution, allowed tolerances, and planning algorithms (OMPL) on the 2-DOF subset.
- [ ] **`FollowJointTrajectory` Action Server**: Bridge MoveIt's trajectory output to the hardware bridge node at 50–100 Hz (`docs/Constraints.md §4b`).
- [ ] **Sim/Hardware Toggle**: Single launch parameter switches the `ros2_control` plugin between `gazebo_ros2_control/GazeboSystem` and the real serial interface (`docs/implementation/software_architecture.md §4`).

---

## Checkpoint D — "It sees"

*Exit criterion: Fixed overhead camera detects a marked part on the print bed; arm picks it up using a vision-derived pose.*

- [ ] **Camera Node**: Off-the-shelf `usb_cam` or `v4l2_camera` node publishing to `/camera/image_raw`.
- [ ] **Part Localization**: ArUco-marker-based pose estimation in the bed frame (`tf2`). CV is intentionally scoped to ArUco for v1 — see `Application.md §1d`.
- [ ] **Pick Pose Service**: Converts a detected part pose into a MoveIt goal (pre-grasp → grasp → lift waypoints).
- [ ] **Gripper Control**: Action client / topic for the parallel-jaw or suction end-effector; coordinates with `MOVE_COMPLETE` telemetry from the STM32.
- [ ] **Collision Avoidance**: Static scene constraints — table, printer enclosure, bed — to prevent self-collision and printer collision (octomap optional).

---

## Checkpoint E — "It tends"

*Exit criterion: Triggering a print-complete event on the printer causes a full unattended tending cycle to execute.*

- [ ] **OctoPrint Bridge**: ROS 2 node that polls or subscribes to OctoPrint's HTTP API for the `PrintDone` event and emits a ROS event. *No CV-based completion detection — see `Application.md §1d`.*
- [ ] **Tending State Machine**: `WAITING → DETECT → APPROACH → GRASP → LIFT → PLACE → SIGNAL_NEXT_JOB → WAITING` with retry logic on missed picks (`Application.md §1b F5`).
- [ ] **Bin Pose Calibration**: One-time procedure to record the bin drop pose in the base frame.
- [ ] **Web Dashboard**: rosbridge-based interface for manual override, E-stop trigger, and cycle-count telemetry (useful for the demo video).
- [ ] **Task Logging**: Per-cycle log of timing, fault counts, and any retried picks — provides the data for the "cycle time < 60 s" soft requirement (`Application.md §1b F7`).

---

## Checkpoint F — "Sim predicted it" *(stretch)*

*Exit criterion: A single command runs the same tending sequence in Gazebo first, then on the real arm, and emits a side-by-side comparison.*

- [ ] **Dry-Run Mode**: Tending state machine accepts a `--sim-only` flag that routes execution to the Gazebo `ros2_control` plugin instead of the serial one.
- [ ] **Trajectory Diff Tool**: Replays the sim and real telemetry side by side, reports per-joint position error over time, and renders a video overlay.
- [ ] **Pre-Flight Hook**: Optional gate that runs the dry-run before every real execution and aborts if the planned trajectory violates a constraint in sim.

---

## Out-of-scope (revisit after Checkpoint E ships)

- Reinforcement learning / behavior cloning policies (`Application.md §2b` — explicitly L3/L4 are out of scope).
- Multi-printer orchestration (`Application.md §1d`).
- Generalized CV pipeline that detects arbitrary part geometries (`Application.md §1d` — v1 assumes ArUco-marked parts).
