# Simulation Roadmap

This folder contains kinematic models and visual simulation environments for testing without hardware.

## Epic: Kinematic Modeling
- [ ] **DH Parameter Refinement**: Align `dh_solver.py` with the actual CAD dimensions.
- [ ] **Forward Kinematics (FK)**: Finalize the Python FK solver for 6-DOF.
- [ ] **Inverse Kinematics (IK)**: Implement an analytical or numerical IK solver for coordinate-based control.

## Epic: ROS 2 Visualization & URDF
- [ ] **URDF Model**: Create a Unified Robot Description Format file for the arm links and joints.
- [ ] **Visual Assets**: Convert CAD STLs to simplified meshes for RViz2/Gazebo visualization.
- [ ] **RViz2 Integration**: Create a launch file to visualize the arm joints and coordinate frames.

## Epic: Physics Simulation
- [ ] **Gazebo/Ignition Setup**: Configure a physics world with gravity and collision models.
- [ ] **Control Plugins**: Implement ROS 2 Control plugins to simulate stepper motor behavior in the physics engine.
