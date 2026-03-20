# Software Architecture (Raspberry Pi 3)

<!--
This document details the high-level software stack running on the Raspberry Pi 3:
OS, ROS 2 infrastructure, MoveIt 2 pipeline, hardware interface, and user-facing APIs.
-->

## 1. Operating System & Environment

- **OS**: Ubuntu 22.04 Server (64-bit) on Raspberry Pi 3
- **ROS 2 Distribution**: ROS 2 Humble Hawksbill (LTS, supported through May 2027)
- **Deployment**: All ROS 2 nodes run inside **Docker containers** managed by `docker-compose`. This keeps the host OS clean, makes the environment fully reproducible, and simplifies switching between simulation and hardware modes.

```
Host (Ubuntu 22.04)
└── docker-compose
    ├── ros2_core     (ROS 2 Humble base image)
    ├── moveit2       (MoveIt 2 planner + URDF)
    └── hw_interface  (Serial bridge node → STM32)
```

**Why Docker on Pi?** Pinning ROS 2 to a container image eliminates dependency conflicts and makes it trivial to rebuild the environment after an SD card failure. The `dev.sh` / `setup.sh` scripts at the repo root handle container lifecycle.

---

## 2. ROS 2 Node Graph

The following nodes make up the software stack:

```
/robot_description      (robot_state_publisher)
         ↓
/joint_states           (published by hw_interface_node)
         ↓
/move_group             (MoveIt 2 — trajectory planner)
    ↑           ↓
/goal_pose          /follow_joint_trajectory (action)
                        ↓
              /hw_interface_node  (serial bridge)
                        ↓
                    [ STM32 UART ]
                        ↓
                    [ Motors ]
```

### 2a. `robot_state_publisher`
- Reads the URDF from the `/robot_description` parameter
- Computes and broadcasts all link transforms using the current `/joint_states`
- Required by RViz and MoveIt 2 for visualization and collision checking

### 2b. `hw_interface_node`
- Implements a `ros2_control` `SystemInterface` hardware plugin
- Opens the STM32 serial port (e.g. `/dev/ttyUSB0` at 115200 baud)
- **Write loop**: Subscribes to `/follow_joint_trajectory` action goals; sends `SET_JOINTS` packets to STM32 at the required timestep rate
- **Read loop**: Periodically sends `GET_STATUS` to STM32; publishes parsed joint angles to `/joint_states`
- **Fault handling**: On receiving a `FAULT` packet, publishes to `/arm_fault` topic and cancels the active trajectory action

### 2c. MoveIt 2 (`move_group` node)
MoveIt 2 is the motion planning backbone. Its pipeline for a single pick-and-place move:

1. **Goal Input**: Receives a `PoseStamped` (XYZ + quaternion) from a Python script or RViz GUI
2. **IK Solving**: Uses the **KDL** or **TRAC-IK** solver to find a set of 6 joint angles that place the end-effector at the goal pose
3. **Planning**: Uses **OMPL (RRTConnect or RRTstar)** to plan a collision-free joint-space trajectory from current → goal
4. **Trajectory Parameterization**: Applies **trapezoidal** or **S-curve** velocity/acceleration limits to make the trajectory time-optimal but smooth
5. **Execution**: Sends the parameterized trajectory to the `hw_interface_node` via `FollowJointTrajectory` action

### 2d. High-Level Task Script (`pick_and_place_node`)
A Python node that defines the application logic:

```python
# Pseudocode sketch
moveit = MoveGroupInterface("arm")
moveit.set_max_velocity_scaling_factor(0.5)

# 1. Move to pre-grasp pose (above the object)
moveit.set_pose_target(pre_grasp_pose)
moveit.go(wait=True)

# 2. Open gripper
gripper_client.send_goal(GripperCommand(position=0.08, max_effort=10.0))

# 3. Move down to grasp
moveit.set_pose_target(grasp_pose)
moveit.go(wait=True)

# 4. Close gripper
gripper_client.send_goal(GripperCommand(position=0.0, max_effort=20.0))

# 5. Lift and move to place location
moveit.set_pose_target(place_pose)
moveit.go(wait=True)
```

---

## 3. Kinematics

### 3a. Forward Kinematics (FK)
FK answers: *"Given these 6 joint angles, where is the end-effector?"*

Using the **Modified DH convention**, the transform from base frame to end-effector is:
```
T_0_6 = T_0_1 × T_1_2 × T_2_3 × T_3_4 × T_4_5 × T_5_6
```
Each `T_i_j` is a 4×4 homogeneous transformation matrix computed from the DH parameters. The Python script in `/simulation/` implements this as a pure `numpy` calculation for rapid prototyping and sanity checking.

### 3b. Inverse Kinematics (IK)
IK answers: *"What 6 joint angles produce this end-effector pose?"*

Two approaches are used:

| Approach | Tool | Use Case |
|:---|:---|:---|
| **Analytical** | Custom Python (DH algebra) | Fast, deterministic, but only works for specific arm geometries |
| **Numerical** | TRAC-IK (within MoveIt 2) | Robust, handles redundancy and joint limits, used in production |

TRAC-IK iteratively solves $J^+ \cdot \Delta x = \Delta q$ using a pseudo-inverse of the Jacobian, with random restarts to escape local minima. It typically converges in < 1ms.

---

## 4. Simulation vs. Hardware Toggle

One of the key design goals is **zero code changes** to switch between running in Gazebo simulation and running the real arm. This is achieved by swapping the `ros2_control` hardware interface plugin in the MoveIt launch file:

```yaml
# For simulation (Gazebo):
ros2_control:
  hardware:
    plugin: "gazebo_ros2_control/GazeboSystem"

# For real hardware:
ros2_control:
  hardware:
    plugin: "robot_arm_hw/SerialInterface"
    params:
      port: "/dev/ttyUSB0"
      baud: 115200
```

The rest of the software stack — MoveIt, task scripts, RViz — is **identical** in both modes.

---

## 5. Repository Layout (`/software/`)

```
software/
├── robot_arm_description/    # URDF, meshes, rviz configs
├── robot_arm_moveit/         # MoveIt 2 config, SRDF, kinematics.yaml
├── robot_arm_hw/             # ros2_control hardware interface plugin (C++)
├── robot_arm_tasks/          # High-level Python task nodes (pick & place, etc.)
├── launch/
│   ├── sim.launch.py         # Launches Gazebo + MoveIt + RViz
│   └── real.launch.py        # Launches hw_interface + MoveIt + RViz
└── docker-compose.yml        # Container definitions for the above
```
