# Architecture index

One-page map into the design documentation. The repository is a **monorepo** spanning
mechanical CAD, electrical design, STM32 firmware, ROS 2 software, Python simulation,
and (planned) Simulink model-based design.

## System context

| Layer | Location | Role |
|-------|----------|------|
| Planning / orchestration | `software/ros2_ws/` | ROS 2 Humble, MoveIt 2, hardware bridge |
| Real-time control | `firmware/stm32_core/` | STM32 Nucleo-F401RE, STEP/DIR, UART protocol |
| Digital twin | `simulation/` | Python kinematics, URDF, Renode firmware smoke |
| Mechanical | `mechanical/` | SolidWorks models; STEP exports where present |
| Electrical | `hardware/kicad/`, `hardware/wireviz/` | Schematics, harness diagrams |
| Model-based design (planned) | `simulink/` | README scaffold only — not in tree yet |

**High-level story:** [`Application.md`](Application.md) · [`Design_Choices.md`](Design_Choices.md) · [`Scope.md`](Scope.md)

## Implementation deep dives

| Topic | Document |
|-------|----------|
| Firmware (STM32, protocol, PID) | [`implementation/firmware_architecture.md`](implementation/firmware_architecture.md) |
| Software (ROS 2, bridge, MoveIt) | [`implementation/software_architecture.md`](implementation/software_architecture.md) |
| Simulation & Gazebo | [`implementation/simulation_environment.md`](implementation/simulation_environment.md) |
| Simulink workflow (planned) | [`implementation/simulink_workflow.md`](implementation/simulink_workflow.md) |
| Mechanical design | [`implementation/mechanical_design.md`](implementation/mechanical_design.md) |
| Electrical / KiCad | [`implementation/electrical_design.md`](implementation/electrical_design.md) |

## Build & verify

- Setup: [`../scripts/setup.sh`](../scripts/setup.sh)
- Tests: [`../TESTING.md`](../TESTING.md)
## Subsystem READMEs

- [`../firmware/README.md`](../firmware/README.md)
- [`../software/README.md`](../software/README.md)
- [`../simulation/renode/README.md`](../simulation/renode/README.md)
- [`../simulink/README.md`](../simulink/README.md) (scaffold)
