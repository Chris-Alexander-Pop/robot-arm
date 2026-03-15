# Software Roadmap

This folder contains the high-level control logic and ROS 2 workspace running on the Raspberry Pi.

## Epic: ROS 2 Foundation
- [ ] **Workspace Setup**: Finalize the package structure for `robot_core` and `robot_description`.
- [ ] **STM32 Serial Bridge**: Implement a node that subscribes to joint commands and sends them to the firmware.
- [ ] **Heartbeat System**: Implement a safety "heartbeat" between the Pi and STM32.

## Epic: Motion Planning
- [ ] **MoveIt 2 Integration**: Configure MoveIt 2 for trajectory planning and collision avoidance.
- [ ] **Point-to-Point Movement**: Implement service calls to move the end-effector to specific (X, Y, Z, R, P, Y) coordinates.
- [ ] **Trajectory Execution**: Ensure smoothInterpolation of joint angles during complex movements.

## Epic: Higher-Level Operations
- [ ] **Pick-and-Place Logic**: Create a script/node for standard pick-and-place operation loops.
- [ ] **Computer Vision Integration (Future)**: Setup OpenCV/Camera nodes for detecting objects to pick.
- [ ] **Web Dashboard**: Create a simple web interface for manual teleoperation and state monitoring.
