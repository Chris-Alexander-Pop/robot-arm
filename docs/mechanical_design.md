# Mechanical Design & Kinematics

## Requirements
* **Payload**: [To Be Determined]
* **Reach**: [To Be Determined]

## Actuators (Motors)
We are using a mix of stepper motors and servos to balance torque and weight:
* **Base/Shoulder (Joints 1 & 2)**: 2x NEMA 23 stepper motors. These are heavy but provide the immense torque required to lift the rest of the arm.
* **Elbow/Wrist (Joints 3 & 4)**: 2x NEMA 17 stepper motors. Lighter, but still capable of holding decent loads.
* **End-Effector (Joints 5 & 6)**: 2x NEMA 14 stepper motors or metal-gear servos. These need to be as light as possible to reduce the moment of inertia on the entire arm.

## Gearing & Power Transmission
* **Timing Belts**: GT2 timing belts and pulleys will be used. This allows us to mount the heavy motors (like the NEMA 17s for the elbow) closer to the base, transmitting power up the arm via belts. This drastically reduces the moving mass.

## Bearings
* **Base Joint**: A large thrust bearing (or slewing bearing) to support the entire axial load (weight) of the arm.
* **Other Joints**: 608ZZ (standard skate bearings) to take the radial and physical loads off the motor shafts. *Never* load a motor shaft directly!

## Manufacturing
* **Materials**: 3D Printed PETG is highly recommended due to its slight flex, high layer adhesion, and impact resistance. PLA+ or ABS are secondary alternatives.
* **Hardware**: Assorted M3/M4 bolts, nuts, and brass heat-set inserts to assemble the 3D-printed parts securely.
