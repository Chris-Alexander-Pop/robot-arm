#pragma once

#include "robot_core/domain/robot_state.hpp"

namespace robot_core {

class HardwareInterface {
 public:
  virtual ~HardwareInterface() = default;

  virtual bool Connect() = 0;
  virtual bool SendGoal(const MotionGoal& goal) = 0;
  virtual RobotState ReadState() = 0;
};

}  // namespace robot_core
