#include "control/joint_controller.h"

namespace robot_arm {

JointController::JointController(StepperDriver& stepper_driver)
    : stepper_driver_(stepper_driver) {}

void JointController::SetCommand(const JointCommand& command) {
  command_ = command;
}

void JointController::UpdateFromSensors(const JointState& measured_state) {
  measured_state_ = measured_state;
}

void JointController::Step(float dt_s) {
  // TODO(contributor): implement the per-joint PID control loop here.
  // For each joint, compute the position error, run it through the PID
  // controller, and forward the resulting velocity command to the stepper:
  //
  //   for (int joint = 0; joint < kJointCount; ++joint) {
  //     float error = command_.target_position_deg[joint]
  //                   - measured_state_.position_deg[joint];
  //     float vel = pid_[joint].Update(error, dt_s);
  //     stepper_driver_.SetJointVelocityDegS(joint, vel);
  //   }
  //
  // PidController::Update() is already implemented in pid_controller.cpp.
  // Tune kp / ki / kd gains per joint via the PidController constructor
  // (see joint_controller.h where pid_[] is declared).
  (void)dt_s;
}

const JointCommand& JointController::command() const {
  return command_;
}

const JointState& JointController::measured_state() const {
  return measured_state_;
}

}  // namespace robot_arm
