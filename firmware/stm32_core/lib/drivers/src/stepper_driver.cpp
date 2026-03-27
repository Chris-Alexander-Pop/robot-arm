#include "drivers/stepper_driver.h"

namespace robot_arm {

bool StepperDriver::Init() {
  // TODO: configure step/direction output pins and timers.
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  (void)joint_index;
  (void)velocity_deg_s;
  // TODO: map velocity to pulse-rate output for each axis.
}

void StepperDriver::Tick() {
  // TODO: service pulse generation at deterministic frequency.
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
