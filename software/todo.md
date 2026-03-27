# Software Roadmap

This document outlines the high-level control logic and ROS 2 workspace running on the Raspberry Pi.

## DEVELOPMENT
*Core workspace setup, serial communication, and isolated logic - before having a fully built arm.*

- [ ] **ROS 2 Workspace Setup**: Establish the package structure for `robot_core` and `robot_description`.
- [ ] **Hardware Bridge Node**: Write a ROS 2 node to communicate via Serial/UART with the STM32 (test with a single motor first).
- [ ] **Telemetry Pipeline**: Implement parsing of incoming serial data to publish joint states (encoders) to `/joint_states`.
- [ ] **Safety & Heartbeat**: Implement a software watchdog/heartbeat node that monitors connection health between Pi and STM32.
- [x] **MoveIt 2 Configuration Generation**: Use the MoveIt Setup Assistant with the preliminary URDF to generate move groups and kinematics plugins.
- [x] **MoveIt 2 Package Scaffold**: Added `robot_arm_moveit` with launch, configuration, and smoke tests.

## INTEGRATION
*Full trajectory planning, advanced controls, and application layers - after the full arm is built and responsive.*

- [ ] **MoveIt 2 Tuning**: Tune trajectory execution, allowed tolerances, and planning algorithms (e.g., OMPL) on the physical arm.
- [ ] **Action Servers**: Implement complete `FollowJointTrajectory` action servers to smoothly interpolate paths between MoveIt and the STM32.
- [ ] **Collision Avoidance**: Integrate octomap or static scene constraints to prevent the arm from hitting itself or the table.
- [ ] **Task Space Control**: Implement high-level Python/C++ scripts to move the end-effector to precise Cartesian coordinates (pick-and-place routines).
- [ ] **Web Dashboard**: Create a web interface (using rosbridge_suite) for manual teleoperation, E-stop triggers, and system diagnostics.
- [ ] **Vision System (Future)**: Integrate OpenCV and a camera node to dynamically identify coordinates for grasping.
