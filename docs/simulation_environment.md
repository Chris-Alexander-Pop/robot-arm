# Simulation Environment

## What is a Robot Simulation?

A simulation is a virtual 3D environment where a mathematically perfect digital twin of your arm lives. 
Instead of sending motor commands to the physical STM32 and risking your robot smashing into a desk (or breaking its 3D-printed parts), you send those exact same commands to the virtual robot.

## Why is it required?
1. **Kinematics Testing**: When you tell your arm to move the wrist to `(X: 10, Y: 5, Z: 20)`, the Inverse Kinematics math calculates what the 6 joint angles need to be. If the math is wrong, the arm might tie itself in a knot. The simulation visualizes this instantly.
2. **Collision Avoidance**: If you tell the arm to pick up an object, does the elbow hit the table on the way down? The simulation engine checks for physical collisions automatically.
3. **Software Development**: You can write 90% of your high-level Python/C++ code on your laptop without the physical robot even being assembled yet.

## How does it work?

### 1. The URDF (Unified Robot Description Format)
This is an XML file that describes the "bones and joints" of your robot. You will export this file from your CAD software (like SolidWorks or Fusion 360). It contains:
* The 3D meshes (the visual look of the links).
* The mass and inertia of each link.
* The physical limits of each joint (e.g., "Joint 2 can only rotate between -90 and +90 degrees").

### 2. The Physics Engine and Control (ROS 2 + Gazebo + MoveIt!)
For this project, we have selected the industry standard stack:

* **Gazebo**: The 3D rigid body simulator. It takes the URDF and simulates physics, gravity, and collisions.
* **ROS 2 (Robot Operating System)**: The middleware that passes messages between the simulator, your Python/C++ control scripts, and eventually the real robot hardware.
* **MoveIt!**: The premier motion planning framework within ROS. It handles inverse kinematics, collision checking, and smooth trajectory generation out of the box.

**The Architecture:**
1. A Python or C++ node running in ROS 2 sends a Cartesian goal (e.g., "Move hand to XYZ").
2. MoveIt! calculates a safe trajectory (a series of joint angles over time).
3. MoveIt! sends this trajectory to `ros2_control`.
4. `ros2_control` sends the joint commands to Gazebo (if simulating) OR via serial/CAN to the STM32 (if running the real arm).

## The Workflow
1. Export URDF from CAD.
2. Load URDF into Gazebo.
3. Configure MoveIt! with the URDF.
4. Write a ROS 2 node that sends goal poses to MoveIt!.
5. Watch the virtual arm move in Gazebo and visualize the paths in RViz.
6. Once validated, swap the hardware interface from "GazeboSystem" to "SerialController" to command the real STM32.
