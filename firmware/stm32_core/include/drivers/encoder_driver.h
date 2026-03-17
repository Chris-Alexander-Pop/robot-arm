#pragma once

namespace robot_arm {

class EncoderDriver {
 public:
  bool Init();
  float ReadJointAngleDeg(int joint_index) const;
};

}  // namespace robot_arm
