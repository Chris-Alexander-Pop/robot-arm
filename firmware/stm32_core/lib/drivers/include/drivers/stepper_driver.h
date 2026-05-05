#pragma once

#include "core/types.h"

namespace robot_arm {

class StepperDriver {
 public:
  bool Init();
  void SetJointVelocityDegS(int joint_index, float velocity_deg_s);
  void Tick();

  bool initialized() const;
  float joint_velocity_deg_s(int joint_index) const;

 private:
  bool initialized_ = false;
  float joint_velocity_deg_s_[kJointCount]{};
};

}  // namespace robot_arm
