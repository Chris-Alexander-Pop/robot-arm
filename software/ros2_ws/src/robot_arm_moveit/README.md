# robot_arm_moveit

MoveIt 2 configuration package for the robot arm.

## What is here
- `config/`: SRDF, kinematics, joint limits, controller, and OMPL settings.
- `launch/`: a headless MoveIt bringup launch file.
- `test/`: smoke tests that validate the generated config and launch wiring.

## Notes
- The launch file expects the ROS 2 MoveIt runtime packages to be available in the environment.
- The smoke tests are intentionally lightweight so they can run before the full hardware bridge exists.