#pragma once

#include "core/types.h"

namespace robot_arm {

class EncoderDriver {
 public:
  bool Init();
  float ReadJointAngleDeg(int joint_index) const;

  void SetSimulatedJointAngleDegForTest(int joint_index, float angle_deg);
  bool initialized() const;
  float simulated_joint_angle_deg(int joint_index) const;

 private:
  bool initialized_ = false;
  float simulated_joint_angle_deg_[kJointCount]{};
};

}  // namespace robot_arm
