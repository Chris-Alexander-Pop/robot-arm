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

}  // namespace robot_arm
