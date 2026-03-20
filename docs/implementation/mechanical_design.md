# Mechanical Design & Kinematics

<!--
This document details the mechanical architecture, actuator selection, power transmission strategy,
structural materials, and kinematic approach for the 6-DOF robot arm.
-->

## 1. Design Goals & Payload Estimate

The arm is designed for **desktop-scale pick-and-place operations**, targeting an estimated payload of **0.5 – 1.0 kg** at the end-effector. The total arm reach is targeted at **~500mm** (from shoulder to wrist) based on the link-length breakdown below. These targets will be validated in simulation before final hardware procurement.

---

## 2. Actuator Selection

The motor stack is split into three tiers based on joint position and torque requirement. Mounting heavier motors closer to the base is a deliberate strategy to minimize the **moving moment of inertia** — the further mass sits from the base pivot, the more energy is wasted just swinging the arm itself.

| Joint | Description | Motor | Driver | Torque | Rationale |
|:---|:---|:---|:---|:---|:---|
| J1 | Base Rotation | NEMA 23 (2.0 Nm) | CL57T Closed-Loop | 2.0 Nm native | Base must spin the full weight of the arm |
| J2 | Shoulder Pitch | NEMA 23 (2.0 Nm) | CL57T Closed-Loop | 2.0 Nm native | Worst-case torque joint — lifts everything upstream |
| J3 | Elbow Pitch | NEMA 17 (80 Ncm) | CL42T Closed-Loop | 0.80 Nm native | Mid-arm, moderate load; kept light to reduce J2 duty |
| J4 | Forearm Twist | NEMA 17 (42 Ncm) | CL42T Closed-Loop | 0.42 Nm native | Rotates forearm; low torque, lower inertia priority |
| J5 | Wrist Pitch | NEMA 14 (14 Ncm) | TMC2209 Open-Loop | 0.14 Nm native | Must be ultra-lightweight — any mass here multiplies elbow torque |
| J6 | Tool Roll | NEMA 14 (14 Ncm) | TMC2209 Open-Loop | 0.14 Nm native | Same rationale as J5 |

> **Note on Closed-Loop Kits**: J1–J4 use integrated closed-loop stepper kits (e.g. `1-CL57T-S20-V41`, `1-CL42T-S08-V41`, `1-CL42T-S04-V41`) which include the motor, matched driver, and encoder wiring. This eliminates the need for a separate AS5600 encoder on those joints; the kit's built-in encoder provides the position feedback the driver uses internally.

---

## 3. Gearing & Power Transmission

Raw motor torque is insufficient for J1 and J2 — a 2.0 Nm NEMA 23 holding a 0.5m arm with a 1 kg payload needs ~5 Nm at the shoulder pivot. The chosen solution is a **Cycloidal Drive** at J1 and J2.

### 3a. Cycloidal Drives (J1 & J2)

A cycloidal drive is a compact, high-ratio speed reducer. Its key advantages over a planetary gearbox for this project are:
- **3D Printable**: The ring pins are hardened steel dowel pins (3–5mm); the cycloidal disk and ring housing are PETG. No custom metal machining required.
- **High Ratio in Small Space**: Achieves **~20:1 to 30:1** reduction in a single stage, giving J1/J2 an effective torque of **40–60 Nm** — far more than enough for the payload target.
- **Near-Zero Backlash**: Great for precise position control compared to standard spur gears.

**Key Components:**
- **Eccentric cam**: `608ZZ` bearings press-fit onto the motor shaft offset by ~1mm
- **Cycloidal disk**: 3D printed PETG with epitrochoidal tooth profile
- **Ring gear**: Hardened steel dowel pins (4mm) press-fit into the housing — prevents shearing under load
- **Output bearings**: `6806` or `6808` thin-section bearings for structural load

> **Lubrication is mandatory**: Synthetic PTFE grease must be packed into the cycloidal assembly. Without it, friction will melt the PETG disk during sustained operation.

### 3b. Folded / Parallel-Axis Belt Drive (J2/J3 Option)

As an alternative (or complementary) strategy, **GT2 timing belt** drive is used on mid-arm joints. By mounting the NEMA 17 motor parallel to the upper arm link instead of at the joint, we:
- Move motor mass closer to the base/shoulder, reducing the elbow's effective moment of inertia
- Keep the joint profile slim
- Achieve a 2:1–3:1 additional reduction using mismatched pulleys (e.g., 16T motor pulley → 36T joint pulley)

**Components**: GT2 6mm belt, 36T idler pulleys with 8mm bore, tensioned via adjustable motor mounting slots.

---

## 4. Structural Materials & Manufacturing

### 4a. Primary Structure: 3D-Printed PETG
PETG is chosen over PLA or ABS for the following reasons:

| Property | PLA | PETG | ABS |
|:---|:---|:---|:---|
| Glass Transition Temp | ~60°C | **~80°C** | ~100°C |
| Layer Adhesion | Good | **Excellent** | Fair |
| Warp Resistance | Good | **Excellent** | Poor |
| Impact Resistance | Brittle | **Good flex** | Good |
| Post-print difficulty | Easy | **Easy** | Hard (fumes, warp) |

Motors generate significant heat. PETG's 80°C glass transition temperature provides headroom that PLA simply does not.

### 4b. Fasteners & Inserts
- **M3 / M4 / M5 Alloy Steel Bolts & Nuts**: Standard metric hardware for all structural connections
- **M3×5mm Brass Heat-Set Inserts**: Press-in with a soldering iron; creates a permanent metallic threaded boss inside PETG that withstands high-torque motor mounting without stripping
- **Blue Loctite**: Applied to all motor mount fasteners to prevent vibration-induced loosening

### 4c. Bearings
- **Base Joint (J1)**: Large thin-section `6806` or `6808` bearing to carry the full axial load of the arm. The motor shaft should **never** carry this structural load directly.
- **Cycloidal Input**: `608ZZ` bearings as the eccentric cam elements
- **All Other Joints**: `608ZZ` or similar deep-groove bearings to absorb radial forces — motor shafts are only coupled for torque, never for structural support

---

## 5. End-Effector
- **Gripper Actuator**: `MG996R` metal-gear micro servo for the claw mechanism. Small, fast, and well-supported by existing ROS 2 packages.
- **Optional Force Feedback**: A **Force Sensitive Resistor (FSR)** on the gripper fingertips can be integrated for a closed-loop grip-force PID, preventing crushing of delicate objects.

---

## 6. Denavit-Hartenberg (DH) Parameters

The arm's kinematics are parameterized using the **Modified DH convention**. These parameters define the geometric relationship between each consecutive joint frame and are used directly in both the Python IK/FK solver and the URDF model.

| Joint | a (mm) | α (deg) | d (mm) | θ (variable) | Notes |
|:---:|:---:|:---:|:---:|:---:|:---|
| J1 | 0 | 90 | d1 | θ1 | Base rotation; d1 = height of base |
| J2 | L1 | 0 | 0 | θ2 | Shoulder pitch; L1 = upper arm length |
| J3 | L2 | 90 | 0 | θ3 | Elbow pitch; L2 = forearm length |
| J4 | 0 | -90 | d4 | θ4 | Forearm twist |
| J5 | 0 | 90 | 0 | θ5 | Wrist pitch |
| J6 | 0 | 0 | d6 | θ6 | Tool roll; d6 = flange offset |

> **L1, L2, d1, d4, d6** are physical link lengths determined by the CAD model. These will be updated once the first prototype joints are dimensioned.
