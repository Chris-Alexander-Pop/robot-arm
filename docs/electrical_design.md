# Electrical Design

## Power System
* **Main Power Supply**: 24V, 15A or 20A switching AC/DC power supply. 24V is critical for steppers to maintain torque at higher speeds. The high current rating is needed to handle the spike when all 6 actuators move simultaneously.
* **Logic Power**: Step-down buck converters to drop 24V to 5V (for the Raspberry Pi) and 3.3V/5V for the STM32 and sensors.

## Compute
* **High-Level Brain**: Raspberry Pi 3 (Handles ROS 2, Kinematics, Path Planning).
* **Low-Level Controller**: STM32 Microcontroller (Handles real-time stepping and PID loops). It connects directly to the Raspberry Pi over a wired serial connection (UART or USB).

## Motor Drivers
* **NEMA 23 Drivers**: TB6600. These are bulkier but can handle the higher current draw (up to ~4A) required by the NEMA 23s.
* **NEMA 17 & 14 Drivers**: TMC2209. Extremely quiet (StealthChop), smooth, and easily integrated for lower-power steppers.

## Sensors & Feedback
* **Position Encoders**: 6x AS5600 magnetic encoders. These provide 12-bit absolute position feedback. With these, the STM32 can implement a closed-loop control system, automatically correcting if a stepper skips a step.
* **Homing**: Limit switches can be used for absolute zero-position calibration at startup.

## Wiring
* Spools of flexible silicone wire (reduces resistance and prevents wire fatigue as the arm moves).
* JST connectors and heat shrink.
* A custom PCB or protoboard acting as a "breakout shield" to neatly route the STM32 pins to the 6 motor drivers and 6 encoders.
