# 6-DOF Robot Arm

A 6-Degree of Freedom (6-DOF) robotic arm project. This repository contains the mechanical CAD models, electrical schematics, firmware for the low-level microcontrollers, and high-level software/simulation code.

## Directory Structure

* `/docs` - Project design documents and specifications.
* `/hardware` - KiCad electrical drafts (`hardware/kicad/robot_arm/`), component PDFs (`hardware/datasheets/`, see `hardware/datasheets/README.txt` for source URLs), and optional WireViz harness tooling (`hardware/wireviz/`, run `./scripts/create-wireviz-venv.sh`).
* `/cad` - Mechanical design files (SolidWorks/Fusion 360).
* `/firmware` - Low-level C/C++ code for ESP32/STM32 microcontrollers.
* `/software` - High-level control software for Raspberry Pi 4 (ROS 2, kinematics).
* `/simulation` - Local simulation environments and URDF models.

## Architecture Overview

1.  **High-Level Control (Software)**: A Raspberry Pi 4 handles inverse kinematics, trajectory planning, and overall orchestration.
2.  **Low-Level Control (Firmware)**: ESP32 or STM32 microcontrollers handle real-time motor control loops, reading encoders, and communicating with the Pi.
3.  **Mechanical/Electrical**: 6 revolute joints, actuators, and position feedback sensors.

## Team Setup

Use one of the setup scripts from the repository root:

- Linux/macOS shell: `./setup.sh`
- PowerShell: `./setup.ps1`

These scripts:

- create `simulation/.venv` and install `simulation/requirements.txt`
- create `.tooling` and install local ROS CLI tools (`colcon`, `vcstool`, `rosdep`)
- install Renode for the headless firmware simulation workflow

Tracked project files use **repository-relative** paths only (e.g. `${workspaceFolder}` in VS Code, `ROOT_DIR` derived from script paths in shell helpers). If you **move or rename** your clone directory, rerun `./setup.sh` / `.\setup.ps1` so the gitignored Renode wrapper under `.tooling/bin/` points at the portable Renode install under `.tooling/renode/` for your new location.

Python virtual environments are used for simulation (`simulation/.venv`), ROS CLI tooling (`.tooling/`), and optional WireViz harness diagrams (`hardware/wireviz/.venv`).
The firmware C++ project uses PlatformIO, not a Python venv.

## Dev Helpers

Use one of the dev helpers from the repository root:

- Shell: `./dev.sh [up|down|shell|build|launch]`
- PowerShell: `./dev.ps1 [up|down|shell|build|launch]`

These commands run ROS with Docker Compose using the configuration in `software/docker-compose.yml`.
