# Raspberry Pi Software

High-level robot orchestration stack.

## Layout
- `ros2_ws/src/robot_core`: runtime control package.
- `ros2_ws/src/robot_description`: URDF/launch/rviz assets.
- `ros2_ws/src/robot_arm_moveit`: MoveIt 2 configuration and smoke tests.
- `Dockerfile`, `docker-compose.yml`: reproducible dev/runtime environment.

## Onboarding
- Start the ROS container with `./scripts/dev.sh up` from the repository root.
- Build the ROS workspace with `software/scripts/build.sh` on Linux/macOS.
- Build the ROS workspace with `software/scripts/build.ps1` on Windows.
- Run the MoveIt smoke tests with `software/scripts/test.sh` on Linux/macOS.
- Run the MoveIt smoke tests with `software/scripts/test.ps1` on Windows.

## MoveIt Bringup
- Use `./scripts/dev.sh moveit-sim` to build the workspace and launch the MoveIt + RViz simulation entry point.
- Use `./scripts/dev.sh moveit-real` to build the workspace and launch the MoveIt + RViz real-hardware entry point.

## STM32 Simulation
- Use Renode as the preferred STM32F401RE emulator for firmware-level integration tests.
- Keep the simulator entry points and test harnesses in this area so the MCU can be exercised without a physical board.
- Use the simulation package for kinematics and trajectory logic; use Renode for firmware behavior and peripheral interaction.
- Prefer headless/console mode so Renode prints to the terminal instead of opening separate analyzer windows.

## MoveIt
- `ros2_ws/src/robot_arm_moveit` holds the MoveIt 2 config package, launch entrypoint, and smoke tests.
- `sim.launch.py` and `real.launch.py` are the documented entry points; both currently wrap the shared headless MoveIt bringup and add RViz.
- Use the MoveIt launch file for planner bringup once the ROS 2 MoveIt runtime is installed in the container or host environment.

Generated ROS artifacts in `ros2_ws/build`, `ros2_ws/install`, and `ros2_ws/log` should not contain source code.
