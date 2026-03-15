# Development & Procurement Strategy

To manage costs and complexity, the development of the 6-DOF robot arm is divided into three focused stages. This allows for iterative design, firmware testing, and mechanical verification without a large initial investment.

## Phase 1: Electronics MVP & Core Design
**Goal**: Establish the "brains" of the robot and verify mechanical mounting for the largest actuators.

### Hardware Purchases
*   **Compute**: 1x STM32 Microcontroller (Low-level), 1x Raspberry Pi 3 (High-level).
*   **Actuator**: 1x NEMA 23 Stepper Motor.
*   **Driver**: 1x TB6600 High-Current Driver.
*   **Feedback**: 1x AS5600 Magnetic Encoder.
*   **Power**: 24V 5A-10A Power Supply.

### Objectives
*   Configure the serial communication protocol between the Raspberry Pi and STM32.
*   Implement closed-loop control on the STM32 using the AS5600 encoder.
*   **Mechanical Verification**: Design and 3D print the Base/Shoulder joint housing. Use the physical NEMA 23 to verify bolt patterns, shaft alignment, and clearance.

---

## Phase 2: Mechanical Prototyping
**Goal**: Validate the design of the remaining joints and the belt-drive transmission system.

### Hardware Purchases
*   **Filament**: 1-2 spools of PETG (recommended for strength).
*   **Bearings**: Standard 608ZZ skate bearings (set of 8-10).
*   **Drivetrain**: GT2 Timing Belts and Pulleys.
*   **Fasteners**: Assorted M3/M4 bolts, nuts, and brass heat-set inserts.

### Objectives
*   Print and assemble the Elbow and Wrist joints.
*   Test belt tensioning and power transmission from the base up to the upper arm.
*   Verify tolerances of 3D-printed parts with heat-set inserts and bearings.

---

## Phase 3: Full Assembly & Integration
**Goal**: Complete the 6-DOF arm and implement full kinematic motion.

### Hardware Purchases
*   **Remaining Motors**: 1x NEMA 23, 2x NEMA 17, 2x NEMA 14 (or servos).
*   **Remaining Drivers**: Mix of TB6600 and TMC2209.
*   **Full Power System**: Upgrade to 24V 20A Switching Power Supply.
*   **Feedback**: 5x additional AS5600 Magnetic Encoders.

### Objectives
*   Final assembly of all 6 joints.
*   Wiring management and custom PCB/breakout board implementation.
*   Inverse kinematics (IK) testing and path planning using ROS 2 on the Raspberry Pi.
