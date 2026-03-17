#pragma once

namespace robot_arm {

class StepperDriver {
 public:
  bool Init();
  void SetJointVelocityDegS(int joint_index, float velocity_deg_s);
  void Tick();
};

}  // namespace robot_arm
