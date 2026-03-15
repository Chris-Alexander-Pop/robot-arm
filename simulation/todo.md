# Simulation Roadmap

This document outlines the kinematic models and visual simulation environments for testing without hardware.

## DEVELOPMENT
*Kinematic modeling and foundational simulation setup - can run parallel to physical prototyping.*

- [ ] **DH Parameter Estimation**: Draft the Denavit-Hartenberg parameters based on preliminary CAD dimensions (`dh_solver.py`).
- [ ] **Forward Kinematics (FK)**: Create a pure Python FK solver to validate coordinate transforms for the 6-DOF arm.
- [ ] **Inverse Kinematics (IK) Prototyping**: Implement and test analytical or Jacobian-based IK solvers on arbitrary points.
- [ ] **URDF Drafting**: Create a basic URDF format file using primitive shapes (cylinders/boxes) to model the joint hierarchy.
- [ ] **RViz2 Visualization**: Set up a ROS 2 launch file to visualize the primitive arm and TF coordinate frames.

## INTEGRATION
*Full physics simulation and digital twin matching - after finalized CAD and hardware.*

- [ ] **URDF Refinement**: Extract actual physical properties (mass, center of mass, inertia tensors) from final CAD and update the URDF.
- [ ] **Visual Assets Integration**: Export finished CAD parts to STLs, decimate them, and link them as visual meshes in the URDF.
- [ ] **Gazebo/Ignition Setup**: Configure a physics world with precise joint friction, gravity, and collision models.
- [ ] **ROS 2 Control Plugins**: Implement `ros2_control` hardware interfaces to simulate stepper motor dynamics (torque curves, microstepping behavior) in Gazebo.
- [ ] **Digital Twin Validation**: Compare the physical robot's reach, joint limits, and movement speeds with the Gazebo simulation and tune the simulation parameters to match reality.
