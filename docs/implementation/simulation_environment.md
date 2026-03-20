# Simulation Environment

<!--
This document explains the simulation stack: the URDF model, the physics engine,
the MoveIt 2 integration, and the workflow for validating the arm before hardware exists.
-->

## 1. What is a Robot Simulation?

A simulation is a virtual 3D environment where a **mathematically and physically accurate digital twin** of the arm lives. Instead of sending motor commands to the physical STM32 (and risking a crash, a broken 3D-printed joint, or a runaway motor), you send those exact same ROS 2 commands to the virtual robot.

The simulation environment is valuable at **every phase** of development:
- **Before hardware exists**: Write and test 90% of the high-level Python/C++ code entirely on a laptop
- **During mechanical iteration**: Validate DH parameters and link lengths against CAD before printing
- **During software tuning**: Tune MoveIt trajectory parameters, PID gains, and task logic in simulation before risking the physical arm
- **As a regression harness**: Re-run standardized task scripts against the simulation after any firmware or software change to confirm nothing broke

---

## 2. Stack Components

### 2a. URDF (Unified Robot Description Format)

The **URDF** is an XML file that is the single source of truth for the arm's geometry. Everything — MoveIt, Gazebo, RViz, the `robot_state_publisher` — reads this file.

**What it defines:**

```xml
<robot name="robot_arm">

  <!-- A "link" is a rigid body (a physical link of the arm) -->
  <link name="upper_arm">
    <visual>
      <geometry><mesh filename="upper_arm.stl"/></geometry>
    </visual>
    <inertial>
      <mass value="0.45"/>  <!-- kg, from CAD -->
      <inertia ixx="..." ixy="0" ixz="0" iyy="..." iyz="0" izz="..."/>
    </inertial>
    <collision>
      <geometry><mesh filename="upper_arm_collision.stl"/></geometry>
    </collision>
  </link>

  <!-- A "joint" connects two links and defines the allowed motion -->
  <joint name="shoulder_pitch" type="revolute">
    <parent link="base_link"/>
    <child link="upper_arm"/>
    <axis xyz="0 1 0"/>    <!-- rotation axis in child frame -->
    <limit lower="-1.57" upper="1.57" effort="50" velocity="3.14"/>
    <origin xyz="0 0 0.12" rpy="0 0 0"/>  <!-- offset from parent -->
  </joint>

</robot>
```

Key attributes per link:
- **Visual mesh**: The `.stl` file exported from Fusion 360 / FreeCAD — what RViz renders
- **Collision mesh**: A simplified convex hull of the visual mesh for fast collision checks in MoveIt
- **Inertial properties**: Mass and inertia tensor, assigned from CAD material properties — critical for Gazebo to simulate realistic dynamics

The URDF is generated from a **xacro** (XML macro) file in practice, which allows parameterized values (link lengths, joint limits) to be set from a single `config.yaml` instead of hard-coded throughout the XML.

### 2b. Gazebo (Physics Simulation)

**Gazebo** (or Gazebo Ignition for newer ROS 2 / Humble setups) is the 3D rigid-body physics engine. It simulates:
- **Gravity** on each link based on its mass
- **Contact forces** and collisions between the arm and objects in the environment (table, target objects, walls)
- **Motor dynamics**: Via the `gazebo_ros2_control` plugin, Gazebo simulates the joint effort (torque) delivery in response to position/velocity commands

Gazebo listens for `ros2_control` joint commands on the normal ROS 2 topics and publishes simulated joint states back — making it **transparent to the rest of the software stack**.

### 2c. RViz2 (Visualization)

RViz is the ROS 2 3D visualization tool. It does **not** simulate physics — it just renders:
- The arm's current pose using `robot_state_publisher` transforms
- The planned trajectory (motion arc in 3D space)
- The collision environment (bounding boxes for the table, objects)
- The end-effector target pose (interactive marker)

For rapid kinematic testing (checking IK solutions, visualizing DH parameters) without launching the full Gazebo stack, **RViz alone** is sufficient.

