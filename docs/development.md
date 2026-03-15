# Development & Procurement Strategy

To manage costs and complexity, the development of the 6-DOF robot arm is divided into three focused stages. This allows for iterative design, firmware testing, and mechanical verification without a large initial investment.

## Phase 1: Electronics MVP & Core Design
**Goal**: Establish the "brains" of the robot and verify mechanical mounting for the largest actuators.

### Hardware Purchases
*   **Compute**: 1x STM32 Microcontroller (Low-level), 1x Raspberry Pi 3 (High-level).
*   **Actuators (Get one of each for testing)**: 1x NEMA 23, 1x NEMA 17, 1x NEMA 14 (or servo). It is highly recommended to buy at least one of each motor size you plan to use before committing to the full BOM. This allows you to validate mounting dimensions, physical footprint, and motor driver capability on the bench.
*   **Drivers**: 1x TB6600 High-Current Driver (for NEMA 23), 1x TMC2209 (for NEMA 17).
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

## Appendix: Gear Reduction and Drive Systems
To convert high-speed, low-torque stepper output into the slow-speed, high-torque movement required by a robotic arm, gear reduction is essential. Below are the drive types you can consider, along with their 3D printability:

### 1. Cycloidal Drives
A cycloidal drive uses an eccentric bearing driving a lobed disc that rolls along an outer ring of pins. 
*   **Pros**: Extremely high gear reduction in a compact space (e.g., 20:1 up to 100+:1 in a single stage). Zero or near-zero backlash. Highly robust against shock loads.
*   **Cons**: Vibrations at high speeds. High friction limits efficiency. 
*   **3D Printability**: **High**. This is considered the ultimate DIY 3D-printable gearbox. Standard PETG or PLA+ works well, though you will need precise printer calibration and metal bearings/pins to ensure smooth operation.

### 2. Planetary Gearboxes
Consists of a central sun gear, multiple planet gears, and an outer ring gear.
*   **Pros**: Co-axial (input and output are on the same axis), good load distribution across multiple teeth, readily available in COTS (commercial off-the-shelf) form.
*   **Cons**: Medium to high backlash unless precision-machined (which is expensive). Harder to achieve very high reduction ratios (like 100:1) without compounding multiple stages, adding length to the joint.
*   **3D Printability**: **Medium/High**. You can easily 3D print herringbone planetary gears for decent performance, but backlash will always be present in a purely 3D-printed plastic planetary gearbox.

### 3. Harmonic / Strain Wave Drives
Uses a flexible spline (cup) that is deformed by an elliptical wave generator inside a circular spline.
*   **Pros**: Zero backlash. Fantastic reduction ratios in a tiny footprint. The industry standard for professional 6-DOF cobots.
*   **Cons**: Very expensive if bought commercially. Very difficult to manufacture.
*   **3D Printability**: **Low/Experimental**. While people have 3D-printed harmonic drives (usually using TPU or thin-walled PETG for the flex spline), the plastic flex splines degrade and fail quickly due to cyclic fatigue. It is not generally recommended for a load-bearing, reliable DIY arm.

### 4. Belt & Pulley Drive (GT2 Belts)
Using standard timing belts and printed pulleys to gear down between parallel shafts.
*   **Pros**: Very cheap, zero backlash (at proper tension), quiet, and allows moving the motors to the base of the robot to reduce the weight/inertia of the moving arm links (using compound pulley systems).
*   **Cons**: Belt stretch over time requires tensioning systems. Limited reduction ratios per stage (usually ~5:1 max before the large pulley gets too big).
*   **3D Printability**: **Very High**. You can easily print large HTD or GT2 pulleys. The belts themselves are cheap COTS items.

### Recommendation for 3D Printed Arm
For the heavy lifting **Base/Shoulder joints**, stick to **3D-printed Cycloidal drives** or buy cheap metal planetary gearboxes. For the **Elbow/Wrist**, utilize **Belt & Pulley drives** to transfer power from motors seated near the shoulder to keep the end of the arm lightweight.
