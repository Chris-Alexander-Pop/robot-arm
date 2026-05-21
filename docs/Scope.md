<!--
This file contains the scope / goal of the entire project, used for context and understanding.
It serves as the primary context document for LLMs and new project contributors.
-->

# 6-DOF Robot Arm — Project Scope & Context

This document outlines the scope, architecture, hardware stack, and technology choices for the 6-Degrees of Freedom (6-DOF) Robot Arm project. It is intended as the **primary context document** for LLMs or new developers joining the project. For deeper detail on any subsystem, refer to the relevant document in `/docs/implementation/`.

---

## 1. Project Overview

The goal is to design, build, and program a **6-DOF robotic arm** that performs **autonomous 3D-print bed tending** — removing finished parts from an FDM printer and starting the next queued job — with all motion validated through a **sim-to-real digital twin** in Gazebo before execution on real hardware.

The design is philosophically aligned with professional lightweight arms: **heavy motors sit at the base, power is transmitted to distal joints via belts or cycloidal drives, and the arm structure is 3D-printed PETG reinforced with metal hardware**. Because the arm's own structural parts are FDM-printed, the chosen application directly serves the build pipeline that produced it (the arm tends the printers that build the arm).

> **For the full application specification** — functional requirements, derived hardware specs, sim-to-real scope discipline, staged delivery checkpoints, and explicit non-goals — see [`Application.md`](Application.md). This document defers all application-level definitions to that file.

### Key Targets
- **Payload**: 0.5 – 1.0 kg at the end-effector (≥ 200 g required for the print-tending application)
- **Reach**: ~630mm total (shoulder to wrist, arm fully extended; ≥ 300 mm over the print bed required)
- **Repeatability**: ±0.5 – 1.0 mm (limited by mechanical compliance, not step resolution; print-tending needs only ±2 mm)
- **Control**: ROS 2 + MoveIt 2 for trajectory planning; STM32 for real-time motor execution
- **Cost**: ~$350–400 CAD for Phase 1 prototyping; ~$700–800 CAD for full 6-axis assembly

### Development Philosophy

