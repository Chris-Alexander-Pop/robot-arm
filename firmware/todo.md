# Firmware Roadmap

This folder contains the low-level real-time control code for the STM32 microcontroller.

## Epic: Core Controller Infrastructure
- [ ] **Serial Protocol**: Define and implement a robust UART/USB command protocol between RPi and STM32.
- [ ] **Stepper Core**: Implement precise timer-based pulse generation for stepper drivers (TB6600/TMC2209).
- [ ] **State Machine**: Implement a robust state machine (IDLE, MOVING, CALIBRATING, ERROR).

## Epic: Closed-Loop Control (Joint 1 & 2 Focus)
- [ ] **I2C Encoder Driver**: Integrate AS5600 magnetic encoders for absolute position feedback.
- [ ] **PID Position Loop**: Implement a control loop that corrects for missed steps using encoder data.
- [ ] **Homing Logic**: Implement homing routines using limit switches or physical hard-stops.

## Epic: Multi-Axis Sync & Safety
- [ ] **6-Axis Orchestration**: Synchronize movement across all 6 joints for smooth trajectories.
- [ ] **Safety Interlocks**: Implement current sensing and emergency stop (E-Stop) interrupts.
- [ ] **EEPROM Storage**: Save calibration offsets and limit settings to non-volatile memory.
