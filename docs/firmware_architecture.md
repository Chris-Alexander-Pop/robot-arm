# Firmware Architecture (STM32)

## Responsibilities
The STM32 acts as the spinal cord of the robot. Its sole job is real-time, deterministic control.
1. Receive target joint angles/velocities from the Raspberry Pi over wired Serial (UART/USB).
2. Read the absolute position of all 6 joints using the AS5600 magnetic encoders (likely over an I2C multiplexer since they have fixed I2C addresses).
3. Execute PID (Proportional-Integral-Derivative) control loops at >1000 Hz to calculate the required motor effort to reach the target angle.
4. Output the precise high-speed pulse trains (STEP/DIR) to the TB6600 and TMC2209 motor drivers.
5. Report the current actual joint positions back to the Raspberry Pi.

## Closed-Loop Stepper Control
Because we are using AS5600 encoders with stepper motors, the STM32 will essentially treat the steppers like high-pole-count brushless/servo motors. If the arm hits an obstacle or the payload is too heavy, the encoder detects the error, and the PID loop pushes more current or attempts to correct the position without losing its coordinate system (no "skipped steps" ruining the calibration).

## Communication Protocol
A simple serial packet format will be used between the STM32 and Raspberry Pi. For example: `[Header][CMD][Joint1...6 Targets][Checksum]`.
