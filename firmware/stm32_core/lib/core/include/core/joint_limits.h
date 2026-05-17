#pragma once

#include "core/types.h"

namespace robot_arm {

// Per-joint travel limits expressed in degrees in the **same frame** as JointCommand /
// ROS setpoints — typically relative to last homed-zero. Clamping rejects targets that
// would wind cable bundles or exceed mechanical envelopes.
//
// Tune per joint based on umbilical routing and hard stops (base azimuth vs wrist
// often differ sharply). Optionally call AlignLimitsToMeasured() after homing so the
// same numeric band tracks the physical datum.
struct JointMotionLimits {
  float min_deg[kJointCount];
  float max_deg[kJointCount];
};

// Conservative defaults (~±350°): wide enough not to pinch unit tests, narrow enough to
// block runaway winding until you replace them with calibrated values.
void DefaultJointMotionLimits(JointMotionLimits* out);

// Clamp each commanded joint angle into [min_deg, max_deg] for that joint.
void ClampJointCommand(const JointMotionLimits& limits, JointCommand* command);

// Re-center the limit window around the measured pose — call once after successful homing
// so allowable travel stays symmetric about the verified physical zero.
void AlignLimitsToMeasured(const JointState& measured, float half_span_deg, JointMotionLimits* limits);

}  // namespace robot_arm
