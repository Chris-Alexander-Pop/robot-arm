<!--
This file contains the scope / goal of the entire project, used for context and understanding.
It serves as the primary context document for LLMs and new project contributors.
-->

# 6-DOF Robot Arm — Project Scope & Context

This document outlines the scope, architecture, hardware stack, and technology choices for the 6-Degrees of Freedom (6-DOF) Robot Arm project. It is intended as the **primary context document** for LLMs or new developers joining the project. For deeper detail on any subsystem, refer to the relevant document in `/docs/implementation/`.

---

## 1. Project Overview

The goal is to design, build, and program a **6-DOF robotic arm** capable of precise pick-and-place operations on a desktop scale. The design is philosophically aligned with professional lightweight arms: **heavy motors sit at the base, power is transmitted to distal joints via belts or cycloidal drives, and the arm structure is 3D-printed PETG reinforced with metal hardware**.

### Key Targets
- **Payload**: 0.5 – 1.0 kg at the end-effector
- **Reach**: ~630mm total (shoulder to wrist, arm fully extended)
- **Repeatability**: ±0.5 – 1.0 mm (limited by mechanical compliance, not step resolution)
- **Control**: ROS 2 + MoveIt 2 for trajectory planning; STM32 for real-time motor execution
- **Cost**: ~$350–400 CAD for Phase 1 prototyping; ~$700–800 CAD for full 6-axis assembly

### Development Philosophy

The project follows a **phased, iterative approach**:
1. **Prove the difficult things first**: The cycloidal drive (novel, fabrication-risk) and the closed-loop command chain (STM32 → driver → motor → feedback) are prototyped before a full arm assembly is committed to.
2. **Simulate before fabricate**: DH parameters and link geometry are validated in Python/RViz before printing. Task scripts are validated in Gazebo before running on hardware.
3. **The simulation is the specification**: If the Gazebo model doesn't match real behavior, the URDF/models are corrected until they do — not the other way around.

---

## 2. System Architecture

The system is divided into four domains: **Mechanical**, **Firmware**, **Software**, and **Simulation**.

### 2a. Hardware & Mechanical

**Actuators & Drivers** (three motor tiers to balance torque and weight):

| Joint | Name | Motor | Driver | Gearing | Notes |
|:---:|:---|:---|:---|:---|:---|
| J1 | Base Rotation | NEMA 23 (2.0 Nm) | CL57T closed-loop | Cycloidal 20:1 | → 40 Nm effective |
| J2 | Shoulder Pitch | NEMA 23 (2.0 Nm) | CL57T closed-loop | Cycloidal 20:1 | Critical torque joint |
| J3 | Elbow Pitch | NEMA 17 (80 Ncm) | CL42T closed-loop | GT2 belt 3–4:1 | Folded-axis mounting |
| J4 | Forearm Twist | NEMA 17 (42 Ncm) | CL42T closed-loop | Direct or belt 2:1 | |
| J5 | Wrist Pitch | NEMA 14 (14 Ncm) | TMC2209 open-loop | Direct | Ultra-lightweight |
| J6 | Tool Roll | NEMA 14 (14 Ncm) | TMC2209 open-loop | Direct | Ultra-lightweight |

**Power Supply**: Mean Well LRS-350-24 (350W, 24V, 14.6A) — sized for the 13.8A peak theoretical draw of all 6 motors.

**Key Components**:
- **Cycloidal drives** (J1/J2): 3D-printed PETG disks + hardened steel dowel pins as ring teeth + 608ZZ eccentric cam bearings + 6806/6808 thin-section output bearings
- **GT2 belts & pulleys**: 6mm belt, 36T pulleys at joint pivots for folded-axis joints
- **74HC245**: Logic level shifter (3.3V STM32 → 5V for industrial drivers)
- **LM2596**: Buck converters to drop 24V to 5V for logic (Pi, STM32)
- **A3144 Hall sensors**: Homing reference for each joint at boot
- **MG996R servo**: End-effector gripper actuator

