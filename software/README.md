# Raspberry Pi Software

High-level robot orchestration stack.

## Layout
- `ros2_ws/src/robot_core`: runtime control package.
- `ros2_ws/src/robot_description`: URDF/launch/rviz assets.
- `Dockerfile`, `docker-compose.yml`: reproducible dev/runtime environment.

## Onboarding
- Start the ROS container with `./dev.sh up` from the repository root.
- Build the ROS workspace with `software/scripts/build.sh` on Linux/macOS.
- Build the ROS workspace with `software/scripts/build.ps1` on Windows.

## STM32 Simulation
- Use Renode as the preferred STM32F401RE emulator for firmware-level integration tests.
- Keep the simulator entry points and test harnesses in this area so the MCU can be exercised without a physical board.
- Use the simulation package for kinematics and trajectory logic; use Renode for firmware behavior and peripheral interaction.
- Prefer headless/console mode so Renode prints to the terminal instead of opening separate analyzer windows.

Generated ROS artifacts in `ros2_ws/build`, `ros2_ws/install`, and `ros2_ws/log` should not contain source code.
