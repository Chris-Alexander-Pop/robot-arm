#pragma once

#include "control/pid_controller.h"
#include "core/joint_limits.h"
#include "core/types.h"
#include "drivers/stepper_driver.h"

namespace robot_arm {

class JointController {
 public:
  explicit JointController(StepperDriver& stepper_driver);

  void SetCommand(const JointCommand& command);
  void UpdateFromSensors(const JointState& measured_state);

  void SetMotionLimits(const JointMotionLimits& limits);
  const JointMotionLimits& motion_limits() const;

  // Supervisor loop (degrees -> velocities -> StepperDriver):
  // 1. compare clamped targets vs measured_state_, 2. run pid_[joint], 3. saturate, 4. command stepper_driver_.
  // See joint_limits / SetMotionLimits / AlignLimitsToMeasured for anti cable-wrap clamps (Tier 1.5).
  void Step(float dt_s);

  const JointCommand& command() const;
  const JointState& measured_state() const;

 private:
  StepperDriver& stepper_driver_;
  PidController pid_[kJointCount];
  JointMotionLimits limits_{};
  JointCommand command_{};
  JointState measured_state_{};
};

}  // namespace robot_arm
