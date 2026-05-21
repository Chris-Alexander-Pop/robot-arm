# Mechanical Roadmap

> **Drive design:** J3–J4 use **cycloidal reducers** on driven joints (see [`docs/Design_Choices.md`](../docs/Design_Choices.md)). The belt-drive item in INTEGRATION below is legacy brainstorming unless you explicitly choose belts for the wrist.

This document outlines the mechanical design, CAD, 3D printing configurations, and assembly instructions.

## DEVELOPMENT
*Prototyping, dimension verification, and testing print tolerances - before full BOM procurement.*

- [ ] **Gearbox Prototyping**: Test print and validate gear reduction designs (e.g., Cycloidal, Planetary, or Harmonic) for backlash, tolerances, and friction.
- [ ] **Material Testing**: Calibrate PETG/ABS print settings for strength, layer adhesion, and dimensional accuracy for heat-set inserts.
- [ ] **Motor Fitment Tests**: Print isolated mounts for NEMA 23, NEMA 17, and Servos to physically verify bolt holes, shaft lengths, and alignments using prototype motors.
- [ ] **Bearing Tolerances**: Print test gauges to verify press-fit tolerances for 608ZZ bearings and thrust bearings.
- [ ] **Base & Joint 1 CAD**: Design the high-torque base housing and thrust bearing integration.
- [ ] **Shoulder (Joint 2) CAD**: Design shoulder linkage and structural reinforcement for vertical lift.

## INTEGRATION
*Final prints, sourcing, and assembly - after purchasing all hardware.*

- [ ] **Mass Printing**: Print all structural arm sections, linkages, and housings with high infill/wall count.
- [ ] **Hardware Sourcing**: Procurement of all M3/M4 bolts, nuts, bearings, belts, and brass inserts.
- [ ] **Sub-assembly - Base & Shoulder**: Install bearings, press-fit motors, and assemble the primary lifting joints.
- [ ] **Wrist / upper-arm drives**: Install cycloidal stages per current CAD (legacy note: GT2 belts were an earlier concept — use cycloidal design unless intentionally changed).
- [ ] **Cable Management**: Route power, encoder, and motor cables through the internal pathways of the linkages.
- [ ] **End-Effector Integration**: Attach the 2-DOF wrist and modular gripper.
- [ ] **Static Stress Test**: Verify the fully assembled arm doesn't deflect significantly or back-drive under its own weight when powered off.
