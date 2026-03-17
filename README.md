# 6-DOF Robot Arm

A 6-Degree of Freedom (6-DOF) robotic arm project. This repository contains the mechanical CAD models, electrical schematics, firmware for the low-level microcontrollers, and high-level software/simulation code.

## Directory Structure

* `/docs` - Project design documents and specifications.
* `/cad` - Mechanical design files (SolidWorks/Fusion 360).
* `/firmware` - Low-level C/C++ code for ESP32/STM32 microcontrollers.
* `/software` - High-level control software for Raspberry Pi 3 (ROS 2, kinematics).
* `/simulation` - Local simulation environments and URDF models.

## Architecture Overview

1.  **High-Level Control (Software)**: A Raspberry Pi 3 handles inverse kinematics, trajectory planning, and overall orchestration.
2.  **Low-Level Control (Firmware)**: ESP32 or STM32 microcontrollers handle real-time motor control loops, reading encoders, and communicating with the Pi.
3.  **Mechanical/Electrical**: 6 revolute joints, actuators, and position feedback sensors.

## Team Setup

Use one of the setup scripts from the repository root:

- Linux/macOS shell: `./setup.sh`
- PowerShell: `./setup.ps1`

These scripts:

- create `simulation/.venv` and install `simulation/requirements.txt`
- create `.tooling` and install local ROS CLI tools (`colcon`, `vcstool`, `rosdep`)

## Dev Helpers

Use one of the dev helpers from the repository root:

- Shell: `./dev.sh [up|down|shell|build|launch]`
- PowerShell: `./dev.ps1 [up|down|shell|build|launch]`

These commands run ROS with Docker Compose using the configuration in `software/docker-compose.yml`.
