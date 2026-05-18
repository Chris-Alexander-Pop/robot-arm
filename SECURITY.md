# Security Policy

This repository documents a **hobby / portfolio robotics project**, not a
production industrial robot. Do not use it for unattended operation or safety-critical
applications without independent risk assessment.

## Reporting

If you discover a security issue in project code or documentation, please open a
[GitHub Issue](https://github.com/Chris-Alexander-Pop/robot-arm/issues) with a
clear description. For sensitive reports, use GitHub private vulnerability reporting
if enabled on the repository.

## Known limitations

- **Serial command interface**: The STM32 firmware accepts motion commands over UART
  without authentication or encryption. Any host connected to the serial port can
  command the arm. Treat the link as a trusted bench connection only.
- **No remote attack surface by design**: There is no cloud API or WiFi control stack
  in the current tree; scope is local ROS + serial.

## Supported versions

Only the current `main` branch is maintained during active development. Tagged
releases will be added when checkpoint demos are ready.
