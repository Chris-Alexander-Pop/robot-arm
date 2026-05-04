<!--
This file stores analysis on similar 6-DOF robot arms to use as a reference,
analyzing their design choices, lessons learned, and direct applicability to this project.
-->

# Reference Arms & Design Examples

This document analyzes existing open-source and commercial 6-DOF robot arms that are directly relevant to this project's design choices. Each entry includes: what the arm does well, what its limitations are, and the specific lessons/components that map to this project.

---

## 1. Annin Robotics AR4 — The Gold Standard

**Links**: [annin.com](https://anninrobotics.com/) | [GitHub](https://github.com/Chris-Annin/AR4_Ver2)

**Specs**:
- 6 DOF, NEMA 23 steppers at base/shoulder, NEMA 17 at elbow/forearm, servos at wrist
- ~2.0 kg payload, ~650mm reach
- Planetary gearboxes at J1/J2/J3
- Controller: Custom Arduino-based; newer version supports ROS

**Why It's Valuable**:
The AR4 is the most complete, well-documented desktop 6-DOF arm in the open-source world. The manual documents **exact gear ratios**, **belt tension procedures**, and **calibration sequences** that represent years of iteration.

**Specific Lessons for This Project**:

| AR4 Solution | This Project's Approach | Notes |
|:---|:---|:---|
| Planetary gearboxes (metal, purchased) | 3D-printed cycloidal drives | AR4's gearboxes cost $80–150 each; cycloidals are ~$15 in materials |
| NEMA 23 at J1/J2 | NEMA 23 at J1/J2 | ✅ Same motor class — AR4 validates this is the right tier |
| Custom Arduino controller | STM32 + Raspberry Pi running ROS 2 | Similar two-tier split; better ecosystem on our side |
| ~10:1 planetary reduction | ~20:1 cycloidal reduction | More reduction headroom gives this project more safety margin |
| 2kg payload | 0.5–1.0 kg target | AR4 achieves 2kg; our softer gearing limits us — acceptable for desktop use |

**Key Takeaway**: The AR4 BOM is a useful sanity-check. If their gear ratio is 10:1 for a 2kg payload, our 20:1 ratio for a 0.5kg payload is **very conservatively over-engineered** — which is correct for a first build.

---

## 2. BCN3D Moveo — Belt Drive Masterclass

**Links**: [BCN3D GitHub](https://github.com/BCN3D/BCN3D-Moveo)

**Specs**:
- 6 DOF, fully 3D-printed
- NEMA 23 at shoulder, NEMA 17 at elbow, NEMA 17/14 at wrist
- **All joints use GT2 belt drives** — no gearboxes
- Controller: Arduino MEGA + RAMPS 1.4

**Why It's Valuable**:
The Moveo proves the **belt-drive-only architecture** for a 6-DOF arm. Every joint uses timing belts to transmit power, with motors mounted parallel to links to minimize the moving inertia. The CAD files are available and show exactly how belts are tensioned, how idler pulleys are mounted, and how the arm shell is structured.

**Specific Lessons for This Project**:

| Moveo Solution | Applicability | Notes |
|:---|:---|:---|
| Motor mounted parallel to upper arm (J3) | ✅ Directly applicable | Copy this mounting strategy for J3 NEMA 17 |
| GT2 6mm belt tensioning via slotted motor mounts | ✅ Same approach | Slotted holes in PETG motor plate allow belt tension adjustment |
| 16T motor → 48T joint = 3:1 reduction | ✅ Reference ratio for J3 | 3:1 achieves ~2.4 Nm at elbow |
| 608ZZ bearings at all belt idler pulleys | ✅ Same hardware | Already in BOM |
| Open-loop steppers → arm loses calibration if steps missed | ❌ Problem to avoid | Solved in this project by closed-loop CL57T/CL42T kits |
| Arduino MEGA controller | ❌ Too slow for ROS 2 | STM32 + Pi is strictly better |

**What to Actually Download**: The **STEP/STL files** from GitHub. Study the belt routing and pulley alignment in the elbow and shoulder assemblies. The Moveo's elbow joint routing inside the link is directly adaptable to our upper arm design.

---

## 3. Niryo Ned 2 — Commercial Reference for Wrist Design

**Links**: [niryo.com](https://niryo.com)

**Specs**:
- 6 DOF, NEMA 17 at base/mid-arm, **Dynamixel XL430 servos at wrist/end-effector**
- ~300g payload, ~440mm reach
- Designed for education and light automation

**Why It's Valuable**:
Niryo explicitly chose **Dynamixel servos for J4–J6** (wrist) specifically to **save weight at the distal joints** — the same motivation behind our NEMA 14 choice. The Ned 2 serves as a commercial validation of this "heavy base, light tip" philosophy.

**Specific Lessons for This Project**:

| Niryo Design Choice | This Project's Equivalent | Notes |
|:---|:---|:---|
| Dynamixel XL430 at wrist (71g, 1.4 Nm) | NEMA 14 14HS10-0404S (130g, 0.14 Nm) | Dynamixel is 2× lighter but 10× more expensive ($50/unit) |
| Parallel-jaw gripper with servo | MG996R servo-actuated gripper | Same concept; MG996R is a low-cost substitute |
| Raspberry Pi as compute unit | Raspberry Pi 3 | ✅ Same compute platform |
| ROS 2 + MoveIt for planning | ROS 2 + MoveIt 2 | ✅ Same software stack |
| 300g payload limit | 0.5–1.0 kg target | Our larger motors and gearing allow heavier loads |

**Key Takeaway**: The Niryo demonstrates that a Raspberry Pi running ROS 2 + MoveIt is completely viable for a 6-DOF arm at this scale. If Niryo can productize it, we can implement it.

---

## 4. Thor Robot Arm — 3D-Printed Planetary Gears

**Links**: [Hackaday.io Project](https://hackaday.io/project/12989-thor) | [GitHub](https://github.com/AngelLM/Thor)

**Specs**:
- 6 DOF, entirely 3D-printed including the **planetary gearboxes**
- NEMA 23 at base, NEMA 17 at elbow/forearm, NEMA 17 at wrist
- Custom ROS-based controller

**Why It's Valuable**:
Thor proves that **3D-printed gearboxes** — in this case, planetary — can withstand the torques of a desktop arm. The project documents the critical printer tolerances required for the planet gears to mesh without binding.

**Specific Lessons for This Project**:

| Thor Solution | Applicability | Notes |
|:---|:---|:---|
| 3D-printed planetary gears | ❌ We chose cycloidal instead | Cycloidals are simpler to print (disk + pins vs. multiple planet gears) |
| Printer tolerance requirements documented | ✅ Relevant | Their finding: planet gears need ≤ 0.2mm tolerance. Cycloidal disks similarly need careful clearances. |
| NEMA 23 at the base | ✅ Same | Confirms motor class |
| Steel pins / inserts in high-stress points | ✅ Directly applicable | Thor uses steel rods to reinforce critical PETG joints — same as our heat-set inserts + ring pins |

**What to Study**: The Thor build log documents **what failed first** during testing — primarily tooth deformation on the planetary gears under load. These failure modes map to PETG cycloidal disk wear in our design → the PTFE grease strategy is the mitigation.

---

## 5. Universal Robots UR5e — Professional Reference

**Links**: [Universal Robots](https://www.universal-robots.com/products/ur5-robot/)

**Specs**:
- 6 DOF, all-brushless servo motors with harmonic drives
- **5 kg payload**, **850mm reach**
- Repeatability: **±0.03mm**

**Why Reference a $35,000 Robot?**

The UR5e is the professional baseline. It defines what the architecture of a modern robot arm **should** look like at a fundamental level:

| UR5e Design Decision | This Project's Approximation | Why We Can't Fully Match It |
|:---|:---|:---|
| Harmonic drive at every joint (~50:1) | Cycloidal at J1/J2 (~20:1), belt at J3 | Harmonic drives cost $300–$800 each |
| Brushless servo motors at every joint | Closed-loop steppers at J1–J4, open-loop at J5/J6 | True servos are far more expensive |
| ±0.03mm repeatability | ±0.5–1.0mm target | PETG compliance, belt stretch, cycloidal play |
| Aluminum link structure | 3D-printed PETG | Weight vs. cost trade-off |
| Integrated cable management | Drag chain + PET sleeving | External cable management |

**Key Takeaway**: The UR5e's architecture **validates the choices** of harmonic drives, brushless servos, and aluminum structure as the "correct" answer — our project is a budget approximation of the same structural logic. Understanding where we diverge from it helps set realistic expectations about the repeatability and payload limits we can achieve.

---

## 6. Summary Comparison

| Arm | Payload | Reach | Gearing | Controller | Cost | Open Source |
|:---|:---:|:---:|:---|:---|:---:|:---:|
| **This Project** | 0.5–1 kg | 630mm | Cycloidal + belt | STM32 + RPi3 + ROS 2 | ~$700 CAD | ✅ |
| AR4 | 2.0 kg | 650mm | Planetary | Arduino + ROS | ~$1,500 USD | ✅ |
| BCN3D Moveo | ~0.5 kg | 550mm | Belt only | Arduino MEGA | ~$500 USD | ✅ |
| Niryo Ned 2 | 0.3 kg | 440mm | Belt + direct | RPi + ROS 2 | ~$2,000 USD | Partial |
| Thor | ~1.0 kg | 600mm | 3D-printed planetary | Custom + ROS | ~$800 USD | ✅ |
| UR5e | 5.0 kg | 850mm | Harmonic drives | Proprietary | ~$35,000 USD | ✗ |