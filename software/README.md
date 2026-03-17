# Raspberry Pi Software

High-level robot orchestration stack.

## Layout
- `ros2_ws/src/robot_core`: runtime control package.
- `ros2_ws/src/robot_description`: URDF/launch/rviz assets.
- `Dockerfile`, `docker-compose.yml`: reproducible dev/runtime environment.

Generated ROS artifacts in `ros2_ws/build`, `ros2_ws/install`, and `ros2_ws/log` should not contain source code.
