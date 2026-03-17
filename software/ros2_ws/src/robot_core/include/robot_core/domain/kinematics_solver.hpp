#pragma once

#include "robot_core/domain/robot_state.hpp"

namespace robot_core {

class KinematicsSolver {
 public:
  RobotState SolveForward(const RobotState& state) const;
  MotionGoal SolveInverse(double x, double y, double z, double roll, double pitch, double yaw) const;
};

}  // namespace robot_core
