# Robot Arm Workspace Instructions

This repository is a 6-DOF robot arm project split across mechanical design, embedded firmware, ROS 2 software, and Python simulation.

## Project Map
- `docs/` holds the project scope, calculations, design rationale, and subsystem implementation notes.
- `firmware/stm32_core/` is the STM32 PlatformIO project for low-level motor control.
- `software/ros2_ws/` is the ROS 2 workspace for the high-level control stack.
- `simulation/` is the Python-first kinematics and trajectory sandbox.
- Repo helper scripts live in `scripts/` (`setup.sh`, `setup.ps1`, `dev.sh`, `dev.ps1`).

## Preferred Workflow
- Read the relevant subsystem docs before changing code: start with `docs/Scope.md`, `docs/Design_Choices.md`, and the matching file under `docs/implementation/`.
- Keep changes small and localized to the subsystem you are working in.
- Update docs or TODO files when a change alters behavior, architecture, or the expected workflow.
- Prefer existing project patterns over introducing new frameworks or abstractions.

## Build And Run
- Initial setup:
  - Linux/macOS: `./scripts/setup.sh`
  - PowerShell: `./scripts/setup.ps1`
- ROS 2 dev container:
  - Start: `./scripts/dev.sh up`
  - Stop: `./scripts/dev.sh down`
  - Shell: `./scripts/dev.sh shell`
  - Build: `./scripts/dev.sh build`
  - Launch RViz demo: `./scripts/dev.sh launch`
- Simulation:
  - Create the virtual environment with the repo setup scripts.
  - Run tests from `simulation/` with `pytest`.
- Firmware:
  - Work inside `firmware/stm32_core/` with PlatformIO.
  - Use the configured `nucleo_f401re` environment (Nucleo-F401RE).

## Conventions And Pitfalls
- Do not treat `software/ros2_ws/build`, `software/ros2_ws/install`, or `software/ros2_ws/log` as source; they are generated ROS artifacts.
- `scripts/dev.sh` resolves paths from the repository root.
- The ROS 2 stack is containerized; source the ROS environment inside the container before building or launching.
- Keep simulation code Python-first and testable with `pytest`.
- Preserve the existing split between documentation, firmware, software, and simulation; avoid moving responsibilities across those boundaries unless the project plan requires it.

## Useful Starting Files
- `README.md`
- `docs/Scope.md`
- `docs/Design_Choices.md`
- `firmware/README.md`
- `software/README.md`
- `simulation/README.md`
