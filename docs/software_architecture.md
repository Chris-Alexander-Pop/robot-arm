# Software Architecture (Raspberry Pi 3)

## Operating System
* Ubuntu Server with ROS 2 (Robot Operating System) [TBD based on decision].

## Core Components
1.  **Forward/Inverse Kinematics**:
    * Translating Cartesian coordinates (X, Y, Z, Roll, Pitch, Yaw) to 6 joint angles.
2.  **Trajectory Planning**:
    * Generating smooth movement profiles (e.g., trapezoidal velocity or S-curve) to avoid jerking.
3.  **Hardware Interface Node**:
    * Custom code that talks to the microcontroller firmware over Serial/CAN.
4.  **User Interface / API**:
    * How the user commands the arm (e.g., a web dashboard, command line, or joystick).