### 2d. MoveIt 2

MoveIt is the motion planning framework. It interfaces with both the simulated Gazebo arm and the real physical arm **through the exact same ROS 2 action interface**. See `software_architecture.md` for the full MoveIt pipeline description.

Within the simulation context, the key MoveIt configuration files are:

| File | Purpose |
|:---|:---|
| `kinematics.yaml` | Selects IK solver (KDL or TRAC-IK) and convergence tolerances |
| `joint_limits.yaml` | Per-joint position, velocity, and acceleration limits |
| `ompl_planning.yaml` | OMPL planner selection (RRTConnect, RRTstar, etc.) |
| `robot_arm.srdf` | Semantic Robot Description — defines the `"arm"` planning group, end-effector, and pre-defined poses (home, ready) |

---

## 3. Python Kinematic Prototyping (`/simulation/`)

Before the full ROS 2/Gazebo stack is needed, a **pure Python DH solver** (`/simulation/kinematics.py`) provides rapid, dependency-light kinematic visualization.

### Forward Kinematics Script

```python
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def dh_matrix(a, alpha, d, theta):
    """Modified DH transformation matrix for one joint."""
    ct, st = np.cos(theta), np.sin(theta)
    ca, sa = np.cos(alpha), np.sin(alpha)
    return np.array([
        [ct,       -st,       0,       a      ],
        [st*ca,    ct*ca,    -sa,     -sa*d   ],
        [st*sa,    ct*sa,     ca,      ca*d   ],
        [0,        0,         0,       1      ]
    ])

# DH parameters: [a, alpha, d, theta_offset]
DH_PARAMS = [
    [0,    np.pi/2,  0.12,  0],   # J1 - Base
    [0.28, 0,        0,     0],   # J2 - Shoulder
    [0.25, np.pi/2,  0,     0],   # J3 - Elbow
    [0,   -np.pi/2,  0.06,  0],   # J4 - Forearm
    [0,    np.pi/2,  0,     0],   # J5 - Wrist
    [0,    0,        0.05,  0],   # J6 - Tool
]

def forward_kinematics(joint_angles_deg):
    T = np.eye(4)
    joint_origins = [T[:3, 3].copy()]

    for i, (params, angle_deg) in enumerate(zip(DH_PARAMS, joint_angles_deg)):
        a, alpha, d, theta_offset = params
        theta = np.radians(angle_deg) + theta_offset
        T = T @ dh_matrix(a, alpha, d, theta)
        joint_origins.append(T[:3, 3].copy())

    return joint_origins, T

# Example: arm at 45° shoulder, 30° elbow, rest at 0
origins, T_end = forward_kinematics([0, 45, 30, 0, 0, 0])
print(f"End-effector position: {T_end[:3, 3]}")
# → End-effector position: [x, y, z] in meters
```

This script is used to:
1. Sanity-check DH parameters immediately after updating link lengths from CAD
2. Generate 3D matplotlib plots of the arm for the /docs/ and initial design reviews
3. Prototype IK algorithms before translating to C++ for the production solver

---

## 4. Workflow: From Simulation to Hardware

```
[CAD Model (Fusion 360)]
        ↓  Export .stl meshes + measure link lengths
[Update DH parameters in Python script]
        ↓  Validate joint positions visually in matplotlib
[Write/update URDF xacro with measured dimensions]
        ↓  Load in RViz; check visual appearance and joint axes
[Launch Gazebo simulation]
        ↓  Run MoveIt task scripts against simulated arm
        ↓  Verify IK solutions, collision avoidance, trajectory smoothness
[Flash STM32 firmware; bring up hw_interface_node]
        ↓  Run identical MoveIt task scripts against real arm
        ↓  Compare joint telemetry vs. simulation to validate physical model
[Iterate: update URDF/DH params if physical and simulated behavior diverge]
```

The goal is to reach a state where the **simulation is accurate enough** that a task validated in simulation runs on the physical arm with **no code changes** — only the `ros2_control` hardware plugin is swapped.