**Structural Materials**: 3D-printed PETG; M3/M4/M5 alloy steel hardware; M3×5mm brass heat-set inserts for all high-torque bolt connections.

---

### 2b. Firmware (STM32 — Low-Level Control)

The STM32 microcontroller runs a **hard-real-time control loop** at ≥ 1 kHz:
- Receives target joint angles from the Raspberry Pi over UART/USB (`SET_JOINTS` binary packet)
- Generates precise STEP/DIR pulses to CL57T, CL42T, and TMC2209 motor drivers using hardware timer peripherals
- Reads position feedback from AS5600 encoders (I2C via TCA9548A multiplexer) where applicable
- Executes per-joint **PID control loops** to minimize position error
- Manages the **homing sequence** (Hall sensor-based) on boot
- Monitors CL57T/CL42T **ALARM pins** for driver faults and sends `FAULT` packets to the Pi
- Implements a **serial watchdog**: if no command arrives within ~500ms, all joints enter hold mode

**Environment**: PlatformIO (C/C++), targeting the STM32 family.

**Communication Protocol**: Binary framed packets (`[0xAA 0x55][CMD][PAYLOAD][XOR_CHECKSUM]`) at 115200 baud default.

---

### 2c. Software (Raspberry Pi 3 — High-Level Control)

The Raspberry Pi 3 runs **ROS 2 Humble** in Docker containers, handling all compute-intensive, non-real-time tasks:

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

---

## 3. Phased Roadmap

### Phase 1 — Prototyping & Validation (~$350–400 CAD)

**Goal**: Prove the control chain and cycloidal drive geometry before committing to a full 6-axis build.

1. Assemble a **single-joint bench prototype**: 1× NEMA 23 + CL57T driver + one cycloidal drive unit
2. Wire STM32 → 74HC245 → CL57T; verify reliable STEP/DIR command delivery
3. Validate cycloidal drive under load (add measured weight; measure output angle vs. commanded angle)
4. Wire UART between STM32 and Raspberry Pi; test the full command chain (Python script → ROS 2 → Pi → STM32 → motor)
5. Build and test the second motor tier: 1× NEMA 17 + CL42T; test belt drive geometry in CAD

**Exit Criteria**: Successfully command a joint to a target angle and read back the actual position with < 1° error under load.

### Phase 2 — Integration & Assembly (~$700–800 CAD total)

**Goal**: Assemble all 6 axes; validate full kinematic chain; achieve first pick-and-place.

1. Complete all 6 joint CAD designs with validated masses and URDF dimensions
2. Print and assemble J1–J4 cycloidal/belt units + structural links
3. Flash firmware with all 6 joints configured; run homing sequence
4. Load URDF into Gazebo; validate physics simulation matches physical arm behavior
5. Run MoveIt pick-and-place task scripts; validate end-effector reach and repeatability
6. Integrate gripper (MG996R servo); implement full pick-and-place task

---

## 4. Repository Structure

```
robot-arm/
├── cad/                    # Fusion 360 / FreeCAD source files
├── docs/
│   ├── finances/
│   │   ├── BOM.md          # Full Bill of Materials (phase 1 & 2)
│   │   └── EXPENSES.md     # Actual spending tracker
│   ├── implementation/
│   │   ├── mechanical_design.md
│   │   ├── electrical_design.md
│   │   ├── firmware_architecture.md
│   │   ├── software_architecture.md
│   │   └── simulation_environment.md
│   ├── Scope.md            # ← This file
│   ├── Calculations.md     # Engineering math & trade studies
│   ├── Design_Choices.md   # Decision log with rationale
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
├── simulation/             # Standalone Python kinematic scripts
│   └── kinematics.py
├── setup.sh                # Initial Docker environment setup
└── dev.sh                  # Start/stop ROS 2 Docker containers
```
