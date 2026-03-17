#include "robot_core/domain/kinematics_solver.hpp"

namespace robot_core {

RobotState KinematicsSolver::SolveForward(const RobotState& state) const {
  // TODO: replace with DH or URDF-derived forward kinematics.
  return state;
}

MotionGoal KinematicsSolver::SolveInverse(double x, double y, double z, double roll, double pitch, double yaw) const {
  (void)x;
  (void)y;
  (void)z;
  (void)roll;
  (void)pitch;
  (void)yaw;

  // TODO: replace with numeric IK solver and limit handling.
  return MotionGoal{};
}

}  // namespace robot_core
