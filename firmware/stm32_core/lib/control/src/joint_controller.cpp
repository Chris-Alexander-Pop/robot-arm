#include "control/joint_controller.h"

namespace robot_arm {

JointController::JointController(StepperDriver& stepper_driver) : stepper_driver_(stepper_driver), pid_{} {
  DefaultJointMotionLimits(&limits_);
}

void JointController::SetMotionLimits(const JointMotionLimits& limits) {
  limits_ = limits;
}

const JointMotionLimits& JointController::motion_limits() const {
  return limits_;
}

void JointController::SetCommand(const JointCommand& command) {
  command_ = command;
  ClampJointCommand(limits_, &command_);
}

void JointController::UpdateFromSensors(const JointState& measured_state) {
  measured_state_ = measured_state;
}

void JointController::Step(float dt_s) {
  // TODO(contributor): implement the per-joint supervisory loop (degrees → commanded velocity → StepperDriver):
  //
  //  1. If dt_s <= 0, skip PidController::Update (already returns 0) and optionally force-all-zero
  //     stepper velocities for predictability once you emit motion.
  //  2. For joint j:
  //        error[j] = command_.target_position_deg[j] - measured_state_.position_deg[j]
  //     (targets are already clamped by JointMotionLimits in SetCommand()).
  //  3. vel[j] = pid_[j].Update(error[j], dt_s) — initialise pid_[j] in the constructor once you have
  //     stable gains (start with kp>0 only; Ki/Kd incrementally during hardware bring-up).
  //  4. Saturate |vel| to a sane max °/s, then stepper_driver_.SetJointVelocityDegS(j, vel[j]).
  //
  // Wiring note: EncoderDriver reads are 0 on CL57T/CL42T until you expose absolute feedback to the MCU —
  // that changes how you tune or observe this loop vs testing on host with simulated measured_state.
  (void)dt_s;
}

const JointCommand& JointController::command() const {
  return command_;
}

const JointState& JointController::measured_state() const {
  return measured_state_;
}

}  // namespace robot_arm
