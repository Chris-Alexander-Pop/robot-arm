#pragma once

#include <array>

namespace robot_core {

constexpr std::size_t kJointCount = 6;

struct RobotState {
  std::array<double, kJointCount> joint_position_rad{};
  std::array<double, kJointCount> joint_velocity_rad_s{};
};

struct MotionGoal {
  std::array<double, kJointCount> target_joint_rad{};
};

}  // namespace robot_core