The project follows a **phased, iterative approach**:
1. **Prove the difficult things first**: The cycloidal drive (novel, fabrication-risk) and the closed-loop command chain (STM32 → driver → motor → feedback) are prototyped before a full arm assembly is committed to.
2. **Simulate before fabricate**: DH parameters and link geometry are validated in Python/RViz before printing. Task scripts are validated in Gazebo before running on hardware.
3. **The simulation is the specification**: If the Gazebo model doesn't match real behavior, the URDF/models are corrected until they do — not the other way around. The print-tending application is the concrete workload that exercises this loop end-to-end.
4. **Every checkpoint is independently demo-able**: The [staged delivery plan](Application.md#3-staged-delivery-plan) is sequenced so that no in-progress milestone leaves the project without a complete artifact to show.

---

## 2. System Architecture

The system is divided into five domains: **Mechanical**, **Firmware**, **Software**, **Simulation** (Python + Gazebo, free-space dynamics), and **Model-Based Design** (Simulink + Simscape + Embedded Coder, joint-level dynamics + controller codegen).

### 2a. Hardware & Mechanical

**Actuators & Drivers** (three motor tiers to balance torque and weight):

| Joint | Name | Motor | Driver | Gearing | Notes |
|:---:|:---|:---|:---|:---|:---|
| J1 | Base Rotation | NEMA 23 (2.0 Nm) | CL57T closed-loop | Cycloidal 20:1 | → 40 Nm effective; twin-disk |
| J2 | Shoulder Pitch | NEMA 23 (2.0 Nm) | CL57T closed-loop | Cycloidal 20:1 | Critical torque joint; twin-disk |
| J3 | Elbow Pitch | NEMA 17 (80 Ncm) | CL42T closed-loop | Cycloidal 15:1 | → 12 Nm; resolves torque shortfall |
| J4 | Forearm Twist | NEMA 17 (42 Ncm) | CL42T closed-loop | Cycloidal 10:1 | Resolution + damping; → 4.2 Nm |
| J5 | Wrist Pitch | NEMA 14 (14 Ncm) | TMC2209 open-loop | Direct | Ultra-lightweight |
| J6 | Tool Roll | NEMA 14 (14 Ncm) | TMC2209 open-loop | Direct | Ultra-lightweight |

**Power Supply**: Mean Well LRS-350-24 (350W, 24V, 14.6A) — sized for the 13.8A peak theoretical draw of all 6 motors.

**Key Components**:
- **Cycloidal drives (J1–J4)**: 3D-printed PETG disks + hardened steel dowel pins (ring teeth) + 608ZZ eccentric cam bearings + 6804/6806/6808 thin-section output bearings. Ratios: 20:1 (J1/J2), 15:1 (J3), 10:1 (J4).
- **Counterweights (J1–J4)**: Second 608ZZ on the rear motor shaft at 180°, cancelling eccentric imbalance at all speeds.
- **Twin-disk design (J1/J2)**: Two cycloidal disks 180° out of phase, cancelling torque ripple and axial forces.
- **74HC245**: Logic level shifter (3.3V STM32 → 5V for industrial CL57T/CL42T drivers)
- **LM2596**: Buck converters to drop 24V to 5V for logic (Pi, STM32)
- **A3144 Hall sensors**: Homing reference for each joint at boot
- **MG996R servo**: End-effector gripper actuator

**Structural Materials**: 3D-printed PETG; M3/M4/M5 alloy steel hardware; M3×5mm brass heat-set inserts for all high-torque bolt connections.

---

### 2b. Firmware (STM32 — Low-Level Control)

The STM32 microcontroller runs a **hard-real-time control loop** at ≥ 1 kHz:
- Receives target joint angles from the Raspberry Pi over UART/USB (`SET_JOINTS` binary packet)
- Generates precise STEP/DIR pulses to CL57T, CL42T, and TMC2209 motor drivers using hardware timer peripherals
- Reads position feedback over RS-485 from the closed-loop drivers on J1–J4 (their factory motor encoders are wired to the driver, not the STM32). For J5 / J6, position normally comes from the homed step counter — an AS5600 + (optionally) TCA9548A is a Phase 2 add-on if measured open-loop drift demands it.
- Executes per-joint **PID control loops** to minimize position error
- Manages the **homing sequence** (Hall sensor-based) on boot
- Monitors CL57T/CL42T **ALARM pins** for driver faults and sends `FAULT` packets to the Pi
- Implements a **serial watchdog**: if no command arrives within ~500ms, all joints enter hold mode

**Environment**: PlatformIO (C/C++), targeting the STM32 family.

**Communication Protocol**: Binary framed packets (`[0xAA 0x55][CMD][PAYLOAD][XOR_CHECKSUM]`) at 115200 baud default.

---

### 2c. Software (Raspberry Pi 4 — High-Level Control)

The Raspberry Pi 4 runs **ROS 2 Humble** in Docker containers, handling all compute-intensive, non-real-time tasks:

- **`robot_state_publisher`**: Publishes the URDF-based transform tree (`/tf`) from current `/joint_states`
- **MoveIt 2 (`move_group`)**: Trajectory planning (OMPL), IK solving (TRAC-IK), collision checking, trajectory parameterization
- **`hw_interface_node`**: `ros2_control` hardware plugin; bridges the STM32 serial port to ROS 2 topics/actions
- **`pick_and_place_node`**: High-level Python task logic — defines the workflow (approach, grasp, lift, place)

The software stack is designed so that **swapping between simulation and hardware requires changing only one parameter** in the MoveIt launch file (the `ros2_control` hardware interface plugin).

---

### 2d. Simulation & Digital Twin

- **Python IK/FK solver** (`/simulation/kinematics.py`): Pure NumPy implementation of FK and IK using DH parameters. Used for rapid kinematic validation before firing up ROS 2 or printing parts.
- **URDF model** (`/software/robot_arm_description/`): Full URDF xacro file with accurate link geometry, mass, inertia, and joint limits — exported from CAD and refined through simulation.
- **Gazebo (Ignition)**: Full physics simulation — tests ROS 2 control commands, collision avoidance, and trajectory quality with simulated joint dynamics.
- **RViz2**: Lightweight visualization — used for URDF checking, MoveIt motion planning visualization, and joint state monitoring.

### 2e. Model-Based Design & Codegen (Simulink)

The `/simulink/` workspace handles **joint-level dynamics and controller design** — a layer below what Gazebo simulates and one above the bare firmware loop. See [`implementation/simulink_workflow.md`](implementation/simulink_workflow.md) for the full workflow.

- **Simscape Multibody plant models** (`simulink/plant_models/`): Per-joint electromechanical models (motor torque-speed + cycloidal backlash/friction + link inertia), imported from the project URDF via `smimport`.
- **Controller designs** (`simulink/controllers/`): Per-joint PID (and gravity-comp feed-forward where needed), tuned with PID Tuner against the plant.
- **Embedded Coder codegen** (`simulink/codegen/`): Auto-generates C source for the controllers; output is committed and linked into the PlatformIO firmware build at `firmware/stm32_core/lib/control/generated/`. The firmware build itself does not require MATLAB.

The three simulators are layered, each catching a different class of bug:

| Simulator | Question it answers |
|:--|:--|
| Python FK/IK | "Do my DH parameters agree with first principles?" |
| Simscape (Simulink) | "Does this controller drive one joint correctly given motor + gearbox dynamics?" |
| Gazebo | "Does the whole arm execute MoveIt trajectories without collision or kinematic surprise?" |

---

## 3. Phased Roadmap

The hardware phases below (budget-driven) are interleaved with the application-driven **staged delivery checkpoints A–F** defined in [`Application.md §3`](Application.md#3-staged-delivery-plan). Every checkpoint is independently demo-able so that no in-progress phase leaves the project without a complete artifact.

### Phase 1 — Prototyping & Validation (~$350–400 CAD)

**Goal**: Prove the control chain and cycloidal drive geometry before committing to a full 6-axis build. Map directly to **Checkpoints A–C**.

1. Assemble a **single-joint bench prototype**: 1× NEMA 23 + CL57T driver + one 20:1 cycloidal drive unit (with counterweight + twin-disk) — *Checkpoint A: "It moves"*
2. Wire STM32 → 74HC245 → CL57T; verify reliable STEP/DIR command delivery — *Checkpoint A*
3. Validate cycloidal drive under load: add measured weight, measure output angle vs. commanded angle, confirm counterweight eliminates vibration
4. Wire UART between STM32 and Raspberry Pi; test the full command chain (Python script → ROS 2 → Pi → STM32 → motor)
5. Load preliminary URDF (J1 + J2 only) into RViz; verify joint sliders drive both sim and real — *Checkpoint B: "It moves in sim"*
6. Configure MoveIt for the 2-DOF subset; execute a planned trajectory end-to-end — *Checkpoint C: "It plans"*
7. Build and test the J3 motor tier: 1× NEMA 17 + CL42T + 15:1 cycloidal drive; validate torque and fit within upper arm link envelope

**Exit Criteria**: Successfully command a joint to a target angle and read back the actual position with < 1° error under load **and** demonstrate a MoveIt-planned trajectory executing on the real arm with the corresponding Gazebo dry-run matching within ±5° per joint.

### Phase 2 — Integration & Assembly (~$700–800 CAD total)

**Goal**: Assemble all 6 axes; validate full kinematic chain; deliver the print-tending application. Maps to **Checkpoints D–F**.

1. Complete all 6 joint CAD designs with validated masses and URDF dimensions
2. Print and assemble J1–J4 cycloidal/belt units + structural links
3. Flash firmware with all 6 joints configured; run homing sequence
4. Load full URDF into Gazebo; validate physics simulation matches physical arm behavior (satisfies M1–M3 in [`Application.md §2c`](Application.md#2c-functional-requirements))
5. Mount overhead camera; implement ArUco-based part localization on the print bed — *Checkpoint D: "It sees"*
6. Integrate gripper (MG996R servo or suction variant); wire OctoPrint webhook → ROS 2 trigger; demonstrate full end-to-end tending cycle — *Checkpoint E: "It tends"*
7. Record side-by-side Gazebo + real execution of the tending sequence — *Checkpoint F (stretch): "Sim predicted it"*

---

## 4. Repository Structure

```
robot-arm/
├── cad/                    # Fusion 360 / FreeCAD source files
├── docs/
│   ├── implementation/
│   │   ├── mechanical_design.md
│   │   ├── electrical_design.md
│   │   ├── firmware_architecture.md
│   │   ├── software_architecture.md
│   │   ├── simulation_environment.md
│   │   └── simulink_workflow.md    # Model-based design + Embedded Coder workflow
│   ├── Scope.md            # ← This file
│   ├── Application.md      # Target application + sim-to-real method (single source of truth)
│   ├── Constraints.md      # Hard/soft engineering limits
│   ├── Calculations.md     # Engineering math & trade studies
│   ├── Design_Choices.md   # Decision log with rationale
│   ├── Journal.md          # Running development log
│   └── Examples.md         # Reference arms & lessons learned
├── firmware/               # PlatformIO STM32 project
│   ├── src/
│   └── platformio.ini
├── software/               # ROS 2 packages
│   ├── robot_arm_description/  # URDF, meshes
│   ├── robot_arm_moveit/       # MoveIt 2 config
│   ├── robot_arm_hw/           # ros2_control hardware plugin
│   ├── robot_arm_tasks/        # Python task nodes
│   ├── launch/
│   └── docker-compose.yml
├── simulation/             # Standalone Python kinematic scripts (free-space dynamics)
│   └── kinematics.py
├── simulink/               # Simulink workspace (joint dynamics + controller codegen)
│   ├── plant_models/       # Simscape Multibody per-joint models
│   ├── controllers/        # PID + feed-forward designs (.slx)
│   ├── codegen/            # Embedded Coder configuration + generated C (committed)
│   ├── scripts/            # MATLAB .m setup + parameter scripts
│   └── tests/              # Simulink Test harnesses
├── scripts/                # Repo-wide setup, dev, and utility scripts
│   ├── setup.sh            # Initial environment setup
│   └── dev.sh              # Start/stop ROS 2 Docker containers
```
