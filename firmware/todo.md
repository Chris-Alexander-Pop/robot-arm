# Firmware Roadmap

This document outlines the low-level real-time control code for the STM32 microcontroller.

## DEVELOPMENT
*Testing, driver validation, and core logic - to be completed with prototype hardware (one of each motor/driver) before the final build.*

- [ ] **Serial Protocol**: Define and implement a robust UART/USB command protocol between RPi and STM32.
- [ ] **Stepper Core**: Implement precise timer-based pulse generation for stepper drivers (TB6600/TMC2209) on a single test axis.
- [ ] **State Machine**: Implement a robust state machine (IDLE, MOVING, CALIBRATING, ERROR).
- [ ] **I2C Encoder Driver**: Integrate AS5600 magnetic encoders to read absolute position.
- [ ] **PID Position Loop**: Implement a basic control loop that corrects for missed steps using encoder data on a single prototype joint.
- [ ] **Homing Logic**: Draft homing routines using limit switches or physical hard-stops.
- [ ] **Driver Validation**: Validate that the STM32 can reliably drive both NEMA 23 (via TB6600) and NEMA 17 (via TMC2209) at varying speeds.

## INTEGRATION
*Full system implementation - to be completed after all hardware is purchased and assembled.*

- [ ] **Multi-Axis Sync**: Synchronize movement across all 6 joints for smooth trajectories using Bresenham's or similar multi-axis algorithms.
- [ ] **Global Error Handling**: Implement cross-axis safety (e.g., if Joint 2 stalls, halt all joints).
- [ ] **Safety Interlocks**: Implement current sensing and emergency stop (E-Stop) hardware interrupts.
- [ ] **EEPROM Storage**: Save calibration offsets, soft limits, and PID tunings to non-volatile memory for all 6 axes.
- [ ] **Tuning & Optimization**: Tune PID loops for all joints under active load and gravity compensation.
- [ ] **Communication Stress Test**: Validate high-speed command streaming from the Raspberry Pi during complex continuous movements.
