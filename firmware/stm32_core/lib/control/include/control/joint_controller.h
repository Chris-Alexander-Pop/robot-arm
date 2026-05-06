#pragma once

#include "control/pid_controller.h"
#include "core/types.h"
#include "drivers/stepper_driver.h"

namespace robot_arm {

class JointController {
 public:
  explicit JointController(StepperDriver& stepper_driver);

  void SetCommand(const JointCommand& command);
  void UpdateFromSensors(const JointState& measured_state);

  // Runs one control step of duration dt_s seconds.
  //
  // TODO(contributor): implement per-joint velocity commanding in joint_controller.cpp.
  //
  // Hardware context: CL57T / CL42T drivers handle closed-loop correction internally.
  // The STM32 does not receive encoder feedback, so measured_state_ position values
  // are 0 (see EncoderDriver).  A simple approach is a proportional velocity command
  // based on the commanded position target — the driver ensures the motor follows:
  //
  //   for (int joint = 0; joint < kJointCount; ++joint) {
  //     float error = command_.target_position_deg[joint]
  //                   - measured_state_.position_deg[joint];
  //     float vel = pid_[joint].Update(error, dt_s);
  //     stepper_driver_.SetJointVelocityDegS(joint, vel);
  //   }
  //
  // PidController::Update() is fully implemented — tune kp/ki/kd per joint.
  // If you add a real position sensor later, wire it through EncoderDriver and
  // UpdateFromSensors() without changing this interface.
  void Step(float dt_s);

  const JointCommand& command() const;
  const JointState& measured_state() const;

 private:
  StepperDriver& stepper_driver_;
  PidController pid_[kJointCount];
  JointCommand command_{};
  JointState measured_state_{};
};

}  // namespace robot_arm
