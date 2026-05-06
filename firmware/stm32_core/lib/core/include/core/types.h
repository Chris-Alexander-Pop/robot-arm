#pragma once

#include <stdint.h>

namespace robot_arm {

constexpr int kJointCount = 6;

struct JointState {
  float position_deg[kJointCount];
  float velocity_deg_s[kJointCount];
};

struct JointCommand {
  float target_position_deg[kJointCount];
};

struct ControlGains {
  float kp;
  float ki;
  float kd;
};

}  // namespace robot_arm
