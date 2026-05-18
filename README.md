# 6-DOF Robot Arm

A 6-degree-of-freedom robotic arm for **autonomous 3D-print bed tending** — motion planning on a Raspberry Pi, real-time stepping on an **STM32**, and validation through simulation (Python, Gazebo URDF) plus Renode firmware-in-the-loop tests.

**Stack:** ROS 2 Humble · MoveIt 2 · STM32 (PlatformIO) · KiCad · SolidWorks · Python simulation · Simulink workflow *(planned, not in repo yet)*

[![CI](https://github.com/Chris-Alexander-Pop/robot-arm/actions/workflows/ci.yml/badge.svg)](https://github.com/Chris-Alexander-Pop/robot-arm/actions/workflows/ci.yml)

## Status

Active development — not ready for a versioned product release. Checkpoint progress and honest scope: [`docs/Application.md`](docs/Application.md).

## Application

The arm targets **autonomous 3D-print bed tending** — detecting print completion, removing the finished part, and starting the next job. Structural parts are FDM-printed (PETG cycloidal drives on driven joints).

Trajectories are validated through a **sim-to-real** path: plan in ROS / Gazebo, exercise firmware on the bench and in Renode, then run on hardware.

## Architecture

| Layer | Location | Role |
|-------|----------|------|
| Planning | `software/ros2_ws/` | ROS 2, MoveIt 2, serial bridge |
| Real-time control | `firmware/stm32_core/` | STM32 Nucleo-F401RE, UART protocol, hand-coded PID scaffold |
| Simulation | `simulation/` | Python kinematics, Renode smoke tests |
| Mechanical | `mechanical/` | SolidWorks; STEP exports where present |
| Electrical | `hardware/` | KiCad, WireViz, datasheet references |
| Model-based design | `simulink/` | **Planned** — README scaffold only |

Details: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) · [`docs/Design_Choices.md`](docs/Design_Choices.md)

## Quickstart (~10 minutes)

**Prerequisites:** Git, Python 3.11+, [PlatformIO](https://platformio.org/) (`pio`), Docker (for ROS tests).

```bash
git clone https://github.com/Chris-Alexander-Pop/robot-arm.git
cd robot-arm
./setup.sh

# Firmware (host native tests — no hardware)
./firmware/scripts/test.sh

# Python simulation
./simulation/scripts/test.sh

# ROS / MoveIt (Docker; slower first run)
./dev.sh up
./dev.sh moveit-test
```

Full matrix: [`TESTING.md`](TESTING.md)

## Repository map

| Path | Contents |
|------|----------|
| `docs/` | Requirements, architecture, journal |
| `mechanical/` | CAD (SolidWorks); see [`docs/implementation/cad_exports.md`](docs/implementation/cad_exports.md) |
| `hardware/` | KiCad, datasheets, WireViz |
| `firmware/` | STM32 C++ (PlatformIO) |
| `software/` | ROS 2 workspace + Docker |
| `simulation/` | Python sim + Renode |
| `simulink/` | Planned MBD workspace (not in tree yet) |

## Verification (CI)

| Layer | Local | CI job |
|-------|-------|--------|
| Firmware (native) | `./firmware/scripts/test.sh` | `firmware-native-tests` |
| Firmware (Renode) | build `nucleo_f401re_renode`, `simulation/renode/tests/firmware_smoke.robot` | `firmware-build-and-renode` |
| ROS / MoveIt | `./software/scripts/test.sh` | `software-behavioral-tests` |
| Simulation | `./simulation/scripts/test.sh` | `simulation-tests` |

## Team setup

- Linux/macOS: [`./setup.sh`](setup.sh)
- Windows: [`./setup.ps1`](setup.ps1)

Creates `simulation/.venv`, `.tooling/` ROS CLI tools, and Renode for headless firmware tests. Firmware uses PlatformIO, not a Python venv.

Dev helpers: [`./dev.sh`](dev.sh) / [`./dev.ps1`](dev.ps1) for Dockerized ROS.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`firmware/CONTRIBUTING.md`](firmware/CONTRIBUTING.md). Firmware uses intentional `TODO(contributor)` scaffolds for learning — not abandoned code.

## Licenses and third-party materials

- **Source code** (firmware, ROS, simulation, scripts): [MIT](LICENSE) — see [NOTICE](NOTICE).
- **Datasheets:** vendor URLs in [`hardware/datasheets/README.txt`](hardware/datasheets/README.txt) (PDFs not in git; download locally if needed).
- **CAD:** SolidWorks native files are proprietary format; STEP provided where applicable.
- **StepperDriver** (firmware dependency, laurb9): MIT — STEP/DIR for hardware tests and future `StepperDriver` implementation.
- **MATLAB / Simulink:** optional future workflow; trademarks of The MathWorks, Inc.

## Security

Hobby / bench project only — see [`SECURITY.md`](SECURITY.md). Serial control has no authentication; use a trusted USB-UART link.

## Deep dive

- [`docs/Application.md`](docs/Application.md) — checkpoints A–F
- [`docs/Examples.md`](docs/Examples.md) — comparison with AR4, Moveo, etc.