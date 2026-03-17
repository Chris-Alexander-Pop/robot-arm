#pragma once

#include <vector>

#include "robot_core/domain/robot_state.hpp"

namespace robot_core {

class TrajectoryPlanner {
 public:
  std::vector<MotionGoal> PlanJointTrajectory(const RobotState& start, const MotionGoal& goal) const;
};

}  // namespace robot_core
