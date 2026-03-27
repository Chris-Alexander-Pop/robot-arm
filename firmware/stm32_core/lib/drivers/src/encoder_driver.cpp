#include "drivers/encoder_driver.h"

namespace robot_arm {

bool EncoderDriver::Init() {
  // TODO: initialize I2C and probe all AS5600 channels.
  return true;
}

float EncoderDriver::ReadJointAngleDeg(int joint_index) const {
  (void)joint_index;
  // TODO: read true angle for specified joint.
  return 0.0F;
}

void EncoderDriver::SetSimulatedJointAngleDegForTest(int joint_index, float angle_deg) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }

  simulated_joint_angle_deg_[joint_index] = angle_deg;
}

bool EncoderDriver::initialized() const {
  return initialized_;
}

float EncoderDriver::simulated_joint_angle_deg(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }

  return simulated_joint_angle_deg_[joint_index];
}

}  // namespace robot_arm
