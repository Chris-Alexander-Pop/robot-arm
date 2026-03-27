#include "control/joint_controller.h"

namespace robot_arm {

JointController::JointController() = default;

void JointController::SetCommand(const JointCommand& command) {
  command_ = command;
}

void JointController::UpdateFromSensors(const JointState& measured_state) {
  measured_state_ = measured_state;
}

void JointController::Step(float dt_s) {
  (void)dt_s;
  // TODO: run per-joint PID loops and send outputs to stepper driver.
}

const JointCommand& JointController::command() const {
  return command_;
}

const JointState& JointController::measured_state() const {
  return measured_state_;
}

}  // namespace robot_arm
