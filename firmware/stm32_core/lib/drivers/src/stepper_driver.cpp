#include "drivers/stepper_driver.h"

namespace robot_arm {

bool StepperDriver::Init() {
  initialized_ = true;
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }

  joint_velocity_deg_s_[joint_index] = velocity_deg_s;
}

void StepperDriver::Tick() {
}

bool StepperDriver::initialized() const {
  return initialized_;
}

float StepperDriver::joint_velocity_deg_s(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }

  return joint_velocity_deg_s_[joint_index];
}

}  // namespace robot_arm
