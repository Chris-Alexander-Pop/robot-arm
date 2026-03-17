#include "robot_core/domain/trajectory_planner.hpp"

namespace robot_core {

std::vector<MotionGoal> TrajectoryPlanner::PlanJointTrajectory(const RobotState& start, const MotionGoal& goal) const {
  (void)start;
  // TODO: replace with trapezoidal or S-curve planner.
  return {goal};
}

}  // namespace robot_core
