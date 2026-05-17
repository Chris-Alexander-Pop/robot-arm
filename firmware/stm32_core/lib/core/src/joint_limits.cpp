#include "core/joint_limits.h"

#include <algorithm>

namespace robot_arm {

void DefaultJointMotionLimits(JointMotionLimits* out) {
  if (out == nullptr) {
    return;
  }
  for (int joint = 0; joint < kJointCount; ++joint) {
    constexpr float span_half = 350.0F;
    out->min_deg[joint] = -span_half;
    out->max_deg[joint] = span_half;
  }
}

void ClampJointCommand(const JointMotionLimits& limits, JointCommand* command) {
  if (command == nullptr) {
    return;
  }
  for (int joint = 0; joint < kJointCount; ++joint) {
    command->target_position_deg[joint] = std::min(
        std::max(command->target_position_deg[joint], limits.min_deg[joint]), limits.max_deg[joint]);
  }
}

void AlignLimitsToMeasured(const JointState& measured, float half_span_deg, JointMotionLimits* limits) {
  if (limits == nullptr || half_span_deg <= 0.0F) {
    return;
  }
  for (int joint = 0; joint < kJointCount; ++joint) {
    const float center = measured.position_deg[joint];
    limits->min_deg[joint] = center - half_span_deg;
    limits->max_deg[joint] = center + half_span_deg;
  }
}

}  // namespace robot_arm
